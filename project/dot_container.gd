@tool
extends Node2D
class_name DotContainer

@export_group("Scene Controls")
@export var _info_label: Label
@export var _nq2d: NeighbourQuery2D

enum InitPlacementMode { UNIFORM, DENSITY }

@export_group("")
@export var dot_template: PackedScene
@export var dot_count: int = 1000:
	set(value):
		var prev_count := _dots.size()
		dot_count = maxi(0, value)
		if not is_inside_tree() or Engine.is_editor_hint(): return
		var diff := dot_count - prev_count
		if diff > 0:
			for i in diff: _spawn_dot()
		elif diff < 0:
			for i in -diff:
				if _dots.is_empty(): break
				_dots.pop_back().queue_free()

@export var placement_mode: InitPlacementMode = InitPlacementMode.UNIFORM:
	set(value):
		placement_mode = value
		queue_redraw()

@export var density_texture: Texture2D:
	set(value):
		density_texture = value
		_density_image = null
		queue_redraw()

var _dots: Array[Node2D] = []
var _density_image: Image
var _query_nodes: Array[DotQueryNode] = []
var _debug_info: Dictionary = {}
var _movement_mode: Dot.MovementMode = Dot.MovementMode.NONE

func _ready() -> void:
	if Engine.is_editor_hint(): return
	if density_texture:
		density_texture.changed.connect(reinitialize_positions, CONNECT_ONE_SHOT)
	_nq2d.debug_info.connect(_on_ns_debug_info)
	_info_label.visible = false
	for i in dot_count:
		_spawn_dot()
	var bounds: Rect2 = _nq2d.domain
	for func_idx in DotQueryNode.QueryFunc.size():
		var qn := DotQueryNode.new()
		qn.query_func = func_idx as DotQueryNode.QueryFunc
		qn.bounds = bounds
		qn.nq2d = _nq2d
		qn.position = bounds.get_center()
		add_child(qn)
		_query_nodes.append(qn)

func reinitialize_positions() -> void:
	_density_image = null
	for dot in _dots:
		dot.position = _get_spawn_position()

func _get_spawn_position() -> Vector2:
	var bounds: Rect2 = _nq2d.domain
	if placement_mode == InitPlacementMode.DENSITY and density_texture and not _density_image:
		_density_image = density_texture.get_image()
	if placement_mode == InitPlacementMode.UNIFORM or not _density_image:
		return Vector2(randf_range(bounds.position.x, bounds.end.x), randf_range(bounds.position.y, bounds.end.y))
	for _i in 100:
		var pos := Vector2(randf_range(bounds.position.x, bounds.end.x), randf_range(bounds.position.y, bounds.end.y))
		var uv := (pos - bounds.position) / bounds.size
		var px := _density_image.get_pixel(int(uv.x * (_density_image.get_width() - 1)), int(uv.y * (_density_image.get_height() - 1)))
		if randf() < px.r:
			return pos
	return Vector2(randf_range(bounds.position.x, bounds.end.x), randf_range(bounds.position.y, bounds.end.y))

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
	dot.display_mode = Dot.DisplayMode.INACTIVE if randf() > 0.5 else Dot.DisplayMode.NORMAL
	dot.nq2d = _nq2d
	dot.bounds = _nq2d.domain
	dot.movement_mode = _movement_mode
	add_child(dot)
	dot.position = _get_spawn_position()
	_dots.append(dot)

## Clears all dots and re-spawns count new ones with a fixed seed for reproducibility.
func recreate_dots(count: int, p_placement_mode: InitPlacementMode) -> void:
	for dot in _dots:
		dot.queue_free()
	_dots.clear()
	_density_image = null
	placement_mode = p_placement_mode
	seed(0)
	for i in count:
		_spawn_dot()
	dot_count = count  # sync exported var; setter is a no-op since _dots.size() == count

## Resets all query node Lissajous time offsets and speed.
func reset_query_node_positions(speed: float = DotQueryNode.BASE_FREQ) -> void:
	for qn in _query_nodes:
		qn.speed = speed
		qn._reset_phase(DotQueryNode.BASE_FREQ)

func update_movement_for_all(mode: Dot.MovementMode) -> void:
	_movement_mode = mode
	for dot in _dots:
		dot.movement_mode = mode

func set_query_range(max_range: float, min_range: float) -> void:
	for qn in _query_nodes:
		qn.query_max_range = max_range
		qn.query_min_range = min_range

func set_query_speed(speed: float) -> void:
	for qn in _query_nodes:
		qn.speed = speed

func wait_for_debug_report() -> String:
	var key: StringName = &""
	var report: String = ""
	while key != &"debug_report":
		var args: Array = await _nq2d.debug_info
		key = args[0]
		report = args[1]
	return report

func _physics_process(_delta: float) -> void:
	if Engine.is_editor_hint(): return

func _draw() -> void:
	if density_texture and placement_mode == InitPlacementMode.DENSITY:
		draw_texture_rect(density_texture, _nq2d.domain, false, Color(1, 1, 1, 0.2))
