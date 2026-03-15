extends Node2D

@export var dot_template: PackedScene
@export var dot_count: int = 1000

var _dots: Array[Node2D] = []
var _velocities: Array[Vector2] = []
var _highlighted: Array[CanvasItem] = []
var _closest_dot: Node2D = null

@onready var _ns: NeighbourhoodServer = $NeighbourhoodServer

func _ready() -> void:
	_ns.refresh_intervall = 0
	var viewport_size := get_viewport_rect().size
	for i in dot_count:
		var dot: Node2D = dot_template.instantiate()
		dot.position = Vector2(randf() * viewport_size.x, randf() * viewport_size.y)
		add_child(dot)
		_dots.append(dot)
		var speed := randf_range(20.0, 120.0)
		var angle := randf() * TAU
		_velocities.append(Vector2(cos(angle), sin(angle)) * speed)
		_ns.subscribe(dot, 1, dot)

func _exit_tree() -> void:
	for dot in _dots:
		_ns.unsubscribe(dot)

func _process(delta: float) -> void:
	var viewport_size := get_viewport_rect().size
	for i in _dots.size():
		var dot := _dots[i]
		var vel := _velocities[i]
		dot.position += vel * delta
		if dot.position.x < 0.0 or dot.position.x > viewport_size.x:
			vel.x = -vel.x
			dot.position.x = clamp(dot.position.x, 0.0, viewport_size.x)
		if dot.position.y < 0.0 or dot.position.y > viewport_size.y:
			vel.y = -vel.y
			dot.position.y = clamp(dot.position.y, 0.0, viewport_size.y)
		_velocities[i] = vel

	for dot in _highlighted:
		dot.modulate = Color.WHITE
	_highlighted.clear()

	var neighbours := _ns.get_all(get_global_mouse_position(), 100.0)
	for n in neighbours:
		var dot := n as CanvasItem
		if dot:
			dot.modulate = Color(1.0, 0.0, 0.0)
			_highlighted.append(dot)

	if _closest_dot:
		(_closest_dot as CanvasItem).modulate = Color.WHITE
	_closest_dot = _ns.get_next(get_global_mouse_position()) as Node2D
	if _closest_dot:
		(_closest_dot as CanvasItem).modulate = Color(0.0, 1.0, 0.0)
