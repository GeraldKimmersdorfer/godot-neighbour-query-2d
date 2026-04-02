#pragma once

#ifdef DEBUG_ENABLED
#define DEBUG_INFORMATION 1
#else
#define DEBUG_INFORMATION 1
#endif

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#if DEBUG_INFORMATION
#include "debug_timer.h"
#endif

using namespace godot;

// NOTE: I already tried returning a gdscript object with multiple parameters like squared distance,
// position, node reference such that we don't have to do certain calculations on gd script side.
// BUT instanzing those objects comes with too much overhead such that the get_all function was four
// times slower.

// Per-member data within a SubscriberGroup. moving_entity lives on the group; position is computed on the fly.
struct Subscriber {
	Node2D *node = nullptr;
	// We need to store the node_id for validity check (is_instance_valid does not exists in gdextension)
	uint64_t node_instance_id = 0;
	uint32_t layer = 0;
	// Offset from the group's moving entity position.
	Vector2 offset;
};

// One group per unique moving entity.
struct SubscriberGroup {
	Node2D *moving_entity = nullptr;
	uint64_t moving_entity_instance_id = 0;
	std::vector<Subscriber> members;
};

// Lean struct stored in grid cells containing only what the query functions need. (for better cache locality)
struct GridEntry {
	Node2D *node = nullptr;
	uint64_t node_instance_id = 0;
	uint32_t layer = 0;
	// NOTE: I already tried glm::vec2, also with intrinsics enabled, but no performance gain. godot::Vector2 is fine
	Vector2 position;
};

struct AABB2 {
	real_t min_x, min_y, max_x, max_y;
};

struct AABB2I {
	int min_x, min_y, max_x, max_y;
};

struct GridCell {
	std::vector<GridEntry> entries;
	AABB2 aabb; // tight bounding box of all entry positions; valid only when entries is non-empty

	// Returns true if the cell can be skipped entirely: either empty, or entirely outside [min_dist_sq, max_dist_sq].
	inline bool early_discard_check(const Vector2 &p_position, float min_dist_sq, float max_dist_sq) const {
		if (entries.empty()) {
			return true;
		}
		// Closest point on AABB. If that one is beyond max => the whole cell is too far.
		float ax = std::clamp(p_position.x, aabb.min_x, aabb.max_x);
		float ay = std::clamp(p_position.y, aabb.min_y, aabb.max_y);
		float adx = ax - p_position.x, ady = ay - p_position.y;
		if (adx * adx + ady * ady > max_dist_sq) {
			return true;
		}
		// Farthest point on AABB is always a corner. If that one is still closer than min => the whole cell is too close.
		float fdx = std::max(std::abs(p_position.x - aabb.min_x), std::abs(p_position.x - aabb.max_x));
		float fdy = std::max(std::abs(p_position.y - aabb.min_y), std::abs(p_position.y - aabb.max_y));
		return fdx * fdx + fdy * fdy < min_dist_sq;
	}
};

class NeighbourQuery2D : public Node2D {
	GDCLASS(NeighbourQuery2D, Node2D)

public:
	enum DebugHeatmapMode {
		CELL_READS = 0,  // how many times each cell was visited by query traversal
		QUERY_COUNTS = 1, // how many queries originated from within each cell
	};

private:
	int grid_size = 128;
	float refresh_intervall = 0.0f;
	float gc_interval = 1.0f;
	double m_time_since_refresh = 0.0;
	Rect2 domain = Rect2(0, 0, 1000, 600);
	bool debug_draw_domain = true;
	float debug_draw_heatmap_intervall = 1.0f;
	DebugHeatmapMode debug_heatmap_mode = CELL_READS;
	// NOTE: get_global_position() accounts for a big portion of refresh time, so we
	// allow the user to use get_position() instead
	bool use_global_position = true;
	float debug_report_interval = 1.0f;

	// NOTE: We use a function pointer depending on use_global_position such that we
	// dont have to check use_global_position for each subscriber in each refresh iteration
	Vector2 (Node2D::*m_get_position)() const = &Node2D::get_global_position;

	// Keyed by moving_entity pointer (nullptr = solo group).
	// WARNING: Keys may point to freed memory — always validate via moving_entity_instance_id before use.
	std::unordered_map<Node2D *, SubscriberGroup> m_groups;
	// Reverse lookup: subscriber node → its group key. Enables O(1) unsubscribe and GC cleanup.
	std::unordered_map<Node2D *, Node2D *> m_node_to_group;
	std::mutex m_mutex;

