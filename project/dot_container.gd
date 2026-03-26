extends Node2D

@export_group("Scene Controls")
@export var _info_label: Label
@export var _nq2d: NeighbourQuery2D
@export var _option_container: OptionContainer

@export_group("")
@export var dot_template: PackedScene
@export var dot_count: int = 1000

var _dots: Array[Node2D] = []
var _highlighted: Array[CanvasItem] = []
var _closest_count: int = 5
var _debug_info: Dictionary = {}

func _ready() -> void:
	_nq2d.debug_info.connect(_on_ns_debug_info)
	get_viewport().size_changed.connect(_on_viewport_size_changed)
	var bounds := _get_bounds()
	_info_label.visible = false
	for i in dot_count:
		var dot: Node2D = dot_template.instantiate()
		dot.inactive = i % 2 == 0
		dot.nq2d = _nq2d
		dot.bounds = bounds
		add_child(dot)
		_dots.append(dot)

func _get_bounds() -> Rect2:
	var gs := float(_nq2d.grid_size)
	var viewport_size := get_viewport_rect().size
	# Snap to the last fully visible cell boundary, then pad by one cell on each side
	var snapped_size := Vector2(floor(viewport_size.x / gs) * gs, floor(viewport_size.y / gs) * gs)
	return Rect2(Vector2(gs, gs), snapped_size - Vector2(gs * 2.0, gs * 2.0))

func _on_viewport_size_changed() -> void:
	var bounds := _get_bounds()
	for dot in _dots:
		dot.bounds = bounds

func _on_ns_debug_info(key: StringName, value: Variant) -> void:
	_debug_info[key] = value
	var text := ""
	for k in _debug_info:
		if k != "debug_report":
			text += "%s: %s\n" % [k, str(_debug_info[k])]
		else:
			text += "%s\n" % str(_debug_info[k])
	_info_label.text = text.strip_edges()
	_info_label.visible = true

func _spawn_dot() -> void:
	var dot: Node2D = dot_template.instantiate()
	dot.inactive = randf() > 0.5
	dot.nq2d = _nq2d
	dot.bounds = _get_bounds()
	add_child(dot)
	_dots.append(dot)

func _remove_dot(dot: Node2D) -> void:
	_dots.erase(dot)
	_highlighted.erase(dot)
	dot.queue_free()

func _stress_test() -> void:
	if _dots.is_empty():
		return
	_remove_dot(_dots[randi() % _dots.size()])
	_spawn_dot()

func _physics_process(_delta: float) -> void:
	_stress_test()
	for dot in _highlighted:
		if is_instance_valid(dot):
			dot.modulate = Color.WHITE
	_highlighted.clear()

	var mouse_pos := get_global_mouse_position()
	var neighbours = []
	var mode := _option_container.mode
	var max_range := _option_container.query_max_range
	var min_range := _option_container.query_min_range

	if mode == OptionContainer.QueryMode.GET_ALL:
		neighbours = _nq2d.get_all(mouse_pos, max_range, min_range, Dot.LAYER_ACTIVE)
	elif mode == OptionContainer.QueryMode.GET_NEXT:
		neighbours = [_nq2d.get_next(mouse_pos, max_range, min_range, Dot.LAYER_ACTIVE)]
	elif mode == OptionContainer.QueryMode.GET_CLOSEST:
		neighbours = _nq2d.get_closest(mouse_pos, _closest_count, max_range, min_range, Dot.LAYER_ACTIVE)
	elif mode == OptionContainer.QueryMode.GET_RANDOM:
		neighbours = _nq2d.get_random(mouse_pos, _closest_count, max_range, min_range, Dot.LAYER_ACTIVE)
	else:
		neighbours = [_nq2d.get_next_first(mouse_pos, max_range, min_range, Dot.LAYER_ACTIVE)]

	for dot in neighbours:
		if dot:
			dot.modulate = Color(1.0, 0.0, 0.0)
			_highlighted.append(dot)

	queue_redraw()

func _draw() -> void:
	var mouse_pos := get_global_mouse_position()
	var max_range := _option_container.query_max_range
	var min_range := _option_container.query_min_range
	draw_arc(mouse_pos, max_range, 0.0, TAU, 64, Color(1.0, 0.0, 0.0, 1.0), 2)
	if min_range > 0.0:
		draw_arc(mouse_pos, min_range, 0.0, TAU, 64, Color(1.0, 0.0, 0.0, 0.7), 2)
