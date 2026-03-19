# NeighbourhoodServer

A Godot 4 GDExtension that provides fast 2D spatial queries for nodes in a scene. It maintains a uniform grid refilled on the physics tick (at adjustable time intervals). It lets you efficiently find nodes nearby.

> Derived from the [godot-cpp GDExtension template](https://github.com/godotengine/godot-cpp-template).

## What it does

`NeighbourhoodServer` is a Node2D you add to your scene. Other nodes register themselves as **subscribers**. You can then query by world position:

- `get_next(position, max_distance, min_distance, layer_mask, exclude)`
  Returns the nearest subscriber within range.
- `get_next_first(position, max_distance, min_distance, layer_mask, exclude)`
  Returns the first subscriber found within range. Faster than `get_next` since it stops at the first valid hit without searching for the closest.
- `get_next_random(position, max_distance, min_distance, layer_mask, exclude)`
  Returns a random subscriber within range.
- `get_all(position, max_distance, min_distance, layer_mask, exclude)`
  Returns all subscribers within range.
- `get_closest(position, max_count, max_distance, min_distance, layer_mask, exclude)`
  Returns up to `max_count` nearest subscribers within range.


The grid gets refilled every physics frame by default (`refresh_intervall = 0.0`). When `refresh_intervall` is `0.0`, subscriber validity is hopefully guaranteed by the rebuild itself (invalid nodes are pruned each frame), so the query functions skip the per-result validity check entirely - as this is quite performance heavy. Set `refresh_intervall` to a positive value to throttle rebuilds; in that case query functions perform an additional validity check per result since the grid may be stale.

The domain and grid cell size are adjustable via properties in the editor.

Check the Godot documentation for more details.

## Setup

**Prerequisites:** [SCons](https://scons.org/) must be installed and available on your PATH.

**Clone with submodules:**

```shell
git clone --recurse-submodules https://github.com/your-org/godot-neighbourhood-server.git
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
