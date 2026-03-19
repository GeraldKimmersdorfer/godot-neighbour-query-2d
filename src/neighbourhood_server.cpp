#include "neighbourhood_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#if DEBUG_INFORMATION
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#endif
#include <godot_cpp/core/print_string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

void NeighbourhoodServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("subscribe", "node", "layer", "data"), &NeighbourhoodServer::subscribe);
	ClassDB::bind_method(D_METHOD("unsubscribe", "node"), &NeighbourhoodServer::unsubscribe);
	ClassDB::bind_method(D_METHOD("get_next", "position", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourhoodServer::get_next, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_all", "position", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourhoodServer::get_all, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_closest", "position", "max_count", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourhoodServer::get_closest, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));

	ClassDB::bind_method(D_METHOD("set_grid_size", "grid_size"), &NeighbourhoodServer::set_grid_size);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &NeighbourhoodServer::get_grid_size);

	ClassDB::bind_method(D_METHOD("set_refresh_intervall", "refresh_intervall"), &NeighbourhoodServer::set_refresh_intervall);
	ClassDB::bind_method(D_METHOD("get_refresh_intervall"), &NeighbourhoodServer::get_refresh_intervall);

	ClassDB::bind_method(D_METHOD("set_use_global_position", "use_global_position"), &NeighbourhoodServer::set_use_global_position);
	ClassDB::bind_method(D_METHOD("get_use_global_position"), &NeighbourhoodServer::get_use_global_position);

	ClassDB::bind_method(D_METHOD("set_domain", "domain"), &NeighbourhoodServer::set_domain);
	ClassDB::bind_method(D_METHOD("get_domain"), &NeighbourhoodServer::get_domain);

	ClassDB::bind_method(D_METHOD("set_draw_domain", "draw_domain"), &NeighbourhoodServer::set_draw_domain);
	ClassDB::bind_method(D_METHOD("get_draw_domain"), &NeighbourhoodServer::get_draw_domain);

	ClassDB::bind_method(D_METHOD("set_draw_queries_refresh_interval", "interval"), &NeighbourhoodServer::set_draw_queries_refresh_interval);
	ClassDB::bind_method(D_METHOD("get_draw_queries_refresh_interval"), &NeighbourhoodServer::get_draw_queries_refresh_interval);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_size"), "set_grid_size", "get_grid_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_intervall"), "set_refresh_intervall", "get_refresh_intervall");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_global_position"), "set_use_global_position", "get_use_global_position");
	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "domain"), "set_domain", "get_domain");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_domain"), "set_draw_domain", "get_draw_domain");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "draw_queries_refresh_interval"), "set_draw_queries_refresh_interval", "get_draw_queries_refresh_interval");

#if DEBUG_INFORMATION
	ADD_SIGNAL(MethodInfo("debug_info",
			PropertyInfo(Variant::STRING, "name"),
			PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)));
#endif
}

