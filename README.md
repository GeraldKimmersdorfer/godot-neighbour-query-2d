# <img src="project/icon.svg" width="40" height="40" align="left" style="margin-right:8px"/> NeighbourQuery2D

![Version](https://img.shields.io/badge/version-7-blue) [![Godot 4.6](https://img.shields.io/badge/Godot-4.6-blue?logo=godotengine&logoColor=white)](https://godotengine.org/) ![License](https://img.shields.io/github/license/GeraldKimmersdorfer/godot-neighbour-query-2d)

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


The grid gets refilled every physics frame by default (`refresh_intervall = 0.0`). Set `refresh_intervall` to a positive value to throttle rebuilds; query results may be stale in that case.

The domain and grid cell size are adjustable via properties in the editor.

In debug builds the node emits a `debug_info` signal each physics frame with a `debug_report` string containing per-function timings, frame budget percentages, and call counts.

Check the Godot documentation for more details.

## Setup

**Prerequisites:** [SCons](https://scons.org/) must be installed and available on your PATH.

**Clone with submodules:**

```shell
git clone --recurse-submodules https://github.com/GeraldKimmersdorfer/godot-neighbour-query-2d.git
```

Or if you already cloned without submodules:

```shell
git submodule update --init --recursive
```

**Build:**

```shell
scons
```

The compiled library is placed in `project/bin/`. Import the `project/` folder into Godot to test.
