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
	ClassDB::bind_method(D_METHOD("get_next_random", "position", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourhoodServer::get_next_random, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_next_first", "position", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourhoodServer::get_next_first, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));
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

	ClassDB::bind_method(D_METHOD("set_debug_draw_domain", "debug_draw_domain"), &NeighbourhoodServer::set_debug_draw_domain);
	ClassDB::bind_method(D_METHOD("get_debug_draw_domain"), &NeighbourhoodServer::get_debug_draw_domain);

	ClassDB::bind_method(D_METHOD("set_debug_draw_heatmap_intervall", "interval"), &NeighbourhoodServer::set_debug_draw_heatmap_intervall);
	ClassDB::bind_method(D_METHOD("get_debug_draw_heatmap_intervall"), &NeighbourhoodServer::get_debug_draw_heatmap_intervall);

	ClassDB::bind_method(D_METHOD("set_debug_heatmap_mode", "mode"), &NeighbourhoodServer::set_debug_heatmap_mode);
	ClassDB::bind_method(D_METHOD("get_debug_heatmap_mode"), &NeighbourhoodServer::get_debug_heatmap_mode);

	ClassDB::bind_method(D_METHOD("set_debug_report_interval", "interval"), &NeighbourhoodServer::set_debug_report_interval);
	ClassDB::bind_method(D_METHOD("get_debug_report_interval"), &NeighbourhoodServer::get_debug_report_interval);

	BIND_ENUM_CONSTANT(CELL_READS);
	BIND_ENUM_CONSTANT(QUERY_COUNTS);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_size"), "set_grid_size", "get_grid_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_intervall"), "set_refresh_intervall", "get_refresh_intervall");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_global_position"), "set_use_global_position", "get_use_global_position");
	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "domain"), "set_domain", "get_domain");

	ADD_GROUP("Debug", "debug_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_draw_domain"), "set_debug_draw_domain", "get_debug_draw_domain");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "debug_draw_heatmap_intervall"), "set_debug_draw_heatmap_intervall", "get_debug_draw_heatmap_intervall");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "debug_heatmap_mode", PROPERTY_HINT_ENUM, "CellReads,QueryCounts"), "set_debug_heatmap_mode", "get_debug_heatmap_mode");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "debug_report_interval"), "set_debug_report_interval", "get_debug_report_interval");
	ADD_GROUP("", "");

	ADD_SIGNAL(MethodInfo("debug_info",
			PropertyInfo(Variant::STRING_NAME, "name"),
			PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NIL_IS_VARIANT)));
}

#if DEBUG_INFORMATION