void NeighbourhoodServer::_draw() {
	if (!draw_domain) {
		return;
	}
	const Color border_color(1.0f, 1.0f, 1.0f, 0.8f);
	const Color fill_color(1.0f, 1.0f, 1.0f, 0.1f);
	const Color grid_color(1.0f, 1.0f, 1.0f, 0.4f);

	draw_rect(domain, fill_color, true);

#if DEBUG_INFORMATION
	{
		int max_count = 0;
		for (int c : m_grid_querycount) {
			max_count = std::max(max_count, c);
		}
		if (max_count > 0) {
			Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
			for (int cy = 0; cy < m_grid_rows; cy++) {
				for (int cx = 0; cx < m_grid_cols; cx++) {
					int count = m_grid_querycount[to_cell_index(cx, cy)];
					if (count == 0) {
						continue;
					}
					float t = static_cast<float>(count) / max_count;
					// from ColorBrewer: https://colorbrewer2.org/#type=sequential&scheme=Blues&n=9
					const Color low(0xf7 / 255.0f, 0xfb / 255.0f, 0xff / 255.0f, 0.5f);
					const Color high(0x08 / 255.0f, 0x30 / 255.0f, 0x6b / 255.0f, 0.5f);
					Vector2 cell_pos(domain.position.x + cx * grid_size, domain.position.y + cy * grid_size);
					draw_rect(Rect2(cell_pos, Vector2(grid_size, grid_size)), low.lerp(high, t), true);
					draw_string(font, cell_pos + Vector2(0, grid_size * 0.5f), String::num_int64(count), HORIZONTAL_ALIGNMENT_CENTER, grid_size, 12, Color(1, 1, 1, 1));
				}
			}
			std::fill(m_grid_querycount.begin(), m_grid_querycount.end(), 0);
		}
	}

#endif

	draw_rect(domain, border_color, false, 2.0f);

	float x0 = domain.position.x;
	float y0 = domain.position.y;
	float x1 = x0 + domain.size.x;
	float y1 = y0 + domain.size.y;

	// Extend to the next full cell boundary so the overflow beyond the domain is visible
	float x_end = x0 + std::ceil(domain.size.x / grid_size) * grid_size;
	float y_end = y0 + std::ceil(domain.size.y / grid_size) * grid_size;

	for (float x = x0; x <= x_end; x += grid_size) {
		draw_line(Vector2(x, y0), Vector2(x, y_end), grid_color, 1.0f);
	}
	for (float y = y0; y <= y_end; y += grid_size) {
		draw_line(Vector2(x0, y), Vector2(x_end, y), grid_color, 1.0f);
	}
}

void NeighbourhoodServer::_init() {
	_update_grid_dimensions();
	set_physics_process(true);
#if DEBUG_INFORMATION
	set_process(true);
#endif
}

void NeighbourhoodServer::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
#if DEBUG_INFORMATION
	// NOTE: Emit deferred such that we see it otherwise _ready runs before main _ready
	call_deferred("emit_signal", "debug_info", String("main_thread_id"), Variant(OS::get_singleton()->get_thread_caller_id()));
#endif
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
	refresh();
}

int NeighbourhoodServer::to_cell_index(int cx, int cy) const {
	return cy * m_grid_cols + cx;
}

void NeighbourhoodServer::_update_grid_dimensions() {
	m_grid_cols = std::max(1, static_cast<int>(std::ceil(domain.size.x / grid_size)));
	m_grid_rows = std::max(1, static_cast<int>(std::ceil(domain.size.y / grid_size)));
	m_domain_center = domain.position + domain.size * 0.5f;
	m_domain_diagonal_half = domain.size.length() * 0.5f;
	int cell_count = m_grid_cols * m_grid_rows;
#if DEBUG_INFORMATION
	m_brute_force_threshold = 1;
#else
	m_brute_force_threshold = 50;
#endif
	m_grid_build.assign(cell_count, {});
#if DEBUG_INFORMATION
	m_grid_querycount.assign(cell_count, 0);
#endif
	{
		std::lock_guard<std::mutex> lock(m_grid_mutex);
		m_grid.assign(cell_count, {});
	}
}

void NeighbourhoodServer::refresh() {
#if DEBUG_INFORMATION
	emit_signal("debug_info", "refresh_thread_id", OS::get_singleton()->get_thread_caller_id());
	auto t_start = std::chrono::steady_clock::now();
#endif

	// Clear build buffer in-place (keeps per-cell capacity to avoid repeated reallocations).
	for (auto &cell : m_grid_build) {
		cell.clear();
	}

	std::vector<Node2D *> invalid_nodes;

	for (auto &[node, subscriber] : m_subscribers) {
		if (UtilityFunctions::instance_from_id(subscriber.node_instance_id) == nullptr) {
			invalid_nodes.push_back(node);
			continue;
		}
		subscriber.position = (node->*m_get_position)();
		int cx = static_cast<int>(std::floor((subscriber.position.x - domain.position.x) / grid_size));
		int cy = static_cast<int>(std::floor((subscriber.position.y - domain.position.y) / grid_size));
		// Points outside the domain are ignored and not added to the AS.
		if (!is_cell_in_bounds(cx, cy)) {
			continue;
		}
		m_grid_build[to_cell_index(cx, cy)].push_back(subscriber);
	}

	for (Node2D *node : invalid_nodes) {
		m_subscribers.erase(node);
	}

	{
		std::lock_guard<std::mutex> lock(m_grid_mutex);
		std::swap(m_grid, m_grid_build);
	}

#if DEBUG_INFORMATION
	double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t_start).count();
	emit_signal("debug_info", "refresh_total_ms", ms);
