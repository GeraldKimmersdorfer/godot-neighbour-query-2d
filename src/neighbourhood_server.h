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
#include <mutex>
#include <unordered_map>
#include <vector>

using namespace godot;

struct Subscriber {
	// We need to store the node_id for validity check (is_instance_valid does not exists in gdextension)
	uint64_t node_instance_id = 0;
	uint32_t layer = 0;
	Variant data;
	Vector2 position;
};

class NeighbourhoodServer : public Node2D {
	GDCLASS(NeighbourhoodServer, Node2D)

	int grid_size = 128;
	float refresh_intervall = 0.1f;
	double m_time_since_refresh = 0.0;
	Rect2 domain = Rect2(0, 0, 1000, 600);
	bool draw_domain = true;
	float draw_queries_refresh_interval = 1.0f;
	// NOTE: get_global_position() accounts for a big portion of refresh time, so we
	// allow the user to use get_position() instead
	bool use_global_position = true;

	// NOTE: We use a function pointer depending on use_global_position such that we
	// dont have to check use_global_position for each subscriber in each refresh iteration
	Vector2 (Node2D::*m_get_position)() const = &Node2D::get_global_position;

	// NOTE: We still keep they Node2D* reference as key for m_subscribers since we otherwise
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
	std::mutex m_grid_mutex;

	inline bool is_cell_in_bounds(int cx, int cy) const {
		return cx >= 0 && cx < m_grid_cols && cy >= 0 && cy < m_grid_rows;
	}
	int to_cell_index(int cx, int cy) const;
	void _update_grid_dimensions();
	void refresh();
	Variant get_next_brute_force(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Variant get_next_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_all_brute_force(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_all_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_closest_brute_force(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);
	Array get_closest_grid(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id);

#if DEBUG_INFORMATION
	std::vector<int> m_grid_querycount;
	double m_time_since_querycount_redraw = 0.0;
#endif

protected:
	static void _bind_methods();

public:
	NeighbourhoodServer() = default;
	~NeighbourhoodServer() override = default;

	void _init();
	void _ready() override;
	void _physics_process(double p_delta) override;
	void _draw() override;
#if DEBUG_INFORMATION
	void _process(double p_delta) override;
#endif

	void subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data);
	void unsubscribe(Node2D *p_node);
	Variant get_next(const Vector2 &p_position, float p_max_distance = std::numeric_limits<float>::max(), float p_min_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF, Node2D *p_exclude = nullptr);
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

	void set_draw_domain(bool p_draw_domain);
	bool get_draw_domain() const;

	void set_draw_queries_refresh_interval(float p_interval);
	float get_draw_queries_refresh_interval() const;
};
