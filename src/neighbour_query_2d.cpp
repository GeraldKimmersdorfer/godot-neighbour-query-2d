#include "neighbour_query_2d.h"

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
#include <random>

void NeighbourQuery2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("subscribe", "node", "layer", "moving_entity", "offset"), &NeighbourQuery2D::subscribe, DEFVAL(Variant()), DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("unsubscribe", "node"), &NeighbourQuery2D::unsubscribe);
	ClassDB::bind_method(D_METHOD("get_next", "position", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourQuery2D::get_next, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_next_first", "position", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourQuery2D::get_next_first, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_all", "position", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourQuery2D::get_all, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_closest", "position", "max_count", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourQuery2D::get_closest, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_random", "position", "max_count", "max_distance", "min_distance", "layer_mask", "exclude"), &NeighbourQuery2D::get_random, DEFVAL(std::numeric_limits<float>::max()), DEFVAL(0.0f), DEFVAL(0xFFFFFFFF), DEFVAL(Variant()));

	ClassDB::bind_method(D_METHOD("set_grid_size", "grid_size"), &NeighbourQuery2D::set_grid_size);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &NeighbourQuery2D::get_grid_size);

	ClassDB::bind_method(D_METHOD("set_refresh_intervall", "refresh_intervall"), &NeighbourQuery2D::set_refresh_intervall);
	ClassDB::bind_method(D_METHOD("get_refresh_intervall"), &NeighbourQuery2D::get_refresh_intervall);

	ClassDB::bind_method(D_METHOD("set_gc_interval", "gc_interval"), &NeighbourQuery2D::set_gc_interval);
	ClassDB::bind_method(D_METHOD("get_gc_interval"), &NeighbourQuery2D::get_gc_interval);

	ClassDB::bind_method(D_METHOD("set_use_global_position", "use_global_position"), &NeighbourQuery2D::set_use_global_position);
	ClassDB::bind_method(D_METHOD("get_use_global_position"), &NeighbourQuery2D::get_use_global_position);

	ClassDB::bind_method(D_METHOD("set_domain", "domain"), &NeighbourQuery2D::set_domain);
	ClassDB::bind_method(D_METHOD("get_domain"), &NeighbourQuery2D::get_domain);

	ClassDB::bind_method(D_METHOD("set_debug_draw_domain", "debug_draw_domain"), &NeighbourQuery2D::set_debug_draw_domain);
	ClassDB::bind_method(D_METHOD("get_debug_draw_domain"), &NeighbourQuery2D::get_debug_draw_domain);

	ClassDB::bind_method(D_METHOD("set_debug_draw_heatmap_intervall", "interval"), &NeighbourQuery2D::set_debug_draw_heatmap_intervall);
	ClassDB::bind_method(D_METHOD("get_debug_draw_heatmap_intervall"), &NeighbourQuery2D::get_debug_draw_heatmap_intervall);

	ClassDB::bind_method(D_METHOD("set_debug_heatmap_mode", "mode"), &NeighbourQuery2D::set_debug_heatmap_mode);
	ClassDB::bind_method(D_METHOD("get_debug_heatmap_mode"), &NeighbourQuery2D::get_debug_heatmap_mode);

	ClassDB::bind_method(D_METHOD("set_debug_report_interval", "interval"), &NeighbourQuery2D::set_debug_report_interval);
	ClassDB::bind_method(D_METHOD("get_debug_report_interval"), &NeighbourQuery2D::get_debug_report_interval);

	BIND_ENUM_CONSTANT(CELL_READS);
	BIND_ENUM_CONSTANT(QUERY_COUNTS);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "grid_size"), "set_grid_size", "get_grid_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refresh_intervall"), "set_refresh_intervall", "get_refresh_intervall");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gc_interval"), "set_gc_interval", "get_gc_interval");
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

