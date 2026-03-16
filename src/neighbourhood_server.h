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
	Node2D *node = nullptr;
	uint32_t layer = 0;
	Variant data;
	Vector2 position;
};

class NeighbourhoodServer : public Node {
	GDCLASS(NeighbourhoodServer, Node)

	int grid_size = 128;
	float refresh_intervall = 0.1f;
	double m_time_since_refresh = 0.0;
	bool use_global_position = true;

	// Note: get_global_position() accounts for ~80% of refresh time; function pointer avoids
	// a per-subscriber branch so the choice is made once in the setter, not inside the hot loop.
	Vector2 (Node2D::*m_get_position)() const = &Node2D::get_global_position;

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
