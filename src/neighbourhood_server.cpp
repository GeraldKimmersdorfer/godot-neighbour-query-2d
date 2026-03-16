#include "neighbourhood_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/print_string.hpp>

#include <chrono>
#include <cmath>
#include <limits>

void NeighbourhoodServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("subscribe", "node", "layer", "data"), &NeighbourhoodServer::subscribe);
	ClassDB::bind_method(D_METHOD("unsubscribe", "node"), &NeighbourhoodServer::unsubscribe);
	ClassDB::bind_method(D_METHOD("get_next", "position", "max_distance", "layer_mask"), &NeighbourhoodServer::get_next, DEFVAL(0.0f), DEFVAL(0xFFFFFFFF));
	ClassDB::bind_method(D_METHOD("get_all", "position", "max_distance", "layer_mask"), &NeighbourhoodServer::get_all, DEFVAL(0.0f), DEFVAL(0xFFFFFFFF));

	ClassDB::bind_method(D_METHOD("set_grid_size", "grid_size"), &NeighbourhoodServer::set_grid_size);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &NeighbourhoodServer::get_grid_size);

	ClassDB::bind_method(D_METHOD("set_refresh_intervall", "refresh_intervall"), &NeighbourhoodServer::set_refresh_intervall);
	ClassDB::bind_method(D_METHOD("get_refresh_intervall"), &NeighbourhoodServer::get_refresh_intervall);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_size"), "set_grid_size", "get_grid_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_intervall"), "set_refresh_intervall", "get_refresh_intervall");

#if DEBUG_GRID_INFORMATION
	ClassDB::bind_method(D_METHOD("get_last_queried_cells"), &NeighbourhoodServer::get_last_queried_cells);
#endif
}

void NeighbourhoodServer::_ready() {
	set_physics_process(true);
	print_line(vformat("[NeighbourhoodServer] Main thread ID: %d", OS::get_singleton()->get_thread_caller_id()));
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
	//print_line(vformat("[NeighbourhoodServer] _physics_process thread ID: %d", OS::get_singleton()->get_thread_caller_id()));
	refresh();
}

uint64_t NeighbourhoodServer::to_cell_key(int cell_x, int cell_y) {
	return (static_cast<uint64_t>(static_cast<uint32_t>(cell_x)) << 32) |
		   static_cast<uint64_t>(static_cast<uint32_t>(cell_y));
}

void NeighbourhoodServer::refresh() {
	//auto t_start = std::chrono::steady_clock::now();

	std::unordered_map<uint64_t, std::vector<Subscriber>> grid;
	for (auto &[node, subscriber] : m_subscribers) {
		subscriber.position = node->get_global_position();
		int cx = static_cast<int>(std::floor(subscriber.position.x / grid_size));
		int cy = static_cast<int>(std::floor(subscriber.position.y / grid_size));
		grid[to_cell_key(cx, cy)].push_back(subscriber);
	}

	{
		std::lock_guard<std::mutex> lock(m_grid_mutex);
		m_grid = std::move(grid);
	}

	//double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t_start).count();
	//print_line(vformat("[NeighbourhoodServer] Refresh: built %d cells from %d subscribers in %.3f ms",
	//		m_grid.size(), m_subscribers.size(), ms));
}

void NeighbourhoodServer::subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data) {
	m_subscribers[p_node] = { p_node, p_layer, p_data };
}

void NeighbourhoodServer::unsubscribe(Node2D *p_node) {
	m_subscribers.erase(p_node);
}

Variant NeighbourhoodServer::get_next(const Vector2 &p_position, float p_max_distance, uint32_t p_layer_mask) {
	std::lock_guard<std::mutex> lock(m_grid_mutex);

	int cx0 = static_cast<int>(std::floor(p_position.x / grid_size));
	int cy0 = static_cast<int>(std::floor(p_position.y / grid_size));

	float best_dist_sq = p_max_distance > 0.0f ? p_max_distance * p_max_distance : std::numeric_limits<float>::max();
	const Subscriber *best = nullptr;

#if DEBUG_GRID_INFORMATION
	m_last_queried_cells.clear();
#endif

	for (int r = 0; ; r++) {
		// Lower bound on world-space distance to any cell in ring r is (r-1)*grid_size.
		// If that already exceeds best found, no closer result can exist.
		// In other words: If point is in r we need to check r+1 too since it depends on where
		// the dot sits inside the cells r whether there can be a better result in r+1
		if (r > 0) {
			float min_ring_dist = static_cast<float>((r - 1) * grid_size);
			if (min_ring_dist * min_ring_dist >= best_dist_sq) {
				break;
			}
		}

		// Also stop if the ring is entirely beyond p_max_distance.
		if (p_max_distance > 0.0f && static_cast<float>(std::max(0, r - 1) * grid_size) > p_max_distance) {
			break;
		}

		auto check = [&](int cx, int cy) {
#if DEBUG_GRID_INFORMATION
			m_last_queried_cells.push_back(Vector2i(cx, cy));
#endif
			auto it = m_grid.find(to_cell_key(cx, cy));
			if (it == m_grid.end()) {
				return;
			}
			for (const Subscriber &s : it->second) {
				if ((s.layer & p_layer_mask) == 0) {
					continue;
				}
				float d = s.position.distance_squared_to(p_position);
				if (d < best_dist_sq) {
					best_dist_sq = d;
					best = &s;
				}
			}
		};

		if (r == 0) {
			check(cx0, cy0);
		} else {
			for (int cx = cx0 - r; cx <= cx0 + r; cx++) {
				check(cx, cy0 - r);
				check(cx, cy0 + r);
			}
			for (int cy = cy0 - r + 1; cy <= cy0 + r - 1; cy++) {
				check(cx0 - r, cy);
				check(cx0 + r, cy);
			}
		}
	}

	return best ? best->data : Variant();
}

Array NeighbourhoodServer::get_all(const Vector2 &p_position, float p_max_distance, uint32_t p_layer_mask) {
	std::lock_guard<std::mutex> lock(m_grid_mutex);
	Array result;
	float max_dist_sq = p_max_distance * p_max_distance;

	int min_cx = static_cast<int>(std::floor((p_position.x - p_max_distance) / grid_size));
	int max_cx = static_cast<int>(std::floor((p_position.x + p_max_distance) / grid_size));
	int min_cy = static_cast<int>(std::floor((p_position.y - p_max_distance) / grid_size));
	int max_cy = static_cast<int>(std::floor((p_position.y + p_max_distance) / grid_size));

#if DEBUG_GRID_INFORMATION
	m_last_queried_cells.clear();
#endif

	for (int cy = min_cy; cy <= max_cy; cy++) {
		for (int cx = min_cx; cx <= max_cx; cx++) {
#if DEBUG_GRID_INFORMATION
			m_last_queried_cells.push_back(Vector2i(cx, cy));
#endif
			auto it = m_grid.find(to_cell_key(cx, cy));
			if (it == m_grid.end()) {
				continue;
			}
			for (const Subscriber &s : it->second) {
				if ((s.layer & p_layer_mask) == 0) {
					continue;
				}
				if (s.position.distance_squared_to(p_position) > max_dist_sq) {
					continue;
				}
				result.push_back(s.data);
			}
		}
	}

	return result;
}

#if DEBUG_GRID_INFORMATION
Array NeighbourhoodServer::get_last_queried_cells() const {
	Array result;
	result.resize(m_last_queried_cells.size());
	for (int i = 0; i < (int)m_last_queried_cells.size(); i++) {
		result[i] = m_last_queried_cells[i];
	}
	return result;
}
#endif

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