#endif
}

void NeighbourhoodServer::subscribe(Node2D *p_node, uint32_t p_layer, const Variant &p_data) {
	m_subscribers[p_node] = { static_cast<uint64_t>(p_node->get_instance_id()), p_layer, p_data };
}

void NeighbourhoodServer::unsubscribe(Node2D *p_node) {
	m_subscribers.erase(p_node);
}

Variant NeighbourhoodServer::get_next_brute_force(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	float best_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;
	const bool check_validity = refresh_intervall != 0.0f;
	const Subscriber *best = nullptr;
	for (const auto &[node, s] : m_subscribers) {
		if ((s.layer & p_layer_mask) == 0) {
			continue;
		}
		if (s.node_instance_id == p_exclude_id) {
			continue;
		}
		float d = s.position.distance_squared_to(p_position);
		if (d >= best_dist_sq || d < min_dist_sq) {
			continue;
		}
		if (check_validity && UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
			continue;
		}
		best_dist_sq = d;
		best = &s;
	}
	return best ? best->data : Variant();
}

Array NeighbourhoodServer::get_all_brute_force(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	Array result;
	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;
	const bool check_validity = refresh_intervall != 0.0f;
	for (const auto &[node, s] : m_subscribers) {
		if ((s.layer & p_layer_mask) == 0) {
			continue;
		}
		if (s.node_instance_id == p_exclude_id) {
			continue;
		}
		float d = s.position.distance_squared_to(p_position);
		if (d > max_dist_sq || d < min_dist_sq) {
			continue;
		}
		if (check_validity && UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
			continue;
		}
		result.push_back(s.data);
	}
	return result;
}

Array NeighbourhoodServer::get_closest_brute_force(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	using Entry = std::pair<float, Variant>;
	auto cmp = [](const Entry &a, const Entry &b) { return a.first < b.first; };
	std::vector<Entry> heap;
	heap.reserve(p_max_count + 1);
	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;
	const bool check_validity = refresh_intervall != 0.0f;
	for (const auto &[node, s] : m_subscribers) {
		if ((s.layer & p_layer_mask) == 0) {
			continue;
		}
		if (s.node_instance_id == p_exclude_id) {
			continue;
		}
		float d = s.position.distance_squared_to(p_position);
		if (d > max_dist_sq || d < min_dist_sq) {
			continue;
		}
		if ((int)heap.size() >= p_max_count && d >= heap[0].first) {
			continue;
		}
		if (check_validity && UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
			continue;
		}
		heap.push_back({ d, s.data });
		std::push_heap(heap.begin(), heap.end(), cmp);
		if ((int)heap.size() > p_max_count) {
			std::pop_heap(heap.begin(), heap.end(), cmp);
			heap.pop_back();
		}
		if ((int)heap.size() == p_max_count) {
			max_dist_sq = std::min(max_dist_sq, heap[0].first);
		}
	}
	Array result;
	for (const auto &[d, data] : heap) {
		result.push_back(data);
	}
	return result;
}

