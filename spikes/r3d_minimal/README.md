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
