# <img src="project/icon.svg" width="40" height="40" align="left" style="margin-right:8px"/> NeighbourQuery2D

![Version](https://img.shields.io/badge/version-8-blue) [![Godot 4.6](https://img.shields.io/badge/Godot-4.6-blue?logo=godotengine&logoColor=white)](https://godotengine.org/) ![License](https://img.shields.io/github/license/GeraldKimmersdorfer/godot-neighbour-query-2d)

A Godot 4 GDExtension that provides fast 2D spatial queries for nodes in a scene. It maintains a uniform grid refilled on the physics tick (at adjustable time intervals). It lets you efficiently find nodes nearby.

> Derived from the [godot-cpp GDExtension template](https://github.com/godotengine/godot-cpp-template).

## What it does

`NeighbourQuery2D` is a Node2D you add to your scene. Other nodes register themselves as **subscribers**. You can then query by world position:

- `get_next(position, max_distance, min_distance, layer_mask, exclude)`
  Returns the nearest subscriber within range.
- `get_next_first(position, max_distance, min_distance, layer_mask, exclude)`
  Returns the first subscriber found within range. Faster than `get_next` since it stops at the first valid hit without searching for the closest.
- `get_random(position, max_count, max_distance, min_distance, layer_mask, exclude)`
  Returns up to `max_count` randomly selected subscribers within range.
- `get_all(position, max_distance, min_distance, layer_mask, exclude)`
  Returns all subscribers within range.
- `get_closest(position, max_count, max_distance, min_distance, layer_mask, exclude)`
  Returns up to `max_count` nearest subscribers within range.


The grid (adjustable via `domain`and `grid_size`) gets refilled every physics frame by default, but that can be throttled (`refresh_intervall`). Query results then may not be accurate, but they do promise that all of the resulting Nodes are alive.

The domain and grid cell size are adjustable via properties in the editor.

In debug builds the node emits a `debug_info` signal each physics frame with a `debug_report` string containing per-function timings, frame budget percentages, and call counts.

Check the Godot documentation for more details.

## Usage-Setup

**1. Download the latest release**

Get the latest release from the [Releases page](https://github.com/GeraldKimmersdorfer/godot-neighbour-query-2d/releases).

**2. Add to your project**

Move the `neighbour-query-2d` folder into your project's `addons/` folder. No plugin activation is necessary.

**3. Add the node to your scene**

Place a `NeighbourQuery2D` node in your level scene. In the inspector, set the `domain` rect to cover the full level area. Nodes outside the domain are ignored.

**4. Subscribe and query Example**

```gdscript
extends Node2D

@onready var nq: NeighbourQuery2D = $NeighbourQuery2D

## layers are bitmasks: use powers of 2, combine with | (e.g.
## LAYER_ENEMIES | LAYER_PICKUPS), or pass 0xFFFFFFFF for all
const LAYER_ENEMIES := 1  
const LAYER_PLAYERS := 2

func _ready() -> void:
    nq.subscribe(self, LAYER_ENEMIES)

func _exit_tree() -> void:
    # Not strictly necessary after freeing, as it gets auto-removed in that case
    nq.unsubscribe(self)

func _physics_process(_delta: float) -> void:
    # Push away from the 5 closest enemies within 300px
    var push := Vector2.ZERO
    for enemy in nq.get_closest(global_position, 5, 300.0, 0.0, LAYER_ENEMIES, self):
        var offset: Vector2 = global_position - enemy.global_position
        var dist: float = offset.length()
        if dist > 0.0:
            push += offset / dist * (1.0 - dist / 300.0)  # stronger when closer

    # Pull toward the nearest player
    var pull := Vector2.ZERO
    var player: Node2D = nq.get_next(global_position, INF, 0.0, LAYER_PLAYERS, self)
    if player:
        pull = (player.global_position - global_position).normalized()

    global_position += (push + pull) * delta
```


## Development-Setup
> [!IMPORTANT]
> If you only want to use the Extension in your project please download the latest Release (see [Usage-Setup](#usage-setup)). It already contains the compiled library for most targets.

If you want to modify the Extension you'll need the following:
 - [SCons](https://scons.org/) must be installed and available on your PATH.
 - A C++ Compiler (developed with MSVC)

### Clone and build:

```shell
git clone --recurse-submodules https://github.com/GeraldKimmersdorfer/godot-neighbour-query-2d.git
scons
```

As a post-build step the compiled library is placed in `bin` and copied into `project/bin/`.

### Benchmark / Example Project
Import the `project/` folder into Godot to test.