Variant NeighbourhoodServer::get_next_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	std::lock_guard<std::mutex> lock(m_grid_mutex);

	int cx0 = static_cast<int>(std::floor((p_position.x - domain.position.x) / grid_size));
	int cy0 = static_cast<int>(std::floor((p_position.y - domain.position.y) / grid_size));

	// NOTE: Cap max_distance to guarantee loop termination.
	// dist-to-center + diagonal is a safe upper bound for the farthest reachable point in the domain.
	float upper_bound = p_position.distance_to(m_domain_center) + m_domain_diagonal_half;
	if (p_max_distance > upper_bound) {
		p_max_distance = upper_bound;
	}

	float best_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;
	const bool check_validity = refresh_intervall != 0.0f;
	const Subscriber *best = nullptr;

	for (int r = 0;; r++) {
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
		if (static_cast<float>(std::max(0, r - 1) * grid_size) > p_max_distance) {
			break;
		}

		auto check = [&](int cx, int cy) {
			if (!is_cell_in_bounds(cx, cy)) {
				return;
			}
			const int cell_idx = to_cell_index(cx, cy);
#if DEBUG_INFORMATION
			m_grid_querycount[cell_idx]++;
#endif
			for (const Subscriber &s : m_grid[cell_idx]) {
				if ((s.layer & p_layer_mask) == 0) {
					continue;
				}
				if (s.node_instance_id == p_exclude_id) {
					continue;
				}
				float d = s.position.distance_squared_to(p_position);
				if (d >= best_dist_sq || d < min_dist_sq) {
					continue;
				}
				if (check_validity && UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
					continue;
				}
				best_dist_sq = d;
				best = &s;
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

Variant NeighbourhoodServer::get_next(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	if ((int)m_subscribers.size() < m_brute_force_threshold) {
		return get_next_brute_force(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
	}
	return get_next_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
}

Array NeighbourhoodServer::get_all_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	Array result;

	std::lock_guard<std::mutex> lock(m_grid_mutex);

	// Cap to domain bounds to avoid overflow in cell range computation when p_max_distance is very large.
	float upper_bound = p_position.distance_to(m_domain_center) + m_domain_diagonal_half;
	float range = std::min(p_max_distance, upper_bound);
	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;
	const bool check_validity = refresh_intervall != 0.0f;

	int min_cx = std::max(0, static_cast<int>(std::floor((p_position.x - range - domain.position.x) / grid_size)));
	int max_cx = std::min(m_grid_cols - 1, static_cast<int>(std::floor((p_position.x + range - domain.position.x) / grid_size)));
	int min_cy = std::max(0, static_cast<int>(std::floor((p_position.y - range - domain.position.y) / grid_size)));
	int max_cy = std::min(m_grid_rows - 1, static_cast<int>(std::floor((p_position.y + range - domain.position.y) / grid_size)));

	for (int cy = min_cy; cy <= max_cy; cy++) {
		for (int cx = min_cx; cx <= max_cx; cx++) {
			const int cell_idx = to_cell_index(cx, cy);
#if DEBUG_INFORMATION
			m_grid_querycount[cell_idx]++;
#endif
			// NOTE: I already tried having template functions with static ifs to completely remove checks like validity, min distance,
			// and so on from the hot loop. It did not yield any significant change to the execution time. (same for the other get_ functions)
			for (const Subscriber &s : m_grid[cell_idx]) {
				if ((s.layer & p_layer_mask) == 0) {
					continue;
				}
				if (s.node_instance_id == p_exclude_id) {
					continue;
				}
				float d = s.position.distance_squared_to(p_position);
				if (d > max_dist_sq || d < min_dist_sq) {
					continue;
				}
				if (check_validity && UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
					continue;
				}
				result.push_back(s.data);
			}
		}
	}

	return result;
}

Array NeighbourhoodServer::get_all(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	if ((int)m_subscribers.size() < m_brute_force_threshold) {
		return get_all_brute_force(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
	}
	return get_all_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
}

Array NeighbourhoodServer::get_closest_grid(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	// NOTE: We use a maxheap keyed by squared distance such that heap[0] is always the farthest collected entry.
	// Using a heap here allows us O(logn) for insert whereas a sorted array would be O(n).
	using Entry = std::pair<float, Variant>;
	auto cmp = [](const Entry &a, const Entry &b) { return a.first < b.first; };
	std::vector<Entry> heap;
	heap.reserve(p_max_count + 1);

	{
		std::lock_guard<std::mutex> lock(m_grid_mutex);

		int cx0 = static_cast<int>(std::floor((p_position.x - domain.position.x) / grid_size));
		int cy0 = static_cast<int>(std::floor((p_position.y - domain.position.y) / grid_size));

		float upper_bound = p_position.distance_to(m_domain_center) + m_domain_diagonal_half;
		if (p_max_distance > upper_bound) {
			p_max_distance = upper_bound;
		}
		float max_dist_sq = p_max_distance * p_max_distance;
		float min_dist_sq = p_min_distance * p_min_distance;
		const bool check_validity = refresh_intervall != 0.0f;

		auto check = [&](int cx, int cy) {
			if (!is_cell_in_bounds(cx, cy)) {
				return;
			}
			const int cell_idx = to_cell_index(cx, cy);
#if DEBUG_INFORMATION
			m_grid_querycount[cell_idx]++;
#endif
			for (const Subscriber &s : m_grid[cell_idx]) {
				if ((s.layer & p_layer_mask) == 0) {
					continue;
				}
				if (s.node_instance_id == p_exclude_id) {
					continue;
				}
				float d = s.position.distance_squared_to(p_position);
				if (d > max_dist_sq || d < min_dist_sq) {
					continue;
				}
				if ((int)heap.size() >= p_max_count && d >= heap[0].first) {
					continue;
				}
				if (check_validity && UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
					continue;
				}
				heap.push_back({ d, s.data });
				std::push_heap(heap.begin(), heap.end(), cmp);
				if ((int)heap.size() > p_max_count) {
					std::pop_heap(heap.begin(), heap.end(), cmp);
					heap.pop_back();
				}
				if ((int)heap.size() == p_max_count) {
					max_dist_sq = std::min(max_dist_sq, heap[0].first);
				}
			}
		};

		for (int r = 0;; r++) {
			if (r > 0) {
				float min_ring_dist = static_cast<float>((r - 1) * grid_size);
				if (min_ring_dist * min_ring_dist >= max_dist_sq) {
					break;
				}
			}

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
	}

	Array result;
	for (const auto &[d, data] : heap) {
		result.push_back(data);
	}
	return result;
}

Array NeighbourhoodServer::get_closest(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
	if (p_max_count <= 0) {
		return Array();
	}
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	if ((int)m_subscribers.size() < m_brute_force_threshold) {
		return get_closest_brute_force(p_position, p_max_count, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
	}
	return get_closest_grid(p_position, p_max_count, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
}

#if DEBUG_INFORMATION
void NeighbourhoodServer::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	if (!draw_domain || draw_queries_refresh_interval < 0.0f) {
		return;
	}
	m_time_since_querycount_redraw += p_delta;
	if (m_time_since_querycount_redraw >= draw_queries_refresh_interval) {
		m_time_since_querycount_redraw = 0.0;
		queue_redraw();
	}
}
#endif

void NeighbourhoodServer::set_grid_size(int p_grid_size) {
	grid_size = p_grid_size;
	_update_grid_dimensions();
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
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

void NeighbourhoodServer::set_use_global_position(bool p_use_global_position) {
	use_global_position = p_use_global_position;
	m_get_position = use_global_position ? &Node2D::get_global_position : &Node2D::get_position;
}

bool NeighbourhoodServer::get_use_global_position() const {
	return use_global_position;
}

void NeighbourhoodServer::set_domain(const Rect2 &p_domain) {
	domain = p_domain;
	_update_grid_dimensions();
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
}

Rect2 NeighbourhoodServer::get_domain() const {
	return domain;
}

void NeighbourhoodServer::set_draw_domain(bool p_draw_domain) {
	draw_domain = p_draw_domain;
	queue_redraw();
}

bool NeighbourhoodServer::get_draw_domain() const {
	return draw_domain;
}

void NeighbourhoodServer::set_draw_queries_refresh_interval(float p_interval) {
	draw_queries_refresh_interval = p_interval;
}

float NeighbourhoodServer::get_draw_queries_refresh_interval() const {
	return draw_queries_refresh_interval;
}
