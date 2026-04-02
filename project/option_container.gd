extends PanelContainer

class_name OptionContainer

@export_group("Scene Controls")
@export var _fps_label: Label
@export var _dots_label: Label
@export var _init_placement_label: Label
@export var _move_mode_label: Label
@export var _query_range_label: Label
@export var _query_ms_label: Label
@export var _dot_container: DotContainer
@export var _nq2d: NeighbourQuery2D

@export_group("")

var query_ms: float = 0.5:
	set(value):
		query_ms = value
		_query_ms_label.text = "%0.1f rad/s" % query_ms
		_dot_container.set_query_speed(query_ms)

var query_max_range: float = 150.0:
	set(value):
		query_max_range = value
		_update_range_label()
		_dot_container.set_query_range(query_max_range, query_min_range)

var query_min_range: float = 70.0:
	set(value):
		query_min_range = value
		_update_range_label()
		_dot_container.set_query_range(query_max_range, query_min_range)

var movement_mode: Dot.MovementMode = Dot.MovementMode.NONE:
	set(val):
		if val != movement_mode:
			movement_mode = val
			_move_mode_label.text = Dot.MovementMode.keys()[val]
			_dot_container.update_movement_for_all(val)

var placement_mode: DotContainer.InitPlacementMode = DotContainer.InitPlacementMode.UNIFORM:
	set(val):
		placement_mode = val
		_init_placement_label.text = DotContainer.InitPlacementMode.keys()[val]
		_dot_container.recreate_dots(_dot_container.dot_count, val)
		_dots_label.text = str(_dot_container.dot_count)

func _ready() -> void:
	_move_mode_label.text = Dot.MovementMode.keys()[movement_mode]
	_init_placement_label.text = DotContainer.InitPlacementMode.keys()[placement_mode]
	_update_range_label()
	_query_ms_label.text = "%0.1f rad/s" % query_ms
	_dots_label.text = str(_dot_container.dot_count)

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed:
		if event.keycode == KEY_M:
			movement_mode = (movement_mode + 1) % Dot.MovementMode.size() as Dot.MovementMode
		elif event.keycode == KEY_I:
			placement_mode = (placement_mode + 1) % DotContainer.InitPlacementMode.size() as DotContainer.InitPlacementMode
		elif event.keycode == KEY_B:
			run_benchmark()
		elif event.keycode == KEY_EQUAL:  # "+" key
			_dot_container.dot_count += 500
			_dots_label.text = str(_dot_container.dot_count)
		elif event.keycode == KEY_MINUS:
			_dot_container.dot_count -= 500
			_dots_label.text = str(_dot_container.dot_count)
	if event is InputEventMouseButton:
		if event.ctrl_pressed:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				query_min_range = minf(query_min_range + 5.0, query_max_range)
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				query_min_range = maxf(query_min_range - 5.0, 0.0)
		elif event.alt_pressed:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				query_ms = minf(query_ms + 0.1, 50.0)
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				query_ms = maxf(query_ms - 0.1, 0.0)
		else:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				query_max_range = maxf(query_max_range + 5.0, query_min_range)
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				query_max_range = maxf(query_max_range - 5.0, query_min_range + 10.0)

func run_benchmark() -> void:
	await Benchmark.run_benchmark(self)

func get_state() -> Dictionary:
	return {
		"count": _dot_container.dot_count,
		"placement": _dot_container.placement_mode,
		"movement": movement_mode,
	}

func restore_state(state: Dictionary) -> void:
	movement_mode = state["movement"]
	_dot_container.recreate_dots(state["count"], state["placement"])
	_dot_container.reset_query_node_positions()
	_dots_label.text = str(_dot_container.dot_count)

func run_single_benchmark(count: int, p_placement_mode: DotContainer.InitPlacementMode, p_movement_mode: Dot.MovementMode, benchmark_time: float, query_node_speed: float = DotQueryNode.BASE_FREQ) -> String:
	_dot_container.recreate_dots(count, p_placement_mode)
	_dot_container.update_movement_for_all(p_movement_mode)
	_dot_container.reset_query_node_positions(query_node_speed)

	var original_interval: float = _nq2d.debug_report_interval
	_nq2d.debug_report_interval = 0.0
	await _dot_container.wait_for_debug_report()
	_nq2d.debug_report_interval = benchmark_time
	var report := await _dot_container.wait_for_debug_report()
	_nq2d.debug_report_interval = original_interval

	return report

func _update_range_label() -> void:
	_query_range_label.text = "%d / %d px" % [query_min_range, query_max_range]

func _process(_delta: float) -> void:
	_fps_label.text = "FPS: %d" % Engine.get_frames_per_second()
