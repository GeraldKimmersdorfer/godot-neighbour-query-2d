#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>

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

	std::unordered_map<Node2D *, Subscriber> m_subscribers;
	std::unordered_map<uint64_t, std::vector<Subscriber>> m_grid;
	std::mutex m_grid_mutex;

	static uint64_t to_cell_key(int cell_x, int cell_y);
	void refresh();

protected:
	static void _bind_methods();

public:
	NeighbourhoodServer() = default;
	~NeighbourhoodServer() override = default;

	void _ready() override;
	void _physics_process(double p_delta) override;

	void subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data);
	void unsubscribe(Node2D *p_node);
	Variant get_next(const Vector2 &p_position, float p_max_distance = 0.0f);
	Array get_all(const Vector2 &p_position, float p_max_distance = 0.0f, uint32_t p_layer_mask = 0xFFFFFFFF);

	void set_grid_size(int p_grid_size);
	int get_grid_size() const;

	void set_refresh_intervall(float p_refresh_intervall);
	float get_refresh_intervall() const;
};
