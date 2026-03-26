extends Sprite2D

class_name Dot

const LAYER_ACTIVE = 1
const LAYER_INACTIVE = 2

@export var inactive: bool = false:
	set(value):
		inactive = value
		modulate.a = 0.4 if inactive else 1.0
		set_process(not inactive)
		if is_inside_tree() and is_instance_valid(nq2d):
			nq2d.unsubscribe(self)
			nq2d.subscribe(self, LAYER_INACTIVE if inactive else LAYER_ACTIVE)

@export var bounds: Rect2

var nq2d: NeighbourQuery2D
var velocity: Vector2

func _ready() -> void:
	position = Vector2(randf_range(bounds.position.x, bounds.end.x), randf_range(bounds.position.y, bounds.end.y))
	nq2d.subscribe(self, LAYER_INACTIVE if inactive else LAYER_ACTIVE)
	var speed := randf_range(20.0, 120.0)
	var angle := randf() * TAU
	velocity = Vector2(cos(angle), sin(angle)) * speed

func _exit_tree() -> void:
	if is_instance_valid(nq2d):
		nq2d.unsubscribe(self)

func _process(delta: float) -> void:
	position += velocity * delta
	if position.x < bounds.position.x or position.x > bounds.end.x:
		velocity.x = -velocity.x
		position.x = clamp(position.x, bounds.position.x, bounds.end.x)
	if position.y < bounds.position.y or position.y > bounds.end.y:
		velocity.y = -velocity.y
		position.y = clamp(position.y, bounds.position.y, bounds.end.y)
