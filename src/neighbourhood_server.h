#pragma once

#define DEBUG_INFORMATION 1

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

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

class NeighbourhoodServer : public Node {
	GDCLASS(NeighbourhoodServer, Node)

	int grid_size = 128;
	float refresh_intervall = 0.1f;
	double m_time_since_refresh = 0.0;
	// NOTE: get_global_position() accounts for a big portion of refresh time, so we
	// allow the user to sue get_position() instead
	bool use_global_position = true;

	// NOTE: We use a function pointer depending on use_global_position such that we
	// dont have to check use_global_position for each subscriber in each refresh iteration
	Vector2 (Node2D::*m_get_position)() const = &Node2D::get_global_position;

	// NOTE: We still keep they Node2D* reference as key for m_subscribers since we otherwise
	// have to cast node_instance_id to Node2D* and that seems expensive.
	// WARNING: It may point to freed memory so before use a validity check using the
	// node_instance_id is necessary!
	std::unordered_map<Node2D *, Subscriber> m_subscribers;
	std::unordered_map<uint64_t, std::vector<Subscriber>> m_grid;
	std::mutex m_grid_mutex;

	static uint64_t to_cell_key(int cell_x, int cell_y);
	void refresh();

#if DEBUG_INFORMATION
	std::vector<Vector2i> m_last_queried_cells;
#endif

protected:
	static void _bind_methods();

public:
	NeighbourhoodServer() = default;
	~NeighbourhoodServer() override = default;

	void _ready() override;
	void _physics_process(double p_delta) override;

	void subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data);
	void unsubscribe(Node2D *p_node);
	Variant get_next(const Vector2 &p_position, float p_max_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF);
	Array get_all(const Vector2 &p_position, float p_max_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF);

	void set_grid_size(int p_grid_size);
	int get_grid_size() const;

	void set_refresh_intervall(float p_refresh_intervall);
	float get_refresh_intervall() const;

	void set_use_global_position(bool p_use_global_position);
	bool get_use_global_position() const;

#if DEBUG_INFORMATION
	Array get_last_queried_cells() const;
#endif
};
