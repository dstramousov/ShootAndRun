# ShootAndRun forest assets v2 style locked

This package contains PNG assets for the forest renderer prototype.

## Target path

Unpack from the project root:

```bash
unzip ~/Downloads/ShootAndRun_forest_assets_v2_style_locked.zip -d .
```

Expected result:

```text
assets/visual/forest/
```

## Notes

- The pack is locked to the approved bright forest sheet style.
- Trees are not geometric placeholder triangles.
- Runtime/gameplay collision must still come from `map_package/runtime_grids.json`.
- Tall tree sprites are visual objects with `bottom_center` anchors.
- Tile assets use the 16x16 world grid.

## Next code step

`v0.1.31 forest asset renderer` should load:

```text
assets/visual/forest/forest_assets_manifest.json
assets/visual/forest/forest_rules.json
```

and map the existing forest masks to these asset ids.
