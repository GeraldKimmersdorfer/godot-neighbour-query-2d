#include "neighbourhood_server.h"

void NeighbourhoodServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("subscribe", "node", "data"), &NeighbourhoodServer::subscribe);
	ClassDB::bind_method(D_METHOD("unsubscribe", "node"), &NeighbourhoodServer::unsubscribe);
	ClassDB::bind_method(D_METHOD("get_next", "position", "max_distance"), &NeighbourhoodServer::get_next, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("get_all", "position", "max_distance"), &NeighbourhoodServer::get_all, DEFVAL(0.0f));

	ClassDB::bind_method(D_METHOD("set_grid_size", "grid_size"), &NeighbourhoodServer::set_grid_size);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &NeighbourhoodServer::get_grid_size);

	ClassDB::bind_method(D_METHOD("set_refresh_intervall", "refresh_intervall"), &NeighbourhoodServer::set_refresh_intervall);
	ClassDB::bind_method(D_METHOD("get_refresh_intervall"), &NeighbourhoodServer::get_refresh_intervall);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_size"), "set_grid_size", "get_grid_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_intervall"), "set_refresh_intervall", "get_refresh_intervall");
}

void NeighbourhoodServer::subscribe(Node2D *p_node, const Variant &p_data) {
	// stub
}

void NeighbourhoodServer::unsubscribe(Node2D *p_node) {
	// stub
}

Variant NeighbourhoodServer::get_next(const Vector2 &p_position, float p_max_distance) {
	return Variant();
}

Array NeighbourhoodServer::get_all(const Vector2 &p_position, float p_max_distance) {
	return Array();
}

void NeighbourhoodServer::set_grid_size(int p_grid_size) {
	grid_size = p_grid_size;
}

int NeighbourhoodServer::get_grid_size() const {
	return grid_size;
}

void NeighbourhoodServer::set_refresh_intervall(float p_refresh_intervall) {
	refresh_intervall = p_refresh_intervall;
}

float NeighbourhoodServer::get_refresh_intervall() const {
	return refresh_intervall;
}
