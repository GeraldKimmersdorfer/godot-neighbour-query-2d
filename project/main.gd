extends Node2D

@export var dot_template: PackedScene
@export var dot_count: int = 1000
@export var moving: bool = true

var _dots: Array[Node2D] = []
var _velocities: Array[Vector2] = []

func _ready() -> void:
	var viewport_size := get_viewport_rect().size
	for i in dot_count:
		var dot: Node2D = dot_template.instantiate()
		dot.position = Vector2(randf() * viewport_size.x, randf() * viewport_size.y)
		add_child(dot)
		_dots.append(dot)
		var speed := randf_range(20.0, 120.0)
		var angle := randf() * TAU
		_velocities.append(Vector2(cos(angle), sin(angle)) * speed)

func _process(delta: float) -> void:
	if not moving:
		return
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
