#pragma once

#ifdef DEBUG_ENABLED
#define DEBUG_INFORMATION 1
#else
#define DEBUG_INFORMATION 0
#endif

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <limits>
#include <unordered_map>
#include <vector>

#if DEBUG_INFORMATION
#include "debug_timer.h"
#endif

using namespace godot;

struct Subscriber {
	// We need to store the node_id for validity check (is_instance_valid does not exists in gdextension)
	uint64_t node_instance_id = 0;
	uint32_t layer = 0;
	Variant data;
	// NOTE: I already tried glm::vec2, also with intrinsics enabled, but no performance gain. godot::Vector2 is fine
	Vector2 position;
};

class NeighbourhoodServer : public Node2D {
	GDCLASS(NeighbourhoodServer, Node2D)

public:
	enum DebugHeatmapMode {
		CELL_READS = 0,  // how many times each cell was visited by query traversal
		QUERY_COUNTS = 1, // how many queries originated from within each cell
	};

private:
	int grid_size = 128;
	float refresh_intervall = 0.0f;
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

	// NOTE: We still keep the Node2D* reference as key for m_subscribers since we otherwise
	// have to cast node_instance_id to Node2D* and that seems expensive.
	// WARNING: It may point to freed memory so before use a validity check using the
	// node_instance_id is necessary!
	std::unordered_map<Node2D *, Subscriber> m_subscribers;

	// Flat cell array [cy * m_grid_cols + cx], dimensions derived from domain and grid_size.
	// m_grid_build is written by refresh() without holding the lock, then swapped with m_grid.
	int m_grid_cols = 0;
	int m_grid_rows = 0;
	int m_brute_force_threshold = 0;
	Vector2 m_domain_center;
	float m_domain_diagonal_half = 0.0f;
	std::vector<std::vector<Subscriber>> m_grid;
	std::vector<std::vector<Subscriber>> m_grid_build;

	inline bool is_cell_in_bounds(int cx, int cy) const {
		return cx >= 0 && cx < m_grid_cols && cy >= 0 && cy < m_grid_rows;
	}
	int to_cell_index(int cx, int cy) const;
	void _update_grid_dimensions();
	void refresh();
	Variant get_next_brute_force(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Variant get_next_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Variant get_next_first_brute_force(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Variant get_next_first_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_all_brute_force(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_all_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_closest_brute_force(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_closest_grid(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);

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
	NeighbourhoodServer() = default;
	~NeighbourhoodServer() override = default;

	void _ready() override;
	void _physics_process(double p_delta) override;
	
#if DEBUG_INFORMATION
	void _draw() override;
	void _process(double p_delta) override;
#endif

	void subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data);
	void unsubscribe(Node2D *p_node);
	Variant get_next(const Vector2 &p_position, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
	Variant get_next_random(const Vector2 &p_position, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
	Variant get_next_first(const Vector2 &p_position, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
	Array get_all(const Vector2 &p_position, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
	Array get_closest(const Vector2 &p_position, int p_max_count, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);

	void set_grid_size(int p_grid_size);
	int get_grid_size() const;

	void set_refresh_intervall(float p_refresh_intervall);
	float get_refresh_intervall() const;

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

VARIANT_ENUM_CAST(NeighbourhoodServer::DebugHeatmapMode);
