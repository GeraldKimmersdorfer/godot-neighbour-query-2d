extends Node2D

@export_group("Scene Controls")
@export var _grid: Grid
@export var _info_label: Label

@export_group("")
@export var dot_template: PackedScene
@export var dot_count: int = 1000

var _dots: Array[Node2D] = []
var _highlighted: Array[CanvasItem] = []
var _closest_dot: Node2D = null
var _debug_cells: bool = false
var _query_radius: float = 100.0

const _AVG_ALPHA := 0.05  # exponential moving average smoothing factor
var _avg_get_all_ms: float = 0.0
var _avg_get_next_ms: float = 0.0

@onready var _ns: NeighbourhoodServer = $NeighbourhoodServer

func _ready() -> void:
	_debug_cells = _ns.has_method("get_last_queried_cells")
	_ns.refresh_intervall = 0.5
	_ns.grid_size = 64
	_grid.grid_size = _ns.grid_size
	get_viewport().size_changed.connect(_on_viewport_size_changed)
	var bounds := _get_bounds()
	for i in dot_count:
		var dot: Node2D = dot_template.instantiate()
		dot.position = Vector2(randf_range(bounds.position.x, bounds.end.x), randf_range(bounds.position.y, bounds.end.y))
		var speed := randf_range(20.0, 120.0)
		var angle := randf() * TAU
		dot.velocity = Vector2(cos(angle), sin(angle)) * speed
		dot.bounds = bounds
		add_child(dot)
		_dots.append(dot)
		_ns.subscribe(dot, 1, dot)

func _exit_tree() -> void:
	for dot in _dots:
		_ns.unsubscribe(dot)

func _get_bounds() -> Rect2:
	var margin := float(_ns.grid_size)
	var viewport_size := get_viewport_rect().size
	return Rect2(Vector2(margin, margin), viewport_size - Vector2(margin, margin) * 2.0)

func _on_viewport_size_changed() -> void:
	var bounds := _get_bounds()
	for dot in _dots:
		dot.bounds = bounds

func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_query_radius = _query_radius + 10.0
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_query_radius = maxf(_query_radius - 10.0, 10.0)

func _process(_delta: float) -> void:
	for dot in _highlighted:
		dot.modulate = Color.WHITE
	_highlighted.clear()

	var mouse_pos := get_global_mouse_position()
	var overlays := {}

	var t := Time.get_ticks_usec()
	var neighbours := _ns.get_all(mouse_pos, _query_radius)
	_avg_get_all_ms += _AVG_ALPHA * (float(Time.get_ticks_usec() - t) / 1000.0 - _avg_get_all_ms)
	for n in neighbours:
		var dot := n as CanvasItem
		if dot:
			dot.modulate = Color(1.0, 0.0, 0.0)
			_highlighted.append(dot)
	if _debug_cells:
		for cell in _ns.get_last_queried_cells():
			overlays[cell] = Color(1.0, 0.0, 0.0, 0.2)

	if _closest_dot:
		(_closest_dot as CanvasItem).modulate = Color.WHITE
	t = Time.get_ticks_usec()
	_closest_dot = _ns.get_next(mouse_pos) as Node2D
	_avg_get_next_ms += _AVG_ALPHA * (float(Time.get_ticks_usec() - t) / 1000.0 - _avg_get_next_ms)
	if _closest_dot:
		(_closest_dot as CanvasItem).modulate = Color(0.0, 1.0, 0.0)
	if _debug_cells:
		for cell in _ns.get_last_queried_cells():
			var c := Color(0.0, 1.0, 0.0, 0.2)
			overlays[cell] = overlays[cell].blend(c) if overlays.has(cell) else c
		_grid.set_cell_overlays(overlays)

	if _info_label:
		_info_label.text = "get_all:  %.3f ms (avg)\nget_next: %.3f ms (avg)\nradius: %.0f px" % [
			_avg_get_all_ms, _avg_get_next_ms, _query_radius
		]
