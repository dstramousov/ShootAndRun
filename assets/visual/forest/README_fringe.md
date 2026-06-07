# ShootAndRun forest fringe assets v1

This package adds small visual-only assets for forest edge softening.

## Target path

Unpack from the project root:

```bash
unzip ~/Downloads/ShootAndRun_forest_fringe_assets_v1.zip -d .
```

Expected result:

```text
assets/visual/forest/fringe/
assets/visual/forest/forest_fringe_assets_manifest.json
assets/visual/forest/forest_fringe_rules.json
assets/visual/forest/docs/contact_sheet_fringe_v1.png
```

## Style

Locked to the approved hand-painted top-down fir forest style.

Hard rules:

- no geometric triangle trees;
- no baked grid;
- transparent PNG backgrounds;
- visual-only usage;
- collision and gameplay grids stay controlled by `map_package`.

## Intended renderer usage

Use these assets only near forest edges:

```text
open grass
→ dark grass / moss / litter
→ bushes / ferns / branches
→ saplings / edge shadow
→ medium trees
→ deep forest canopy
```