void NeighbourQuery2D::_draw() {
	if (!debug_draw_domain) {
		return;
	}
	const Color border_color(1.0f, 1.0f, 1.0f, 0.8f);
	const Color fill_color(1.0f, 1.0f, 1.0f, 0.1f);
	const Color grid_color(1.0f, 1.0f, 1.0f, 0.4f);

	draw_rect(domain, fill_color, true);

	if (debug_draw_heatmap_intervall >= 0.0f) {
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

void NeighbourQuery2D::_process(double p_delta) {
	if (!debug_draw_domain || debug_draw_heatmap_intervall < 0.0f) {
		return;
	}
	m_time_since_querycount_redraw += p_delta;
	if (m_time_since_querycount_redraw >= debug_draw_heatmap_intervall) {
		m_time_since_querycount_redraw = 0.0;
		queue_redraw();
	}
}

void NeighbourQuery2D::emit_debug_report() {
	emit_signal("debug_info", StringName("debug_report"), String(m_debug_timer.create_report().c_str()));
	m_debug_timer.reset_all();
}

#endif

NeighbourQuery2D::~NeighbourQuery2D() {
	_stop_gc_thread();
}

void NeighbourQuery2D::_stop_gc_thread() {
	m_gc_stop.store(true);
	m_gc_cv.notify_all();
	if (m_gc_thread.joinable()) {
		m_gc_thread.join();
	}
}

void NeighbourQuery2D::_gc_thread_func() {
	while (!m_gc_stop.load()) {
		{
			std::unique_lock<std::mutex> lock(m_gc_cv_mutex);
			m_gc_cv.wait_for(lock, std::chrono::duration<float>(gc_interval), [this] { return m_gc_stop.load(); });
		}
		if (m_gc_stop.load()) {
			break;
		}
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (auto group_it = m_groups.begin(); group_it != m_groups.end();) {
				SubscriberGroup &group = group_it->second;
				// If the shared moving entity is gone, drop the whole group.
				if (group.moving_entity != nullptr &&
						UtilityFunctions::instance_from_id(group.moving_entity_instance_id) == nullptr) {
					for (auto &sub : group.members) {
						m_node_to_group.erase(sub.node);
					}
					group_it = m_groups.erase(group_it);
					continue;
				}
				// Sweep individual members.
				auto &members = group.members;
				for (auto it = members.begin(); it != members.end();) {
					if (UtilityFunctions::instance_from_id(it->node_instance_id) == nullptr) {
						m_node_to_group.erase(it->node);
						*it = members.back();
						members.pop_back();
					} else {
						++it;
					}
				}
				if (members.empty()) {
					group_it = m_groups.erase(group_it);
				} else {
					++group_it;
				}
			}
		}
	}
}


void NeighbourQuery2D::_ready() {
	_update_grid_dimensions();
	if (Engine::get_singleton()->is_editor_hint()) {
		set_physics_process(false);
		set_process(false);
#if DEBUG_INFORMATION
		queue_redraw();
#endif
	} else {
		set_physics_process(true);
		m_gc_stop.store(false);
		m_gc_thread = std::thread(&NeighbourQuery2D::_gc_thread_func, this);
#if DEBUG_INFORMATION
		set_process(true);
		m_debug_timer = DebugTimer(Engine::get_singleton()->get_physics_ticks_per_second());
#endif
	}
}

void NeighbourQuery2D::_physics_process(double p_delta) {

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

void NeighbourQuery2D::_update_grid_dimensions() {
	m_grid_cols = std::max(1, static_cast<int>(std::ceil(domain.size.x / grid_size)));
	m_grid_rows = std::max(1, static_cast<int>(std::ceil(domain.size.y / grid_size)));
	m_domain_center = domain.position + domain.size * 0.5f;
	m_domain_diagonal = domain.size.length();
	int cell_count = m_grid_cols * m_grid_rows;
	m_grid_build.assign(cell_count, {});
#if DEBUG_INFORMATION
	m_grid_cellreads_debug.assign(cell_count, 0);
	m_grid_querycount_debug.assign(cell_count, 0);
#endif
	m_grid.assign(cell_count, {});
}

Vector2 NeighbourQuery2D::prepare_query(const Vector2 &p_position, float &p_max_distance, float &p_min_distance) const {
	// Cap p_max_distance to the domain diagonal to prevent cell range overflow.
	p_max_distance = std::min(p_max_distance, m_domain_diagonal);

	// If position is inside the domain fine...
	if (p_position.x >= domain.position.x && p_position.x <= domain.position.x + domain.size.x &&
			p_position.y >= domain.position.y && p_position.y <= domain.position.y + domain.size.y) {
		return p_position;
	}

	// ... but if not project it to the closest point inside the domain and adapt min and max distance accordingly
	Vector2 qpos = Vector2(
			std::clamp(p_position.x, domain.position.x, domain.position.x + domain.size.x),
			std::clamp(p_position.y, domain.position.y, domain.position.y + domain.size.y));
	float outside_dist = p_position.distance_to(qpos);
	p_max_distance = std::max(0.0f, p_max_distance - outside_dist);
	p_min_distance = std::max(0.0f, p_min_distance - outside_dist);
	return qpos;
}

void NeighbourQuery2D::refresh() {
#if DEBUG_INFORMATION
	m_debug_timer.start("refresh", "refresh");
#endif

	// Clear build buffer in-place (keeps per-cell capacity to avoid repeated reallocations).
	for (auto &cell : m_grid_build) {
		cell.entries.clear();
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto &[entity_key, group] : m_groups) {
			if (entity_key == nullptr) {
				// Solo group: each member fetches its own position.
				for (auto &sub : group.members) {
					if (UtilityFunctions::instance_from_id(sub.node_instance_id) == nullptr) {
						continue;
					}
					Vector2 pos = (sub.node->*m_get_position)();
					int cx = static_cast<int>(std::floor((pos.x - domain.position.x) / grid_size));
					int cy = static_cast<int>(std::floor((pos.y - domain.position.y) / grid_size));
					if (!is_cell_in_bounds(cx, cy)) {
						continue;
					}
					m_grid_build[to_cell_index(cx, cy)].entries.push_back(GridEntry{ sub.node, sub.node_instance_id, sub.layer, pos });
				}
			} else {
				// Shared entity group: one get_position() call for all members.
				if (UtilityFunctions::instance_from_id(group.moving_entity_instance_id) == nullptr) {
					continue;
				}
				Vector2 base = (entity_key->*m_get_position)();
				for (auto &sub : group.members) {
					if (UtilityFunctions::instance_from_id(sub.node_instance_id) == nullptr) {
						continue;
					}
					Vector2 pos = base + sub.offset;
					int cx = static_cast<int>(std::floor((pos.x - domain.position.x) / grid_size));
					int cy = static_cast<int>(std::floor((pos.y - domain.position.y) / grid_size));
					if (!is_cell_in_bounds(cx, cy)) {
						continue;
					}
					m_grid_build[to_cell_index(cx, cy)].entries.push_back(GridEntry{ sub.node, sub.node_instance_id, sub.layer, pos });
				}
			}
		}
	}

	// Build per-cell AABBs for early discard in queries.
	for (auto &cell : m_grid_build) {
		if (cell.entries.empty()) {
			continue;
		}
		Vector2 mn = cell.entries[0].position;
		Vector2 mx = mn;
		for (size_t i = 1; i < cell.entries.size(); i++) {
			const Vector2 &p = cell.entries[i].position;
			if (p.x < mn.x) mn.x = p.x;
			if (p.y < mn.y) mn.y = p.y;
			if (p.x > mx.x) mx.x = p.x;
			if (p.y > mx.y) mx.y = p.y;
		}
		cell.aabb = { mn.x, mn.y, mx.x, mx.y };
	}

	std::swap(m_grid, m_grid_build);

#if DEBUG_INFORMATION
	m_debug_timer.stop("refresh", "refresh");
#endif
}

void NeighbourQuery2D::subscribe(Node2D *p_node, uint32_t p_layer, Node2D *p_moving_entity, Vector2 p_offset) {
	std::lock_guard<std::mutex> lock(m_mutex);
	// Remove from current group if re-subscribing.
	auto lookup_it = m_node_to_group.find(p_node);
	if (lookup_it != m_node_to_group.end()) {
		Node2D *old_key = lookup_it->second;
		auto group_it = m_groups.find(old_key);
		if (group_it != m_groups.end()) {
			auto &members = group_it->second.members;
			for (auto it = members.begin(); it != members.end(); ++it) {
				if (it->node == p_node) {
					*it = members.back();
					members.pop_back();
					break;
				}
			}
			if (members.empty()) {
				m_groups.erase(group_it);
			}
		}
	}
	// Insert into the new group (created on demand).
	SubscriberGroup &group = m_groups[p_moving_entity];
	if (group.moving_entity == nullptr && p_moving_entity != nullptr) {
		group.moving_entity = p_moving_entity;
		group.moving_entity_instance_id = static_cast<uint64_t>(p_moving_entity->get_instance_id());
	}
	group.members.push_back({ p_node, static_cast<uint64_t>(p_node->get_instance_id()), p_layer, p_offset });
	m_node_to_group[p_node] = p_moving_entity;
}

void NeighbourQuery2D::unsubscribe(Node2D *p_node) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto lookup_it = m_node_to_group.find(p_node);
	if (lookup_it == m_node_to_group.end()) {
		return;
	}
	Node2D *group_key = lookup_it->second;
	auto group_it = m_groups.find(group_key);
	if (group_it != m_groups.end()) {
		auto &members = group_it->second.members;
		for (auto it = members.begin(); it != members.end(); ++it) {
			if (it->node == p_node) {
				*it = members.back();
				members.pop_back();
				break;
			}
		}
		if (members.empty()) {
			m_groups.erase(group_it);
		}
	}
	m_node_to_group.erase(lookup_it);
}

