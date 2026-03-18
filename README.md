# NeighbourhoodServer

A Godot 4 GDExtension that provides fast 2D spatial queries for nodes in a scene. It maintains a uniform grid refilled on the physics tick (at adjustable time intervals). It lets you efficiently nodes nearby.

> Derived from the [godot-cpp GDExtension template](https://github.com/godotengine/godot-cpp-template).

## What it does

`NeighbourhoodServer` is a Node2D you add to your scene. Other nodes register themselves as **subscribers**. You can then query:

- `get_next(position, max_distance, layer_mask, exclude)`
Returns the nearest subscriber within range
- `get_all(position, max_distance, layer_mask, exclude)`
Returns all subscribers within range
- `get_closest(position, max_count, max_distance, layer_mask, exclude)`
Returns up to `max_count` nearest subscribers within range

The grid gets refilled every physics frame by default. Set `refresh_intervall` to throttle rebuilds to at most once per N seconds. The domain and the grid cell size is adjustable via variables in the editor.

Check the Godot-Documentation for more details...

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
