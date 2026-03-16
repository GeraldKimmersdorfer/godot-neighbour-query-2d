extends Node2D

class_name Grid

@export var grid_size : int = 256 :
	set(value):
		grid_size = value
		queue_redraw()

# Dictionary: Vector2i cell coord -> Color
var _cell_overlays: Dictionary = {}

func _ready() -> void:
	queue_redraw()

func set_cell_overlays(overlays: Dictionary) -> void:
	_cell_overlays = overlays
	queue_redraw()

func _draw() -> void:
	var viewport_rect := get_viewport_rect()

	for cell in _cell_overlays:
		var c := cell as Vector2i
		draw_rect(Rect2(Vector2(c.x * grid_size, c.y * grid_size), Vector2(grid_size, grid_size)), _cell_overlays[cell])

	var color := Color(1, 1, 1, 0.5)
	var cols := int(ceil(viewport_rect.size.x / grid_size)) + 1
	var rows := int(ceil(viewport_rect.size.y / grid_size)) + 1
	for i in range(cols):
		var x := i * grid_size
		draw_line(Vector2(x, 0), Vector2(x, viewport_rect.size.y), color)
	for j in range(rows):
		var y := j * grid_size
		draw_line(Vector2(0, y), Vector2(viewport_rect.size.x, y), color)
