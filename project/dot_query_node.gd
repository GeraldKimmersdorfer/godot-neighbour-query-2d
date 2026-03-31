extends Node2D

class_name DotQueryNode

const SPEED: float = 120.0
const CLOSEST_COUNT: int = 5
const COLORS = [
	Color(1.0, 0.5, 0.0),  # GET_ALL - orange
	Color(0.0, 0.9, 0.0),  # GET_NEXT - green
	Color(0.0, 0.7, 1.0),  # GET_CLOSEST - cyan
	Color(0.9, 0.0, 1.0),  # GET_RANDOM - magenta
	Color(1.0, 0.9, 0.0),  # GET_NEXT_FIRST - yellow
]

enum QueryFunc { GET_ALL, GET_NEXT, GET_CLOSEST, GET_RANDOM, GET_NEXT_FIRST }

@export var query_func: QueryFunc = QueryFunc.GET_ALL:
	set(value):
		query_func = value
		queue_redraw()
@export var query_max_range: float = 150.0:
	set(value):
		query_max_range = value
		queue_redraw()
@export var query_min_range: float = 70.0:
	set(value):
		query_min_range = value
		queue_redraw()
@export var bounds: Rect2
@export var nq2d: NeighbourQuery2D

var velocity: Vector2
var _highlighted: Array = []

func _ready() -> void:
	var angle := randf() * TAU
	velocity = Vector2(cos(angle), sin(angle)) * SPEED

func _exit_tree() -> void:
	for dot in _highlighted:
		if is_instance_valid(dot):
			dot.remove_highlight(self)

func _process(delta: float) -> void:
	position += velocity * delta
	if position.x < bounds.position.x or position.x > bounds.end.x:
		velocity.x = -velocity.x
		position.x = clamp(position.x, bounds.position.x, bounds.end.x)
	if position.y < bounds.position.y or position.y > bounds.end.y:
		velocity.y = -velocity.y
		position.y = clamp(position.y, bounds.position.y, bounds.end.y)

func _physics_process(_delta: float) -> void:
	for dot in _highlighted:
		if is_instance_valid(dot):
			dot.remove_highlight(self)

	var result: Array = []
	match query_func:
		QueryFunc.GET_ALL:
			result = nq2d.get_all(position, query_max_range, query_min_range, Dot.LAYER_ACTIVE)
		QueryFunc.GET_NEXT:
			var r = nq2d.get_next(position, query_max_range, query_min_range, Dot.LAYER_ACTIVE)
			if r: result = [r]
		QueryFunc.GET_CLOSEST:
			result = nq2d.get_closest(position, CLOSEST_COUNT, query_max_range, query_min_range, Dot.LAYER_ACTIVE)
		QueryFunc.GET_RANDOM:
			result = nq2d.get_random(position, CLOSEST_COUNT, query_max_range, query_min_range, Dot.LAYER_ACTIVE)
		QueryFunc.GET_NEXT_FIRST:
			var r = nq2d.get_next_first(position, query_max_range, query_min_range, Dot.LAYER_ACTIVE)
			if r: result = [r]

	var color: Color = COLORS[query_func]
	_highlighted.clear()
	for dot in result:
		if is_instance_valid(dot):
			dot.add_highlight(self, color)
			_highlighted.append(dot)

func _draw() -> void:
	var color: Color = COLORS[query_func]
	draw_arc(Vector2.ZERO, query_max_range, 0.0, TAU, 64, color, 2)
	if query_min_range > 0.0:
		draw_arc(Vector2.ZERO, query_min_range, 0.0, TAU, 64, Color(color.r, color.g, color.b, 0.7), 2)
	draw_circle(Vector2.ZERO, 4.0, color)
