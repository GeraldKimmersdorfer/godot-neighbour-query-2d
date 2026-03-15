extends Sprite2D

class_name Dot

@export var moving: bool = true

var _velocity: Vector2

func _ready() -> void:
	var speed := randf_range(20.0, 120.0)
	var angle := randf() * TAU
	_velocity = Vector2(cos(angle), sin(angle)) * speed

func _process(delta: float) -> void:
	if not moving:
		return
	var viewport_size := get_viewport_rect().size
	position += _velocity * delta
	if position.x < 0 or position.x > viewport_size.x:
		_velocity.x = -_velocity.x
		position.x = clamp(position.x, 0.0, viewport_size.x)
	if position.y < 0 or position.y > viewport_size.y:
		_velocity.y = -_velocity.y
		position.y = clamp(position.y, 0.0, viewport_size.y)