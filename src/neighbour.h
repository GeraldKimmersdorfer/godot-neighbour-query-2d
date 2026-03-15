#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

class Neighbour : public RefCounted {
	GDCLASS(Neighbour, RefCounted)

	Node2D *node = nullptr;
	Variant data;

protected:
	static void _bind_methods();

public:
	Neighbour() = default;
	~Neighbour() override = default;

	void set_node(Node2D *p_node);
	Node2D *get_node() const;

	void set_data(const Variant &p_data);
	Variant get_data() const;
};
