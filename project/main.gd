extends Node2D

@export_group("Scene Controls")
@export var _info_label: Label

@export_group("")
@export var dot_template: PackedScene
@export var dot_count: int = 1000

enum QueryMode { GET_ALL, GET_NEXT, GET_CLOSEST }

var _dots: Array[Node2D] = []
var _highlighted: Array[CanvasItem] = []
var _mode: QueryMode = QueryMode.GET_ALL
var _query_radius: float = 100.0
var _closest_count: int = 5
var _debug_info: Dictionary = {}

const _AVG_ALPHA := 0.02 # running average smoothing factor
var _avgs: Dictionary = {
	"query_ms": 0.0,
	"refresh_total_ms": 0.0,
}

@onready var _ns: NeighbourhoodServer = $NeighbourhoodServer

func _ready() -> void:
	if _ns.has_signal("debug_info"):
		_ns.debug_info.connect(_on_ns_debug_info)
	get_viewport().size_changed.connect(_on_viewport_size_changed)
	var bounds := _get_bounds()
	for i in dot_count:
		var dot: Node2D = dot_template.instantiate()
		dot.position = Vector2(randf_range(bounds.position.x, bounds.end.x), randf_range(bounds.position.y, bounds.end.y))
		dot.bounds = bounds
		add_child(dot)
		_dots.append(dot)
		_ns.subscribe(dot, 1, dot)

func _exit_tree() -> void:
	for dot in _dots:
		_ns.unsubscribe(dot)

func _get_bounds() -> Rect2:
	var gs := float(_ns.grid_size)
	var viewport_size := get_viewport_rect().size
	# Snap to the last fully visible cell boundary, then pad by one cell on each side
	var snapped_size := Vector2(floor(viewport_size.x / gs) * gs, floor(viewport_size.y / gs) * gs)
	return Rect2(Vector2(gs, gs), snapped_size - Vector2(gs * 2.0, gs * 2.0))

func _on_viewport_size_changed() -> void:
	var bounds := _get_bounds()
	for dot in _dots:
		dot.bounds = bounds

func _on_ns_debug_info(key: String, value: Variant) -> void:
	if key in _avgs:
		_avgs[key] += _AVG_ALPHA * (float(value) - _avgs[key])
		_debug_info[key] = _avgs[key]
	else:
		_debug_info[key] = value

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed:
		if event.keycode == KEY_1:
			_mode = QueryMode.GET_ALL
		elif event.keycode == KEY_2:
			_mode = QueryMode.GET_NEXT
		elif event.keycode == KEY_3:
			_mode = QueryMode.GET_CLOSEST
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_query_radius = _query_radius + 10.0
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_query_radius = maxf(_query_radius - 10.0, 10.0)

func _spawn_dot() -> void:
	var bounds := _get_bounds()
	var dot: Node2D = dot_template.instantiate()
	dot.position = Vector2(randf_range(bounds.position.x, bounds.end.x), randf_range(bounds.position.y, bounds.end.y))
	dot.bounds = bounds
	add_child(dot)
	_dots.append(dot)
	_ns.subscribe(dot, 1, dot)

func _remove_dot(dot: Node2D, call_unsubscribe: bool) -> void:
	_dots.erase(dot)
	_highlighted.erase(dot)
	if call_unsubscribe:
		_ns.unsubscribe(dot)
	dot.queue_free()

func _stress_test() -> void:
	if _dots.is_empty():
		return
	var idx := randi() % _dots.size()
	var dot := _dots[idx]
	# Randomly decide whether to properly unsubscribe or just free (tests validity checks)
	_remove_dot(dot, randf() > 0.5)
	_spawn_dot()

func _physics_process(_delta: float) -> void:
	_stress_test()
	for dot in _highlighted:
		if is_instance_valid(dot):
			dot.modulate = Color.WHITE
	_highlighted.clear()

	var mouse_pos := get_global_mouse_position()
	var t := Time.get_ticks_usec()

	if _mode == QueryMode.GET_ALL:
		var neighbours := _ns.get_all(mouse_pos, _query_radius)
		_avgs["query_ms"] += _AVG_ALPHA * (float(Time.get_ticks_usec() - t) / 1000.0 - _avgs["query_ms"])
		for n in neighbours:
			var dot = n
			if dot:
				dot.modulate = Color(1.0, 0.0, 0.0)
				_highlighted.append(dot)
	elif _mode == QueryMode.GET_NEXT:
		var closest = _ns.get_next(mouse_pos)
		_avgs["query_ms"] += _AVG_ALPHA * (float(Time.get_ticks_usec() - t) / 1000.0 - _avgs["query_ms"])
		if closest:
			closest.modulate = Color(1.0, 0.0, 0.0)
			_highlighted.append(closest)
	else:
		var neighbours := _ns.get_closest(mouse_pos, _closest_count, _query_radius)
		_avgs["query_ms"] += _AVG_ALPHA * (float(Time.get_ticks_usec() - t) / 1000.0 - _avgs["query_ms"])
		for n in neighbours:
			var dot = n
			if dot:
				dot.modulate = Color(1.0, 0.0, 0.0)
				_highlighted.append(dot)

	if _info_label:
		var text := "mode: %s\nquery:    %.3f ms (avg)\nradius: %.0f px" % [
			QueryMode.keys()[_mode], _avgs["query_ms"], _query_radius
		]
		for key in _debug_info:
			text += "\n%s: %.3f" % [key, _debug_info[key]]
		_info_label.text = text
