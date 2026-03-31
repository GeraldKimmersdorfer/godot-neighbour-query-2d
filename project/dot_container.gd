extends Node2D

@export_group("Scene Controls")
@export var _info_label: Label
@export var _nq2d: NeighbourQuery2D
@export var _option_container: OptionContainer

@export_group("")
@export var dot_template: PackedScene
@export var dot_count: int = 1000

var _dots: Array[Node2D] = []
var _query_nodes: Array[DotQueryNode] = []
var _debug_info: Dictionary = {}

func _ready() -> void:
	_nq2d.debug_info.connect(_on_ns_debug_info)
	_option_container.ranges_changed.connect(_on_ranges_changed)
	_info_label.visible = false
	for i in dot_count:
		_spawn_dot()
	var bounds: Rect2 = _nq2d.domain
	var center := bounds.get_center()
	for func_idx in DotQueryNode.QueryFunc.size():
		var qn := DotQueryNode.new()
		qn.query_func = func_idx as DotQueryNode.QueryFunc
		qn.query_max_range = _option_container.query_max_range
		qn.query_min_range = _option_container.query_min_range
		qn.bounds = bounds
		qn.nq2d = _nq2d
		qn.position = center
		add_child(qn)
		_query_nodes.append(qn)

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
	_validation_test()