Node2D *NeighbourQuery2D::get_next_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	const Vector2 qpos = prepare_query(p_position, p_max_distance, p_min_distance);
	int cx0 = static_cast<int>(std::floor((qpos.x - domain.position.x) / grid_size));
	int cy0 = static_cast<int>(std::floor((qpos.y - domain.position.y) / grid_size));
#if DEBUG_INFORMATION
	if (debug_draw_domain) {
		m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
	}
#endif

	float best_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;

	const GridEntry *best = nullptr;

	auto check = [&](int cx, int cy) {
		if (!is_cell_in_bounds(cx, cy)) {
			return;
		}
		const int cell_idx = to_cell_index(cx, cy);
		const GridCell &cell = m_grid[cell_idx];
		if (cell.early_discard_check(qpos, min_dist_sq, best_dist_sq)) {
			return;
		}
#if DEBUG_INFORMATION
		if (debug_draw_domain)
			m_grid_cellreads_debug[cell_idx]++;
#endif
		for (const GridEntry &s : cell.entries) {
			if ((s.layer & p_layer_mask) == 0) {
				continue;
			}
			if (s.node_instance_id == p_exclude_id) {
				continue;
			}
			float d = s.position.distance_squared_to(qpos);
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

		// NOTE: Instead of this loop I tried precomputed ring traversal order array that are
		// sorted by the Chabyshev distance. This adds additional storage and complexity and didnt
		// seem to be worth it.
		for (int cy = cy0 - r + 1; cy <= cy0 + r - 1; cy++) {
			check(cx0 - r, cy);
			check(cx0 + r, cy);
		}
		for (int cx = cx0 - r; cx <= cx0 + r; cx++) {
			check(cx, cy0 - r);
			check(cx, cy0 + r);
		}

	}

	return best ? best->node : nullptr;
}

