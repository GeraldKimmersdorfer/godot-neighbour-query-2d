#include "neighbour.h"

void Neighbour::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_node", "node"), &Neighbour::set_node);
	ClassDB::bind_method(D_METHOD("get_node"), &Neighbour::get_node);

	ClassDB::bind_method(D_METHOD("set_data", "data"), &Neighbour::set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &Neighbour::get_data);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "node", PROPERTY_HINT_NODE_TYPE, "Node2D"), "set_node", "get_node");
	ADD_PROPERTY(PropertyInfo(Variant::NIL, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "set_data", "get_data");
}

void Neighbour::set_node(Node2D *p_node) {
	node = p_node;
}

Node2D *Neighbour::get_node() const {
	return node;
}

void Neighbour::set_data(const Variant &p_data) {
	data = p_data;
}

Variant Neighbour::get_data() const {
	return data;
}
