extends Node2D

@export var dot_template: PackedScene
@export var dot_count: int = 1000

func _ready() -> void:
	var viewport_size := get_viewport_rect().size
	for i in dot_count:
		var dot := dot_template.instantiate()
		dot.position = Vector2(randf() * viewport_size.x, randf() * viewport_size.y)
		add_child(dot)
