# R3D forest/map slice spike

This is an isolated experiment for the experimental R3D renderer path.
It intentionally does not include or modify the main ShootAndRun runtime loop.

## Goal

Validate whether R3D is useful for the real ShootAndRun forest view, not just for
primitive smoke tests.

The spike now loads:

- `config/app_config.json`;
- `map_package_path` from the app config;
- `config/render3d/asset_library.json`;
- `config/render3d/tileset_dark_forest.json`;
- the existing ShootAndRun `LevelLoader`;
- real `.glb` tree/grass/rock model bindings through R3D + Assimp.

It renders a deterministic map slice around `player_spawn` / `start`, or around
the map center when no start marker exists.

## Build

Run from the repository root:

```bash
git submodule update --init --recursive external/r3d
rm -rf build/r3d_minimal
cmake -S spikes/r3d_minimal -B build/r3d_minimal
cmake --build build/r3d_minimal -j"$(nproc)"
```

This build enables vendored Assimp because R3D needs it to import the project
`.glb` models.

## Run

```bash
./build/r3d_minimal/bin/sar_r3d_minimal
```

Optional overrides:

```bash
./build/r3d_minimal/bin/sar_r3d_minimal --slice=64
./build/r3d_minimal/bin/sar_r3d_minimal --slice=96 --forest-mode=composition
./build/r3d_minimal/bin/sar_r3d_minimal --slice=96 --forest-mode=raw
./build/r3d_minimal/bin/sar_r3d_minimal --map=../TopDownMapGen/output/map_package --slice=64
```

`--slice` is clamped to `16..160` tiles.

`--forest-mode=raw` keeps the first one-forest-tile-to-one-tree experiment.
`--forest-mode=composition` is the default and classifies forest cells as edge, mid, or deep forest. Edge cells keep more readable trees, mid cells are thinned out, and deep forest is rendered as a darker mass with fewer explicit tree models.

## Controls

```text
Q / E      rotate camera yaw
R / F      change camera pitch
Mouse wheel zoom in/out
Esc        close window
```

## Expected result

A window opens with a real map-package fragment:

- terrain-colored tile floor;
- forest floor using edge/mid/deep composition materials;
- real tree/underbrush/detail models selected from the existing 3D tileset;
- primitive fallback trees if a model cannot be loaded;
- primitive wall/object placeholders for collision-heavy ruin/object cells;
- FPS, map slice, forest tile count, and model instance counters in the overlay.

This is still a spike. It is allowed to be visually rough. It now answers two questions:

1. whether R3D + our existing `.glb` assets can render a believable dense forest slice;
2. whether a region-aware forest composition pass looks better than direct tile-to-model placement before we integrate anything into the main renderer.

## Advanced R3D comparison profile

The spike can now switch between a plain baseline profile and an advanced R3D profile:

```bash
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=basic
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=advanced
```

The advanced profile enables the R3D-specific features that must be evaluated
before deciding whether R3D is worth integrating into the main renderer:

- model instancing grouped by model id and map chunk;
- R3D cluster blocks for coarse chunk culling;
- simple distance LOD for underbrush and small ground details;
- directional-light shadow maps;
- fog, ambient setup, SSAO, tonemapping, and color grading;
- tuned material roughness/specular values for terrain, ruins, and water.

Useful toggles:

```bash
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=advanced --no-shadows
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=advanced --no-fog
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=advanced --no-instancing
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=advanced --no-lod
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=advanced --r3d-chunk-size=16
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=advanced --r3d-fog-density=0.018
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --r3d-profile=advanced --r3d-shadow-opacity=0.58
```

Compare `basic` and `advanced` on the same slice and camera angle. If the
advanced profile does not provide a visible improvement from shadows/fog/lighting
or does not recover enough CPU cost through instancing/chunk culling, R3D should
not be promoted into the main renderer yet.

## Feature benchmark modes

The spike also has focused benchmark presets. They are not final gameplay modes;
they isolate one R3D feature family at a time so the result can be compared
without guessing what changed.

```bash
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=forest
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=instancing
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=culling
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=lighting
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=pbr
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=terrain-mesh
```

Recommended comparisons:

```bash
# Instancing benefit.
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=instancing
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=instancing --no-instancing

# Chunk / cluster culling benefit.
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=culling --r3d-chunk-size=8
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=culling --r3d-chunk-size=32

# Visual value from the R3D lighting stack.
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=lighting
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=lighting --no-shadows
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=lighting --no-fog

# Material/PBR visibility on surfaces where it can actually matter.
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=pbr

# Coarse terrain chunk surface experiment. This is intentionally visually rough.
./build/r3d_minimal/bin/sar_r3d_minimal --slice=160 --benchmark=terrain-mesh
```

The overlay shows the active benchmark, instancing counters, chunk state, LOD
hidden count, shadow/fog state, coarse terrain mode, and PBR probe state.