void NeighbourhoodServer::_draw() {
	if (!debug_draw_domain) {
		return;
	}
	const Color border_color(1.0f, 1.0f, 1.0f, 0.8f);
	const Color fill_color(1.0f, 1.0f, 1.0f, 0.1f);
	const Color grid_color(1.0f, 1.0f, 1.0f, 0.4f);

	draw_rect(domain, fill_color, true);

	{
		const std::vector<int> &counts = (debug_heatmap_mode == QUERY_COUNTS) ? m_grid_querycount_debug : m_grid_cellreads_debug;
		int max_count = 0;
		for (int c : counts) {
			max_count = std::max(max_count, c);
		}
		if (max_count > 0) {
			Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
			for (int cy = 0; cy < m_grid_rows; cy++) {
				for (int cx = 0; cx < m_grid_cols; cx++) {
					int count = counts[to_cell_index(cx, cy)];
					if (count == 0) {
						continue;
					}
					float t = static_cast<float>(count) / max_count;
					// from ColorBrewer: https://colorbrewer2.org/#type=sequential&scheme=Blues&n=9
					const Color low(0xf7 / 255.0f, 0xfb / 255.0f, 0xff / 255.0f, 0.5f);
					const Color high(0x08 / 255.0f, 0x30 / 255.0f, 0x6b / 255.0f, 0.5f);
					Vector2 cell_pos(domain.position.x + cx * grid_size, domain.position.y + cy * grid_size);
					draw_rect(Rect2(cell_pos, Vector2(grid_size, grid_size)), low.lerp(high, t), true);
					draw_string(font, cell_pos + Vector2(0, grid_size * 0.5f), String::num_int64(count), HORIZONTAL_ALIGNMENT_CENTER, grid_size, 32, Color(1, 1, 1, 1));
				}
			}
			std::fill(m_grid_cellreads_debug.begin(), m_grid_cellreads_debug.end(), 0);
			std::fill(m_grid_querycount_debug.begin(), m_grid_querycount_debug.end(), 0);
		}
	}

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

void NeighbourhoodServer::_process(double p_delta) {
	if (!debug_draw_domain || debug_draw_heatmap_intervall < 0.0f) {
		return;
	}
	m_time_since_querycount_redraw += p_delta;
	if (m_time_since_querycount_redraw >= debug_draw_heatmap_intervall) {
		m_time_since_querycount_redraw = 0.0;
		queue_redraw();
	}
}

void NeighbourhoodServer::emit_debug_report() {
	auto callback = [this](const std::string &key, const std::string &report) {
		emit_signal("debug_info", StringName(String(key.c_str()) + "_timing"), String(report.c_str()));
	};
	m_debug_timer.report_group("refresh", callback, false);
	m_debug_timer.report_group("query", callback, true);
	m_debug_timer.report_total("total", callback);
	m_debug_timer.reset_all();
}

#endif

void NeighbourhoodServer::_ready() {
	_update_grid_dimensions();
	if (Engine::get_singleton()->is_editor_hint()) {
		set_physics_process(false);
		set_process(false);
#if DEBUG_INFORMATION
		queue_redraw();
#endif
	} else {
		set_physics_process(true);
#if DEBUG_INFORMATION
		set_process(true);
		m_debug_timer = DebugTimer(Engine::get_singleton()->get_physics_ticks_per_second());
#endif
	}
}

void NeighbourhoodServer::_physics_process(double p_delta) {
#if DEBUG_INFORMATION
	if (debug_report_interval >= 0.0f) {
		m_time_since_debug_report += p_delta;
		if (m_time_since_debug_report >= debug_report_interval) {
			m_time_since_debug_report = 0.0;
			emit_debug_report();
		}
	}
#endif

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
	m_grid_cellreads_debug.assign(cell_count, 0);
	m_grid_querycount_debug.assign(cell_count, 0);
#endif
	m_grid.assign(cell_count, {});
}

void NeighbourhoodServer::refresh() {
#if DEBUG_INFORMATION
	m_debug_timer.start("refresh", "refresh");
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

	std::swap(m_grid, m_grid_build);

#if DEBUG_INFORMATION
	m_debug_timer.stop("refresh", "refresh");
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
		if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
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
		if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
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
		if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
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
	int cx0 = static_cast<int>(std::floor((p_position.x - domain.position.x) / grid_size));
	int cy0 = static_cast<int>(std::floor((p_position.y - domain.position.y) / grid_size));
#if DEBUG_INFORMATION
	if (is_cell_in_bounds(cx0, cy0)) {
		m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
	}
#endif

	// NOTE: Cap max_distance to guarantee loop termination.
	// dist-to-center + diagonal is a safe upper bound for the farthest reachable point in the domain.
	float upper_bound = p_position.distance_to(m_domain_center) + m_domain_diagonal_half;
	if (p_max_distance > upper_bound) {
		p_max_distance = upper_bound;
	}

	float best_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;

	const Subscriber *best = nullptr;

	auto check = [&](int cx, int cy) {
		if (!is_cell_in_bounds(cx, cy)) {
			return;
		}
		const int cell_idx = to_cell_index(cx, cy);
#if DEBUG_INFORMATION
		m_grid_cellreads_debug[cell_idx]++;
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
			if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
				continue;
			}
			best_dist_sq = d;
			best = &s;
		}
	};

	check(cx0, cy0);

	for (int r = 1;; r++) {
		// Lower bound on world-space distance to any cell in ring r is (r-1)*grid_size.
		// If that already exceeds best found, no closer result can exist.
		// In other words: If point is in r we need to check r+1 too since it depends on where
		// the dot sits inside the cells r whether there can be a better result in r+1
		float min_ring_dist = static_cast<float>((r - 1) * grid_size);
		if (min_ring_dist * min_ring_dist >= best_dist_sq) {
			break;
		}

		// Also stop if the ring is entirely beyond p_max_distance.
		if (min_ring_dist > p_max_distance) {
			break;
		}

		for (int cx = cx0 - r; cx <= cx0 + r; cx++) {
			check(cx, cy0 - r);
			check(cx, cy0 + r);
		}
		for (int cy = cy0 - r + 1; cy <= cy0 + r - 1; cy++) {
			check(cx0 - r, cy);
			check(cx0 + r, cy);
		}
	}

	return best ? best->data : Variant();
}

