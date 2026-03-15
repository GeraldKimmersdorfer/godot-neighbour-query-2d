#include "neighbourhood_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/print_string.hpp>

#include <cmath>

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

void NeighbourhoodServer::_ready() {
	set_physics_process(true);
}

void NeighbourhoodServer::_exit_tree() {
	stop_thread();
}

void NeighbourhoodServer::stop_thread() {
	if (!m_thread.joinable()) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(m_cv_mutex);
		m_running = false;
	}
	m_cv.notify_all();
	m_thread.join();
}

void NeighbourhoodServer::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	m_time_since_refresh += p_delta;
	if (refresh_intervall > 0.0f && m_time_since_refresh < refresh_intervall) {
		return;
	}
	m_time_since_refresh = 0.0;

	std::vector<Subscriber> snapshot;
	{
		std::lock_guard<std::mutex> lock(m_subscribers_mutex);
		snapshot.reserve(m_subscribers.size());
		for (auto &[node, subscriber] : m_subscribers) {
			subscriber.position = node->get_global_position();
			snapshot.push_back(subscriber);
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_cv_mutex);
		m_snapshot = std::move(snapshot);
		m_refresh_pending = true;
	}
	m_cv.notify_one();
}

uint64_t NeighbourhoodServer::to_cell_key(int cell_x, int cell_y) {
	return (static_cast<uint64_t>(static_cast<uint32_t>(cell_x)) << 32) |
		   static_cast<uint64_t>(static_cast<uint32_t>(cell_y));
}

void NeighbourhoodServer::thread_func() {
	while (true) {
		std::vector<Subscriber> snapshot;
		{
			std::unique_lock<std::mutex> lock(m_cv_mutex);
			m_cv.wait(lock, [this] { return m_refresh_pending || !m_running; });
			if (!m_running) {
				break;
			}
			snapshot = std::move(m_snapshot);
			m_refresh_pending = false;
		}
		build_grid(std::move(snapshot));
	}
}

void NeighbourhoodServer::build_grid(std::vector<Subscriber> p_snapshot) {
	std::unordered_map<uint64_t, std::vector<Subscriber>> grid;
	for (Subscriber &s : p_snapshot) {
		int cx = static_cast<int>(std::floor(s.position.x / grid_size));
		int cy = static_cast<int>(std::floor(s.position.y / grid_size));
		grid[to_cell_key(cx, cy)].push_back(std::move(s));
	}

	{
		std::lock_guard<std::mutex> lock(m_grid_mutex);
		m_grid = std::move(grid);
	}

	print_line("[NeighbourhoodServer] Refresh");
}

void NeighbourhoodServer::subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data) {
	bool start_thread = false;
	{
		std::lock_guard<std::mutex> lock(m_subscribers_mutex);
		m_subscribers[p_node] = { p_node, p_layer, p_data };
		start_thread = !m_thread.joinable();
	}
	if (start_thread) {
		std::lock_guard<std::mutex> lock(m_cv_mutex);
		m_running = true;
		m_thread = std::thread(&NeighbourhoodServer::thread_func, this);
	}
}

void NeighbourhoodServer::unsubscribe(Node2D *p_node) {
	bool should_stop = false;
	{
		std::lock_guard<std::mutex> lock(m_subscribers_mutex);
		m_subscribers.erase(p_node);
		should_stop = m_subscribers.empty();
	}
	if (should_stop) {
		stop_thread();
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
