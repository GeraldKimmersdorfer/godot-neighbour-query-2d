extends Sprite2D

@export var moving: bool = true
@export var bounds: Rect2

var velocity: Vector2

func _process(delta: float) -> void:
	if not moving:
		return
	position += velocity * delta
	if position.x < bounds.position.x or position.x > bounds.end.x:
		velocity.x = -velocity.x
		position.x = clamp(position.x, bounds.position.x, bounds.end.x)
	if position.y < bounds.position.y or position.y > bounds.end.y:
		velocity.y = -velocity.y
		position.y = clamp(position.y, bounds.position.y, bounds.end.y)