Node2D *NeighbourQuery2D::get_next(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_next");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Node2D *result = get_next_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_next");
#endif
	return result;
}

Array NeighbourQuery2D::get_random_grid(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	return Array();
	const Vector2 qpos = prepare_query(p_position, p_max_distance, p_min_distance);
#if DEBUG_INFORMATION
	if (debug_draw_domain) {
		int cx0 = static_cast<int>(std::floor((qpos.x - domain.position.x) / grid_size));
		int cy0 = static_cast<int>(std::floor((qpos.y - domain.position.y) / grid_size));
		if (is_cell_in_bounds(cx0, cy0)) {
			m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
		}
	}
#endif

	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;

	auto cr = get_cell_range(qpos, p_max_distance);
	// We use a static candidates vector to avoid unnecessary reallocations/resizing in subsequent queries
	thread_local static std::vector<const GridEntry *> candidates;
	candidates.clear();
	for (int cy = cr.min_y; cy <= cr.max_y; cy++) {
		for (int cx = cr.min_x; cx <= cr.max_x; cx++) {
			const int cell_idx = to_cell_index(cx, cy);
			const GridCell &cell = m_grid[cell_idx];
			if (cell.early_discard_check(qpos, min_dist_sq, max_dist_sq)) {
				continue;
			}
#if DEBUG_INFORMATION
			if (debug_draw_domain)
				m_grid_cellreads_debug[cell_idx]++;
#endif
			for (const GridEntry &s : cell.entries) {
				if ((s.layer & p_layer_mask) == 0) {
					continue;
				}
				if (s.node_instance_id == p_exclude_id) {
					continue;
				}
				float d = s.position.distance_squared_to(qpos);
				if (d > max_dist_sq || d < min_dist_sq) {
					continue;
				}
				candidates.push_back(&s);
			}
		}
	}

	//NOTE: Partial Fisher-Yates method since we dont have to completely shuffle the candidates
	// pick a random element from the remaining unprocessed pool, verify it and shrink the pool
	thread_local static std::default_random_engine rng(std::random_device{}());
	int remaining = (int)candidates.size();
	Array result;
	result.resize(MIN(remaining, p_max_count));
	int count = 0;
	while (remaining > 0 && count < p_max_count) {
		int idx = rng() % remaining;
		const GridEntry *s = candidates[idx];
		candidates[idx] = candidates[--remaining];
		if (UtilityFunctions::instance_from_id(s->node_instance_id) == nullptr) {
			continue;
		}
		result[count++] = s->node;
	}
	result.resize(count);
	return result;
}

