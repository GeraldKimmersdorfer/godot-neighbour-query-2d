@tool
extends Node2D
class_name DotContainer

@export_group("Scene Controls")
@export var _info_label: Label
@export var _nq2d: NeighbourQuery2D
@export var _option_container: OptionContainer

enum InitPlacementMode { UNIFORM, DENSITY_TEXTURE_BASED }

@export_group("")
@export var dot_template: PackedScene
@export var dot_count: int = 1000
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

func _ready() -> void:
	if Engine.is_editor_hint(): return
	if density_texture:
		density_texture.changed.connect(reinitialize_positions, CONNECT_ONE_SHOT)
	_nq2d.debug_info.connect(_on_ns_debug_info)
	_option_container.ranges_changed.connect(_on_ranges_changed)
	_info_label.visible = false
	for i in dot_count:
		_spawn_dot()
	var bounds: Rect2 = _nq2d.domain
	for func_idx in DotQueryNode.QueryFunc.size():
		var qn := DotQueryNode.new()
		qn.query_func = func_idx as DotQueryNode.QueryFunc
		qn.query_max_range = _option_container.query_max_range
		qn.query_min_range = _option_container.query_min_range
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
	if placement_mode == InitPlacementMode.DENSITY_TEXTURE_BASED and density_texture and not _density_image:
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

func _on_ranges_changed(max_range: float, min_range: float) -> void:
	for qn in _query_nodes:
		qn.query_max_range = max_range
		qn.query_min_range = min_range

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
	dot.movement_mode = _option_container.movement_mode
	add_child(dot)
	dot.position = _get_spawn_position()
	_dots.append(dot)

func _remove_dot(dot: Node2D) -> void:
	_dots.erase(dot)
	dot.queue_free()

func update_movement_for_all(mode: Dot.MovementMode) -> void:
	for dot in _dots:
		dot.movement_mode = mode

func _validation_test() -> void:
	if _dots.is_empty() or not _option_container.validation_test_enabled:
		return
	_remove_dot(_dots[randi() % _dots.size()])
	_spawn_dot()

func _physics_process(_delta: float) -> void:
	if Engine.is_editor_hint(): return
	_validation_test()

func start_benchmark(node_count: int, p_placement_mode: InitPlacementMode, p_movement_mode: Dot.MovementMode, benchmark_time: float, query_node_speed: float = DotQueryNode.BASE_FREQ) -> String:
	for dot in _dots:
		dot.queue_free()
	_dots.clear()
	placement_mode = p_placement_mode
	for i in node_count:
		_spawn_dot()
	update_movement_for_all(p_movement_mode)
	for qn in _query_nodes:
		qn._time = DotQueryNode.LISSAJOUS_TIME_OFFSETS[qn.query_func]
		qn.speed = query_node_speed
	var original_interval: float = _nq2d.debug_report_interval
	_nq2d.debug_report_interval = 0.0
	await _wait_for_debug_report()
	_nq2d.debug_report_interval = benchmark_time
	var report := await _wait_for_debug_report()
	_nq2d.debug_report_interval = original_interval
	return report

func _wait_for_debug_report() -> String:
	var key: StringName = &""
	var report: String = ""
	while key != &"debug_report":
		var args: Array = await _nq2d.debug_info
		key = args[0]
		report = args[1]
	return report

func _draw() -> void:
	if density_texture and placement_mode == InitPlacementMode.DENSITY_TEXTURE_BASED:
		draw_texture_rect(density_texture, _nq2d.domain, false, Color(1, 1, 1, 0.2))
