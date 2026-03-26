extends PanelContainer

class_name OptionContainer

enum QueryMode { GET_ALL, GET_NEXT, GET_CLOSEST, GET_RANDOM, GET_NEXT_FIRST }

@export_group("Scene Controls")
@export var _label_mode: Label
@export var _min_range_slider: Slider
@export var _max_range_slider: Slider
@export var _label_fps: Label

@export_group("")

var mode: QueryMode = QueryMode.GET_ALL
var query_max_range: float = 150.0
var query_min_range: float = 70.0

var _updating_sliders: bool = false

func _ready() -> void:
	_min_range_slider.value_changed.connect(_on_min_range_slider_changed)
	_max_range_slider.value_changed.connect(_on_max_range_slider_changed)
	_sync_sliders()
	_sync_mode_label()

func _input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed:
		match event.keycode:
			KEY_1: _set_mode(QueryMode.GET_ALL)
			KEY_2: _set_mode(QueryMode.GET_NEXT)
			KEY_3: _set_mode(QueryMode.GET_CLOSEST)
			KEY_4: _set_mode(QueryMode.GET_RANDOM)
			KEY_5: _set_mode(QueryMode.GET_NEXT_FIRST)
	if event is InputEventMouseButton:
		if event.ctrl_pressed:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				_set_min_range(minf(query_min_range + 10.0, query_max_range))
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				_set_min_range(maxf(query_min_range - 10.0, 0.0))
		else:
			if event.button_index == MOUSE_BUTTON_WHEEL_UP:
				_set_max_range(maxf(query_max_range + 10.0, query_min_range))
			elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
				_set_max_range(maxf(query_max_range - 10.0, query_min_range + 10.0))

func _set_mode(new_mode: QueryMode) -> void:
	mode = new_mode
	_sync_mode_label()

func _set_max_range(value: float) -> void:
	query_max_range = value
	_sync_sliders()

func _set_min_range(value: float) -> void:
	query_min_range = value
	_sync_sliders()

func _sync_mode_label() -> void:
	if _label_mode:
		_label_mode.text = QueryMode.keys()[mode].to_lower()

func _process(_delta: float) -> void:
	if _label_fps:
		_label_fps.text = "FPS: %d" % Engine.get_frames_per_second()

func _sync_sliders() -> void:
	_updating_sliders = true
	if _min_range_slider:
		_min_range_slider.value = query_min_range
	if _max_range_slider:
		_max_range_slider.value = query_max_range
	_updating_sliders = false

func _on_min_range_slider_changed(value: float) -> void:
	if _updating_sliders:
		return
	_set_min_range(minf(value, query_max_range))

func _on_max_range_slider_changed(value: float) -> void:
	if _updating_sliders:
		return
	_set_max_range(maxf(value, query_min_range + 10.0))
