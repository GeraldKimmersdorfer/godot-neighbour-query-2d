extends PanelContainer

class_name OptionContainer

signal movement_mode_changed(mode: Dot.MovementMode)
signal ranges_changed(max_range: float, min_range: float)

@export_group("Scene Controls")
@export var _min_range_label: Label
@export var _max_range_label: Label
@export var _label_fps: Label
@export var _label_movement_mode: Label
@export var _check_button_validation_check: CheckButton

@export_group("")

var query_max_range: float = 150.0:
	set(value):
		query_max_range = value
		_max_range_label.text = "%d px" % value
		ranges_changed.emit(query_max_range, query_min_range)

var query_min_range: float = 70.0:
	set(value):
		query_min_range = value
		_min_range_label.text = "%d px" % value
		ranges_changed.emit(query_max_range, query_min_range)

var movement_mode: Dot.MovementMode = Dot.MovementMode.NONE:
	set(val):
		if val != movement_mode:
			movement_mode = val
			_label_movement_mode.text = Dot.MovementMode.keys()[val]
			movement_mode_changed.emit(val)

var validation_test_enabled: bool:
	get: return _check_button_validation_check.button_pressed

func _ready() -> void:
	_label_movement_mode.text = Dot.MovementMode.keys()[movement_mode]
	query_max_range = query_max_range
	query_min_range = query_min_range

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed:
		if event.keycode == KEY_M:
			movement_mode = (movement_mode + 1) % Dot.MovementMode.size() as Dot.MovementMode
	if event is InputEventMouseButton:
		if event.ctrl_pressed:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				query_min_range = minf(query_min_range + 5.0, query_max_range)
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				query_min_range = maxf(query_min_range - 5.0, 0.0)
		else:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				query_max_range = maxf(query_max_range + 5.0, query_min_range)
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				query_max_range = maxf(query_max_range - 5.0, query_min_range + 10.0)

func _process(_delta: float) -> void:
	if _label_fps:
		_label_fps.text = "FPS: %d" % Engine.get_frames_per_second()