Array NeighbourQuery2D::get_random(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
	if (p_max_count <= 0) {
		return Array();
	}
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_random");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Array result = get_random_grid(p_position, p_max_count, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_random");
#endif
	return result;
}


Node2D *NeighbourQuery2D::get_next_first_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	const Vector2 qpos = prepare_query(p_position, p_max_distance, p_min_distance);
	int cx0 = static_cast<int>(std::floor((qpos.x - domain.position.x) / grid_size));
	int cy0 = static_cast<int>(std::floor((qpos.y - domain.position.y) / grid_size));
#if DEBUG_INFORMATION
	if (debug_draw_domain) {
		m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
	}
#endif

	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;


	// Returns the first valid subscriber in the cell, or nullptr if none.
	auto check_cell = [&](int cx, int cy) -> const GridEntry * {
		if (!is_cell_in_bounds(cx, cy)) {
			return nullptr;
		}
		const int cell_idx = to_cell_index(cx, cy);
		const GridCell &cell = m_grid[cell_idx];
		if (cell.early_discard_check(qpos, min_dist_sq, max_dist_sq)) {
			return nullptr;
		}
#if DEBUG_INFORMATION
		if (debug_draw_domain)
			m_grid_cellreads_debug[cell_idx]++;
#endif
		for (const GridEntry &s : cell.entries) {
			if ((s.layer & p_layer_mask) == 0) {
				continue;
			}
			if (s.node_instance_id == p_exclude_id) {
				continue;
			}
			float d = s.position.distance_squared_to(qpos);
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

	if (const GridEntry *s = check_cell(cx0, cy0)) {
		return s->node;
	}

	for (int r = 1;; r++) {
		if (static_cast<float>((r - 1) * grid_size) > p_max_distance) {
			break;
		}

		for (int cy = cy0 - r + 1; cy <= cy0 + r - 1; cy++) {
			if (const GridEntry *s = check_cell(cx0 - r, cy)) return s->node;
			if (const GridEntry *s = check_cell(cx0 + r, cy)) return s->node;
		}
		for (int cx = cx0 - r; cx <= cx0 + r; cx++) {
			if (const GridEntry *s = check_cell(cx, cy0 - r)) return s->node;
			if (const GridEntry *s = check_cell(cx, cy0 + r)) return s->node;
		}
	}

	return nullptr;
}

Node2D *NeighbourQuery2D::get_next_first(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_next_first");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Node2D *result = get_next_first_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_next_first");
#endif
	return result;
}

