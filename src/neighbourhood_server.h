#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <unordered_map>

using namespace godot;

struct Subscriber {
	Node2D *node = nullptr;
	uint32_t layer = 0;
	Variant data;
};

class NeighbourhoodServer : public Object {
	GDCLASS(NeighbourhoodServer, Object)

	int grid_size = 128;
	float refresh_intervall = 0.1f;

	std::unordered_map<Node2D *, Subscriber> m_subscribers;

protected:
	static void _bind_methods();

public:
	NeighbourhoodServer() = default;
	~NeighbourhoodServer() override = default;

	void subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data);
	void unsubscribe(Node2D *p_node);
	Variant get_next(const Vector2 &p_position, float p_max_distance = 0.0f);
	Array get_all(const Vector2 &p_position, float p_max_distance = 0.0f);

	void set_grid_size(int p_grid_size);
	int get_grid_size() const;

	void set_refresh_intervall(float p_refresh_intervall);
	float get_refresh_intervall() const;
};