	// Background GC thread: once per second validates all subscribers and removes stale ones.
	std::thread m_gc_thread;
	std::atomic<bool> m_gc_stop{ false };
	std::condition_variable m_gc_cv;
	std::mutex m_gc_cv_mutex;
	void _gc_thread_func();
	void _stop_gc_thread();

	// Flat cell array [cy * m_grid_cols + cx], dimensions derived from domain and grid_size.
	// m_grid_build is written by refresh() without holding the lock, then swapped with m_grid.
	int m_grid_cols = 0;
	int m_grid_rows = 0;
	Vector2 m_domain_center;
	float m_domain_diagonal_half = 0.0f;
	std::vector<GridCell> m_grid;
	std::vector<GridCell> m_grid_build;

	inline bool is_cell_in_bounds(int cx, int cy) const {
		return cx >= 0 && cx < m_grid_cols && cy >= 0 && cy < m_grid_rows;
	}

	// Returns the inclusive grid cell index range that a circle of radius range centered at p_position overlaps.
	inline AABB2I get_cell_range(const Vector2 &p_position, float range) const {
		return {
			std::max(0, static_cast<int>(std::floor((p_position.x - range - domain.position.x) / grid_size))),
			std::max(0, static_cast<int>(std::floor((p_position.y - range - domain.position.y) / grid_size))),
			std::min(m_grid_cols - 1, static_cast<int>(std::floor((p_position.x + range - domain.position.x) / grid_size))),
			std::min(m_grid_rows - 1, static_cast<int>(std::floor((p_position.y + range - domain.position.y) / grid_size)))
		};
	}

	// Returns p_max_distance clamped to the farthest reachable point in the domain, to prevent overflow in cell range calculations.
	inline float clamp_query_range(const Vector2 &p_position, float p_max_distance) const {
		// dist-to-center + diagonal is a safe upper bound for the farthest reachable point in the domain.
		return std::min(p_max_distance, p_position.distance_to(m_domain_center) + m_domain_diagonal_half);
	}


	// Returns the row aligned one dimensional index for the given cell coordinates
	inline int to_cell_index(int cx, int cy) const {
		return cy * m_grid_cols + cx;
	}

	void _update_grid_dimensions();
	void refresh();
	Node2D *get_next_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Node2D *get_next_first_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_all_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_closest_grid(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_random_grid(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);

#if DEBUG_INFORMATION
	// CELL_READS: incremented each time a cell is visited during a query
	std::vector<int> m_grid_cellreads_debug;
	// QUERY_COUNTS: incremented once per query call, in the cell that contains the query position
	std::vector<int> m_grid_querycount_debug;
	double m_time_since_querycount_redraw = 0.0;
	double m_time_since_debug_report = 0.0;
	DebugTimer m_debug_timer;
	void emit_debug_report();
#endif

protected:
	static void _bind_methods();

public:
	NeighbourQuery2D() = default;
	~NeighbourQuery2D() override;

	void _ready() override;
	void _physics_process(double p_delta) override;
	
#if DEBUG_INFORMATION
	void _draw() override;
	void _process(double p_delta) override;
#endif

	void subscribe(Node2D *p_node, uint32_t p_layer, Node2D *p_moving_entity = nullptr, Vector2 p_offset = Vector2());
	void unsubscribe(Node2D *p_node);
	Node2D *get_next(const Vector2 &p_position, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
	Node2D *get_next_first(const Vector2 &p_position, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
	Array get_all(const Vector2 &p_position, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
	Array get_closest(const Vector2 &p_position, int p_max_count, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
	Array get_random(const Vector2 &p_position, int p_max_count, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);

	void set_grid_size(int p_grid_size);
	int get_grid_size() const;

	void set_refresh_intervall(float p_refresh_intervall);
	float get_refresh_intervall() const;

	void set_gc_interval(float p_gc_interval);
	float get_gc_interval() const;

	void set_use_global_position(bool p_use_global_position);
	bool get_use_global_position() const;

	void set_domain(const Rect2 &p_domain);
	Rect2 get_domain() const;

	void set_debug_draw_domain(bool p_debug_draw_domain);
	bool get_debug_draw_domain() const;

	void set_debug_draw_heatmap_intervall(float p_interval);
	float get_debug_draw_heatmap_intervall() const;

	void set_debug_heatmap_mode(DebugHeatmapMode p_mode);
	DebugHeatmapMode get_debug_heatmap_mode() const;

	void set_debug_report_interval(float p_interval);
	float get_debug_report_interval() const;
};

VARIANT_ENUM_CAST(NeighbourQuery2D::DebugHeatmapMode);