Array NeighbourQuery2D::get_all_grid(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	return Array();
	const Vector2 qpos = prepare_query(p_position, p_max_distance, p_min_distance);
#if DEBUG_INFORMATION
	if (debug_draw_domain) {
		int cx0 = static_cast<int>(std::floor((qpos.x - domain.position.x) / grid_size));
		int cy0 = static_cast<int>(std::floor((qpos.y - domain.position.y) / grid_size));
		if (is_cell_in_bounds(cx0, cy0)) {
			m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
		}
	}
#endif

	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;

	auto cr = get_cell_range(qpos, p_max_distance);
	thread_local static std::vector<const GridEntry *> candidates;
	candidates.clear();
	for (int cy = cr.min_y; cy <= cr.max_y; cy++) {
		for (int cx = cr.min_x; cx <= cr.max_x; cx++) {
			const int cell_idx = to_cell_index(cx, cy);
			const GridCell &cell = m_grid[cell_idx];
			if (cell.early_discard_check(qpos, min_dist_sq, max_dist_sq)) {
				continue;
			}
#if DEBUG_INFORMATION
			if (debug_draw_domain)
				m_grid_cellreads_debug[cell_idx]++;
#endif
			// NOTE: I already tried having template functions with static ifs to completely remove checks like validity, min distance,
			// and so on from the hot loop. It did not yield any significant change to the execution time. (same for the other get_ functions)
			for (const GridEntry &s : cell.entries) {
				if ((s.layer & p_layer_mask) == 0) {
					continue;
				}
				if (s.node_instance_id == p_exclude_id) {
					continue;
				}
				float d = s.position.distance_squared_to(qpos);
				if (d > max_dist_sq || d < min_dist_sq) {
					continue;
				}
				candidates.push_back(&s);
			}
		}
	}

	Array result;
	result.resize(candidates.size());
	int count = 0;
	for (const GridEntry *s : candidates) {
		if (UtilityFunctions::instance_from_id(s->node_instance_id) == nullptr) {
			continue;
		}
		result[count++] = s->node;
	}
	result.resize(count);
	return result;
}

Array NeighbourQuery2D::get_all(const Vector2 &p_position, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_all");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Array result = get_all_grid(p_position, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_all");
#endif
	return result;
}

