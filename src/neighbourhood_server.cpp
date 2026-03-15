#include "neighbourhood_server.h"

#include <godot_cpp/core/print_string.hpp>

#include <chrono>

void NeighbourhoodServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("subscribe", "node", "layer", "data"), &NeighbourhoodServer::subscribe);
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

void NeighbourhoodServer::thread_func() {
	while (m_running) {
		auto next = std::chrono::steady_clock::now();

		refresh();

		if (refresh_intervall <= 0.0f) {
			continue;
		}
		next += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
				std::chrono::duration<float>(refresh_intervall));
		std::this_thread::sleep_until(next);
	}
}

void NeighbourhoodServer::refresh() {
	print_line("[NeighbourhoodServer] Refresh");
}

void NeighbourhoodServer::subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data) {
	m_subscribers[p_node] = { p_node, p_layer, p_data };

	if (!m_running) {
		m_running = true;
		m_thread = std::thread(&NeighbourhoodServer::thread_func, this);
	}
}

void NeighbourhoodServer::unsubscribe(Node2D *p_node) {
	m_subscribers.erase(p_node);

	if (m_subscribers.empty() && m_running) {
		m_running = false;
		m_thread.join();
	}
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
