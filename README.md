# NeighbourhoodServer

A Godot 4 GDExtension that provides fast 2D spatial queries for nodes in a scene. It maintains a uniform hash grid updated on the physics tick, letting you efficiently find the nearest node or all nodes within a radius.

> Derived from the [godot-cpp GDExtension template](https://github.com/godotengine/godot-cpp-template).

## What it does

`NeighbourhoodServer` is a singleton-style node you add to your scene. Other nodes register themselves as **subscribers**. You can then query:

- `get_next(position, max_distance, layer_mask)` — returns the nearest subscriber within range
- `get_all(position, max_distance, layer_mask)` — returns all subscribers within range

The grid is rebuilt every `refresh_interval` seconds (default `0.1`). Grid cell size and whether to use global or local positions are configurable.

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