Array NeighbourQuery2D::get_closest_grid(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, uint64_t p_exclude_id) {
	// NOTE: We use a maxheap keyed by squared distance such that heap[0] is always the farthest collected entry.
	// Using a heap here allows us O(logn) for insert whereas a sorted array would be O(n).
	using Entry = std::pair<float, Node2D *>;
	auto cmp = [](const Entry &a, const Entry &b) { return a.first < b.first; };
	thread_local static std::vector<Entry> heap;
	heap.clear();

	const Vector2 qpos = prepare_query(p_position, p_max_distance, p_min_distance);
	int cx0 = static_cast<int>(std::floor((qpos.x - domain.position.x) / grid_size));
	int cy0 = static_cast<int>(std::floor((qpos.y - domain.position.y) / grid_size));
#if DEBUG_INFORMATION
	if (debug_draw_domain) {
		m_grid_querycount_debug[to_cell_index(cx0, cy0)]++;
	}
#endif

	float max_dist_sq = p_max_distance * p_max_distance;
	float min_dist_sq = p_min_distance * p_min_distance;


	auto check = [&](int cx, int cy) {
		if (!is_cell_in_bounds(cx, cy)) {
			return;
		}
		const int cell_idx = to_cell_index(cx, cy);
		const GridCell &cell = m_grid[cell_idx];
		if (cell.early_discard_check(qpos, min_dist_sq, max_dist_sq)) {
			return;
		}
#if DEBUG_INFORMATION
		if (debug_draw_domain)
			m_grid_cellreads_debug[cell_idx]++;
#endif
		for (const GridEntry &s : cell.entries) {
			if ((s.layer & p_layer_mask) == 0) {
				continue;
			}
			if (s.node_instance_id == p_exclude_id) {
				continue;
			}
			float d = s.position.distance_squared_to(qpos);
			if (d > max_dist_sq || d < min_dist_sq) {
				continue;
			}
			if ((int)heap.size() >= p_max_count && d >= heap[0].first) {
				continue;
			}
			if (UtilityFunctions::instance_from_id(s.node_instance_id) == nullptr) {
				continue;
			}
			heap.push_back({ d, s.node });
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

		for (int cy = cy0 - r + 1; cy <= cy0 + r - 1; cy++) {
			check(cx0 - r, cy);
			check(cx0 + r, cy);
		}
		for (int cx = cx0 - r; cx <= cx0 + r; cx++) {
			check(cx, cy0 - r);
			check(cx, cy0 + r);
		}
	}

	Array result;
	for (const auto &[d, node] : heap) {
		result.push_back(node);
	}
	return result;
}

Array NeighbourQuery2D::get_closest(const Vector2 &p_position, int p_max_count, float p_max_distance, float p_min_distance, uint32_t p_layer_mask, Node2D *p_exclude) {
	if (p_max_count <= 0) {
		return Array();
	}
#if DEBUG_INFORMATION
	m_debug_timer.start("query", "get_closest");
#endif
	const uint64_t exclude_id = p_exclude ? static_cast<uint64_t>(p_exclude->get_instance_id()) : 0;
	Array result = get_closest_grid(p_position, p_max_count, p_max_distance, p_min_distance, p_layer_mask, exclude_id);
#if DEBUG_INFORMATION
	m_debug_timer.stop("query", "get_closest");
#endif
	return result;
}

void NeighbourQuery2D::set_grid_size(int p_grid_size) {
	grid_size = p_grid_size;
	_update_grid_dimensions();
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
}

int NeighbourQuery2D::get_grid_size() const {
	return grid_size;
}

void NeighbourQuery2D::set_refresh_intervall(float p_refresh_intervall) {
	refresh_intervall = p_refresh_intervall;
}

float NeighbourQuery2D::get_refresh_intervall() const {
	return refresh_intervall;
}

void NeighbourQuery2D::set_gc_interval(float p_gc_interval) {
	gc_interval = p_gc_interval;
}

float NeighbourQuery2D::get_gc_interval() const {
	return gc_interval;
}

void NeighbourQuery2D::set_use_global_position(bool p_use_global_position) {
	use_global_position = p_use_global_position;
	m_get_position = use_global_position ? &Node2D::get_global_position : &Node2D::get_position;
}

bool NeighbourQuery2D::get_use_global_position() const {
	return use_global_position;
}

void NeighbourQuery2D::set_domain(const Rect2 &p_domain) {
	domain = p_domain;
	_update_grid_dimensions();
	if (Engine::get_singleton()->is_editor_hint()) {
		queue_redraw();
	}
}

Rect2 NeighbourQuery2D::get_domain() const {
	return domain;
}

void NeighbourQuery2D::set_debug_draw_domain(bool p_debug_draw_domain) {
	debug_draw_domain = p_debug_draw_domain;
	queue_redraw();
}

bool NeighbourQuery2D::get_debug_draw_domain() const {
	return debug_draw_domain;
}

void NeighbourQuery2D::set_debug_draw_heatmap_intervall(float p_interval) {
	debug_draw_heatmap_intervall = p_interval;
}

float NeighbourQuery2D::get_debug_draw_heatmap_intervall() const {
	return debug_draw_heatmap_intervall;
}

void NeighbourQuery2D::set_debug_heatmap_mode(DebugHeatmapMode p_mode) {
	debug_heatmap_mode = p_mode;
}

NeighbourQuery2D::DebugHeatmapMode NeighbourQuery2D::get_debug_heatmap_mode() const {
	return debug_heatmap_mode;
}

void NeighbourQuery2D::set_debug_report_interval(float p_interval) {
	debug_report_interval = p_interval;
}

float NeighbourQuery2D::get_debug_report_interval() const {
	return debug_report_interval;
}