Variant NeighbourhoodServer::get_next(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_next");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Variant result = ((int)m_subscribers.size() < m_brute_force_threshold)
			? get_next_brute_force(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id)
			: get_next_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_next");
#endif
	return result;
}

Variant NeighbourhoodServer::get_next_random(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_next_random");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Array all = ((int)m_subscribers.size() < m_brute_force_threshold)
			? get_all_brute_force(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id)
			: get_all_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
	Variant result = all.is_empty() ? Variant() : all[UtilityFunctions::randi() % all.size()];
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_next_random");
#endif
	return result;
}

Variant NeighbourhoodServer::get_next_first_brute_force(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;

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
		if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
			continue;
		}
		return s.data;
	}
	return Variant();
}

Variant NeighbourhoodServer::get_next_first_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	int cx0 = static_cast<int>(std::floor((p_position.x - domain.position.x) / grid_size));
	int cy0 = static_cast<int>(std::floor((p_position.y - domain.position.y) / grid_size));
#if DEBUG_INFORMATION
	if (is_cell_in_bounds(cx0, cy0)) {
		m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
	}
#endif

	float upper_bound = p_position.distance_to(m_domain_center) + m_domain_diagonal_half;
	if (p_max_distance > upper_bound) {
		p_max_distance = upper_bound;
	}

	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;


	// Returns the first valid subscriber in the cell, or nullptr if none.
	auto check_cell = [&](int cx, int cy) -> const Subscriber * {
		if (!is_cell_in_bounds(cx, cy)) {
			return nullptr;
		}
		const int cell_idx = to_cell_index(cx, cy);
#if DEBUG_INFORMATION
		m_grid_cellreads_debug[cell_idx]++;
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
			if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
				continue;
			}
			return &s;
		}
		return nullptr;
	};

	if (const Subscriber *s = check_cell(cx0, cy0)) {
		return s->data;
	}

	for (int r = 1;; r++) {
		if (static_cast<float>((r - 1) * grid_size) > p_max_distance) {
			break;
		}

		for (int cx = cx0 - r; cx <= cx0 + r; cx++) {
			if (const Subscriber *s = check_cell(cx, cy0 - r)) return s->data;
			if (const Subscriber *s = check_cell(cx, cy0 + r)) return s->data;
		}
		for (int cy = cy0 - r + 1; cy <= cy0 + r - 1; cy++) {
			if (const Subscriber *s = check_cell(cx0 - r, cy)) return s->data;
			if (const Subscriber *s = check_cell(cx0 + r, cy)) return s->data;
		}
	}

	return Variant();
}

Variant NeighbourhoodServer::get_next_first(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_next_first");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Variant result = ((int)m_subscribers.size() < m_brute_force_threshold)
			? get_next_first_brute_force(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id)
			: get_next_first_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_next_first");
#endif
	return result;
}

Array NeighbourhoodServer::get_all_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	Array result;

#if DEBUG_INFORMATION
	{
		int cx0 = static_cast<int>(std::floor((p_position.x - domain.position.x) / grid_size));
		int cy0 = static_cast<int>(std::floor((p_position.y - domain.position.y) / grid_size));
		if (is_cell_in_bounds(cx0, cy0)) {
			m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
		}
	}
