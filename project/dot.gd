extends Sprite2D

class_name Dot

const LAYER_ACTIVE = 1
const LAYER_INACTIVE = 2

const BOID_NEIGHBOUR_COUNT: int = 5
const BOID_PERCEPTION_RADIUS: float = 50.0
const BOID_SEPARATION_WEIGHT: float = 100.0
const BOID_ALIGNMENT_WEIGHT: float = 1.0
const BOID_COHESION_WEIGHT: float = 1.0
const BOID_MAX_SPEED: float = 200.0
const BOID_MAX_FORCE: float = 600.0
const BOID_BOUNDS_FORCE: float = 1.0 ## force to pull back when outside
const BOID_USE_RANDOM_NEIGHBOURS: bool = false

enum MovementMode { NONE, STRAIGHT, BOID }

@export var inactive: bool = false:
	set(value):
		inactive = value
		modulate.a = 0.4 if inactive else 1.0
		if is_inside_tree() and is_instance_valid(nq2d):
			nq2d.subscribe(self, LAYER_INACTIVE if inactive else LAYER_ACTIVE)

@export var bounds: Rect2
@export var movement_mode: MovementMode = MovementMode.STRAIGHT:
	set(value):
		movement_mode = value
		if value == MovementMode.BOID: _movement_func = _boid_process
		elif value == MovementMode.STRAIGHT: _movement_func = _straight_process
		else: _movement_func = _none_process
		set_process(value != MovementMode.NONE and not inactive)

var nq2d: NeighbourQuery2D
var velocity: Vector2

var _movement_func: Callable

func _ready() -> void:
	position = Vector2(randf_range(bounds.position.x, bounds.end.x), randf_range(bounds.position.y, bounds.end.y))
	var angle := randf() * TAU
	velocity = Vector2(cos(angle), sin(angle)) * randf_range(20.0, BOID_MAX_SPEED)
	movement_mode = movement_mode # call setter to init _movement_func
	inactive = inactive # call setter for proper init

func _process(delta: float) -> void:
	_movement_func.call(delta)

func _none_process(_delta: float) -> void:
	pass

func _straight_process(delta: float) -> void:
	position += velocity * delta
	if position.x < bounds.position.x or position.x > bounds.end.x:
		velocity.x = -velocity.x
		position.x = clamp(position.x, bounds.position.x, bounds.end.x)
	if position.y < bounds.position.y or position.y > bounds.end.y:
		velocity.y = -velocity.y
		position.y = clamp(position.y, bounds.position.y, bounds.end.y)

func _boid_process(delta: float) -> void:
	var separation := Vector2.ZERO
	var avg_velocity := Vector2.ZERO
	var center_of_mass := Vector2.ZERO
	var count := 0
	var layer := LAYER_INACTIVE if inactive else LAYER_ACTIVE
	var query_result := nq2d.get_random(position, BOID_NEIGHBOUR_COUNT, BOID_PERCEPTION_RADIUS, 0.0, layer, self) if BOID_USE_RANDOM_NEIGHBOURS else nq2d.get_closest(position, BOID_NEIGHBOUR_COUNT, BOID_PERCEPTION_RADIUS, 0.0, layer, self)
	for n in query_result:
		var dot := n as Dot
		if not dot:
			continue
		count += 1
		var diff := position - dot.position
		var dist := diff.length()
		if dist > 0.0:
			separation += diff.normalized() / dist
		avg_velocity += dot.velocity
		center_of_mass += dot.position

	var steering := (bounds.get_center() - position) * BOID_BOUNDS_FORCE if not bounds.has_point(position) else Vector2.ZERO
	if count > 0:
		avg_velocity /= count
		center_of_mass /= count
		steering += separation * BOID_SEPARATION_WEIGHT \
			+ (avg_velocity - velocity).limit_length(BOID_MAX_FORCE) * BOID_ALIGNMENT_WEIGHT \
			+ (center_of_mass - position).limit_length(BOID_MAX_FORCE) * BOID_COHESION_WEIGHT

	velocity = (velocity + steering.limit_length(BOID_MAX_FORCE) * delta).limit_length(BOID_MAX_SPEED)
	if velocity.length_squared() < 400.0:
		velocity = velocity.normalized() * 20.0
	position += velocity * delta