#endif

	// Cap to domain bounds to avoid overflow in cell range computation when p_max_distance is very large.
	float upper_bound = p_position.distance_to(m_domain_center) + m_domain_diagonal_half;
	float range = std::min(p_max_distance, upper_bound);
	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;


	int min_cx = std::max(0, static_cast<int>(std::floor((p_position.x - range - domain.position.x) / grid_size)));
	int max_cx = std::min(m_grid_cols - 1, static_cast<int>(std::floor((p_position.x + range - domain.position.x) / grid_size)));
	int min_cy = std::max(0, static_cast<int>(std::floor((p_position.y - range - domain.position.y) / grid_size)));
	int max_cy = std::min(m_grid_rows - 1, static_cast<int>(std::floor((p_position.y + range - domain.position.y) / grid_size)));

	for (int cy = min_cy; cy <= max_cy; cy++) {
		for (int cx = min_cx; cx <= max_cx; cx++) {
			const int cell_idx = to_cell_index(cx, cy);
#if DEBUG_INFORMATION
			m_grid_cellreads_debug[cell_idx]++;
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
				if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
					continue;
				}
				result.push_back(s.data);
			}
		}
	}

	return result;
}

Array NeighbourhoodServer::get_all(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_all");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Array result = ((int)m_subscribers.size() < m_brute_force_threshold)
			? get_all_brute_force(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id)
			: get_all_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_all");
#endif
	return result;
}

Array NeighbourhoodServer::get_closest_grid(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	// NOTE: We use a maxheap keyed by squared distance such that heap[0] is always the farthest collected entry.
	// Using a heap here allows us O(logn) for insert whereas a sorted array would be O(n).
	using Entry = std::pair<float, Variant>;
	auto cmp = [](const Entry &a, const Entry &b) { return a.first < b.first; };
	std::vector<Entry> heap;
	heap.reserve(p_max_count + 1);

	int cx0 = static_cast<int>(std::floor((p_position.x - domain.position.x) / grid_size));
	int cy0 = static_cast<int>(std::floor((p_position.y - domain.position.y) / grid_size));
#if DEBUG_INFORMATION
	if (is_cell_in_bounds(cx0, cy0)) {
		m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
	}
#endif

	float upper_bound = p_position.distance_to(m_domain_center) + m_domain_diagonal_half;
	if (p_max_distance > upper_bound) {
		p_max_distance = upper_bound;
	}
	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;


	auto check = [&](int cx, int cy) {
		if (!is_cell_in_bounds(cx, cy)) {
			return;
		}
		const int cell_idx = to_cell_index(cx, cy);
#if DEBUG_INFORMATION
		m_grid_cellreads_debug[cell_idx]++;
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
			if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
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

	check(cx0, cy0);

	for (int r = 1;; r++) {
		float min_ring_dist = static_cast<float>((r - 1) * grid_size);
		if (min_ring_dist * min_ring_dist >= max_dist_sq) {
			break;
		}

		for (int cx = cx0 - r; cx <= cx0 + r; cx++) {
			check(cx, cy0 - r);
			check(cx, cy0 + r);
		}
		for (int cy = cy0 - r + 1; cy <= cy0 + r - 1; cy++) {
			check(cx0 - r, cy);
			check(cx0 + r, cy);
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
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_closest");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Array result = ((int)m_subscribers.size() < m_brute_force_threshold)
			? get_closest_brute_force(p_position, p_max_count, p_max_distance, p_min_distance, p_layer_mask, exclude_id)
			: get_closest_grid(p_position, p_max_count, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_closest");
#endif
	return result;
}

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

void NeighbourhoodServer::set_debug_draw_domain(bool p_debug_draw_domain) {
	debug_draw_domain = p_debug_draw_domain;
	queue_redraw();
}

bool NeighbourhoodServer::get_debug_draw_domain() const {
	return debug_draw_domain;
}

void NeighbourhoodServer::set_debug_draw_heatmap_intervall(float p_interval) {
	debug_draw_heatmap_intervall = p_interval;
}

float NeighbourhoodServer::get_debug_draw_heatmap_intervall() const {
	return debug_draw_heatmap_intervall;
}

void NeighbourhoodServer::set_debug_heatmap_mode(DebugHeatmapMode p_mode) {
	debug_heatmap_mode = p_mode;
}

NeighbourhoodServer::DebugHeatmapMode NeighbourhoodServer::get_debug_heatmap_mode() const {
	return debug_heatmap_mode;
}

void NeighbourhoodServer::set_debug_report_interval(float p_interval) {
	debug_report_interval = p_interval;
}

float NeighbourhoodServer::get_debug_report_interval() const {
	return debug_report_interval;
}
