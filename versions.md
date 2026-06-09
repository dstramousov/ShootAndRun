# Versions

## v0.1.0

- Initial C++20 + raylib bootstrap.
- Added application window, main menu, confirm dialog, FPS display, debug overlay, logger, normalized input layer, and initial level data structures.

## v0.1.0 -> v0.1.1

- Fixed Linux terminal detection includes.
- Fixed raylib 5.5 rounded-line API usage.
- Fixed `Application::LogStartup()` const-correctness issue.


## v0.1.1 -> v0.1.2

- Added `versions.md` as the project changelog.
- Added `.gitignore` for `build/`, `.deps/`, editor noise, and binary artifacts.
- Moved CMake `FetchContent` cache to the repository-level `.deps/` directory.
- Fixed input routing so `Esc` is handled by the active screen instead of always using menu input.
- Added game-screen `Esc` handling: the game placeholder returns to the main menu.
- Re-applied target FPS after starting a new game to keep frame pacing stable.

## v0.1.2 -> v0.1.3

- Bumped project and runtime application version to `0.1.3`.
- Added explicit application `target_fps` configuration.
- Centralized raylib frame pacing through `Application::ApplyFramePacing()`.
- Reworked `Esc` detection to use a tracked key-down edge instead of relying only on raylib pressed events.
- Added game-screen `Esc` down-state handling so the placeholder game screen returns to the main menu reliably.

## v0.1.3 -> v0.1.4

- Bumped project and runtime application version to `0.1.4`.
- Added `config/app_config.json` with `map_package_path`.
- Added project configuration loader for the map package path.
- Added `--config=` CLI argument to override the project config path.
- Updated `New Game` flow to read and validate the map package path from configuration before entering the game screen.
- Updated README with project config usage.

## v0.1.4 -> v0.1.5

- Bumped project and runtime application version to `0.1.5`.
- Added configurable UI font path and font size to `config/app_config.json`.
- Added RAII-managed UI font loading with fallback to the raylib default font.
- Updated title, menu, dialog, FPS, debug overlay, and game placeholder text to use the configured UI font.
- Added basic `LevelLoader` validation for `terrain.json` and `runtime_grids.json`.
- Updated `New Game` flow to load and validate the configured map package before entering the game screen.

## v0.1.5 -> v0.1.6

- Bumped CMake project and runtime application version to `0.1.6`.
- Added `SAR_APP_VERSION` compile definition as the runtime version source.
- Added `--version` CLI argument.
- Fixed configurable UI font size usage across title, menu, dialog, FPS, debug overlay, and game placeholder text.

## v0.1.6 -> v0.1.7

- Bumped CMake project and runtime application version to `0.1.7`.
- Updated default `map_package_path` to point at the map package root.
- Added `map.json` manifest support to `LevelLoader`.
- Added support for manifest-based terrain and runtime grid file paths.
- Added support for terrain layers stored in `layers/terrain.json`.
- Added support for runtime grids stored as nested `{ format, rows }` grid objects.
- Kept flat `terrain.json` and `runtime_grids.json` loading as a fallback.


## v0.1.7 -> v0.1.8

- Bumped CMake project and runtime application version to `0.1.8`.
- Added configurable preferred window size to `config/app_config.json`.
- Added configurable fallback window size, monitor fraction, and resize flag.
- Updated window layout to use a larger default `1600x900` preferred window.
- Kept `1280x720` as the UI reference size so large windows scale UI up.
- Updated project config loading, tests, and README for window settings.

## v0.1.8 -> v0.1.9

- Bumped CMake project and runtime application version to `0.1.9`.
- Added configurable service info overlay controls to `config/app_config.json`.
- Added process RSS memory display to the service info overlay with throttled updates.
- Added configurable raylib trace log level and set the default to `warning` to hide normal startup noise.
- Updated project config dump and tests for service info and raylib log settings.

## v0.1.9 -> v0.1.10

- Bumped CMake project and runtime application version to `0.1.10`.
- Added `LevelData` terrain cells to the basic map package load result.
- Added debug terrain renderer for the loaded map package.
- Added free camera panning with WASD/arrow keys and mouse-wheel zoom in the game screen.
- Added camera clamping to map bounds and visible-tile rendering.
- Updated terrain type aliases for current generator terrain identifiers.
- Updated README and tests for the debug level renderer step.

## v0.1.10 -> v0.1.11

- Bumped CMake project and runtime application version to `0.1.11`.
- Added optional `markers.json` loading through the map package manifest or package root fallback.
- Added marker count to `LevelPackageSummary` diagnostics.
- Added debug marker rendering on top of the terrain view.
- Updated initial free-camera centering to prefer `player_spawn`/spawn markers when available.
- Updated tests for manifest-based marker loading.

## v0.1.11 -> v0.1.12

- Added an isolated `src/visual_pipeline` subsystem for map preparation.
- Added `VisualPreparationPipeline`, `PipelineProgress`, and `PreparedLevel` skeletons.
- Changed `New Game` flow to load raw map data, show a preparation progress screen, run preparation steps, and enter the game screen only after preparation completes.
- Added progress bar and current preparation step text on the map preparation screen.
- Kept terrain rendering as a temporary debug view after preparation while the visual pipeline is being filled in.

## v0.1.12 -> v0.1.13

- Bumped CMake project and runtime application version to `0.1.13`.
- Added real semantic mask generation inside the isolated `visual_pipeline` subsystem.
- Added terrain masks for open ground, forest, road, swamp, water, ruins, walls, and unknown terrain.
- Added runtime masks for walkable, blocked, vision-blocked, projectile-blocked, cover, concealment, and height data.
- Added semantic mask summary counters to `PreparedLevel` diagnostics.
- Added tests for semantic mask generation and pipeline output.

## v0.1.13 -> v0.1.14

- Bumped CMake project and runtime application version to `0.1.14`.
- Added configurable log execution context output through `log.show_execution_context`.
- Added visual pipeline step start/done diagnostics with per-step duration.
- Added map analysis summary logging for terrain and runtime semantic masks.
- Added warnings for unknown terrain tiles found during semantic mask generation.


## v0.1.14 -> v0.1.15

- Bumped CMake project and runtime application version to `0.1.15`.
- Added separate developer-only log configuration in `config/developer_log_config.json`.
- Moved developer log controls out of the user-facing application config.
- Added configurable log color, execution-context visibility, visual-pipeline diagnostics, and regex highlight rules.
- Added CLI support for `--developer-config=<path>`.

## v0.1.15 -> v0.1.16

- Bumped CMake project and runtime application version to `0.1.16`.
- Fixed `LoggerConfig` initialization to remove the missing `highlight_rules` initializer warning.
- Replaced fragile positional aggregate initialization with explicit field initialization.


## v0.1.16 -> v0.1.17

- Bumped CMake project and runtime application version to `0.1.17`.
- Added real connected-component terrain region building to the isolated `visual_pipeline` subsystem.
- Added `TerrainRegion`, `TerrainRegions`, and region summary diagnostics for forest, open ground, road, swamp, water, ruins, wall, and unknown terrain.
- Added border and inner tile counters for each terrain region as input for future smoothing passes.
- Replaced the terrain-region placeholder pipeline step with `BuildTerrainRegionsStep`.
- Added terrain-region summary and warning diagnostics to the visual preparation pipeline.
- Added tests for terrain region generation.

## v0.1.17 -> v0.1.18

- Bumped CMake project and runtime application version to `0.1.18`.
- Added `RegionBorders` and `RegionBorderInfo` data structures inside the isolated `visual_pipeline` subsystem.
- Replaced the border-smoothing placeholder with a real `Classify region borders` pipeline step.
- Added classification for edge, corner, thin, complex, and map-edge border tiles.
- Added neighboring terrain counters for region borders as input for future smoothing and transition passes.
- Added region-border diagnostics to the visual preparation pipeline log.
- Added tests for region border classification.

## v0.1.18 -> v0.1.19

- Bumped CMake project and runtime application version to `0.1.19`.
- Added catalog-aware terrain semantic mapping through `catalogs/tile_types.json` from the map package manifest.
- Added raw terrain type count diagnostics and unknown terrain type diagnostics to the visual pipeline log.
- Loaded runtime grid values into `RuntimeCell` instead of only validating grid shapes.
- Fixed runtime semantic mask summaries for walkable, blocked, vision/projectile blocking, cover, concealment, and height.
- Added tests for catalog-aware terrain mapping and runtime grid cell data.

## v0.1.19 -> v0.1.20

- Bumped CMake project and runtime application version to `0.1.20`.
- Added `visual_pipeline` configuration for prepared visual-map loading mode, fallback behavior, and optional C++ analysis.
- Added `VisualMapLoader` and `VisualMapData` for loading `visual_map/visual_map.json`, visual layers, visual objects, and visual chunks.
- Added prepared visual-map validation against raw map package dimensions.
- Extended `PreparedLevel` with source tracking and prepared visual-map summary data.
- Updated the map preparation pipeline to use prepared Python `visual_map` data when available while keeping the C++ analysis pipeline as fallback/debug data.
- Added tests for prepared visual-map loading and pipeline integration.

## v0.1.20 -> v0.1.21

- Bumped CMake project and runtime application version to `0.1.21`.
- Changed window sizing so `max_monitor_fraction` is a hard upper limit and oversized preferred windows are scaled down proportionally instead of falling straight back to the fallback size.
- Added developer-log controls for compact visual-pipeline summaries and optional per-step details.
- Replaced noisy INFO-level visual-pipeline step logs with a single compact map preparation report.
- Moved detailed visual-pipeline step diagnostics to DEBUG logging when `visual_pipeline_step_details` is enabled.

## v0.1.21 -> v0.1.22

- Bumped CMake project and runtime application version to `0.1.22`.
- Added parsed prepared visual-map layer grids and visual object data to `VisualMapData`.
- Added a prepared visual-map render mode that draws Python-prepared visual tiles and placeholder visual objects.
- Added runtime view switching in the game screen: `F1` raw terrain, `F2` C++ analysis borders, `F3` prepared visual map.
- Added concise game overlay text showing the active view mode and prepared-level source.

## v0.1.22 -> v0.1.23

- Bumped CMake project and runtime application version to `0.1.23`.
- Added prepared `visual_map/final_render.png` reference loading as a raylib texture.
- Added `F4` game view mode for the baked final-render reference image.
- Defaulted the game view to the final-render reference when it is available, while keeping `F1` raw terrain, `F2` C++ analysis, and `F3` prepared visual-map placeholder views.
- Extended visual-map diagnostics and overlay data with the final-render reference path.

## v0.1.23 -> v0.1.24

- Bumped CMake project and runtime application version to `0.1.24`.
- Added a C++ ruin-scene visual pass that groups ruin floor and wall cells into sites.
- Added wall detail classification for intact, broken, corner, and endcap wall pieces.
- Added visual-only ruin floor, rubble, and entrance roles for cleaner preview rendering.
- Added ruin debug artifacts: `06_ruin_regions.png`, `06_ruin_compositions.png`, and `06_ruins_pass.json`.
- Updated the visual intent preview to draw ruin compositions instead of raw black wall lines.


## v0.1.24 -> v0.1.25

- Bumped CMake project and runtime application version to `0.1.25`.
- Finalized ruin preview readability for the current ruin pass.
- Reduced visual-only ruin dressing spread so rubble and overgrowth no longer flood central ruin sites.
- Increased non-black wall contrast so wall cores remain readable without returning to the old technical black-line look.
- Kept ruin rendering visual-only: gameplay collision, markers, routes, and runtime grids remain unchanged.

## v0.1.25 -> v0.1.26

- Bumped CMake project and runtime application version to `0.1.26`.
- Added a visual-only water and swamp pass for `water_core`, `water_edge`, `mud_ring`, `wet_grass`, `reed_zone`, and crossing roles.
- Added water debug artifacts: `07_water_regions.png`, `07_water_visual.png`, and `07_water_pass.json`.
- Updated the visual intent preview to draw water-zone intent without changing collision, markers, routes, or runtime grids.

## v0.1.26 -> v0.1.27

- Bumped CMake project and runtime application version to `0.1.27`.
- Added typed runtime-object visual mapping and removed `object.generic` from the C++ visual preview path.
- Added object mapping debug artifacts: `08_object_mapping.png`, `08_object_fallbacks.png`, and `08_object_mapping.json`.
- Kept object mapping visual-only: roads, ruins, water, routes, collision, markers, and runtime grids remain unchanged.

## v0.1.27 -> v0.1.28

- Bumped CMake project and runtime application version to `0.1.28`.
- Added a visual-only micro-scene dressing pass for camps, roadside debris, logging spots, ruin debris clusters, swamp crossing details, object-scene dressing, and cache hints.
- Added micro-scene debug artifacts: `09_micro_scenes.png`, `09_micro_scene_dressing.png`, and `09_micro_scenes.json`.
- Updated the visual intent preview to draw micro-scene ground dressing below typed runtime objects.
- Kept micro-scenes visual-only: roads, ruins, water, routes, collision, markers, and runtime grids remain unchanged.

## v0.1.28 -> v0.1.29

- Bumped CMake project and runtime application version to `0.1.29`.
- Added final C++ visual package writing to `prepared_map/visual_map.json`, `visual_layers.json`, `visual_objects.json`, and `visual_chunks.json`.
- Added C++ pipeline `final_render.png` generation instead of relying only on the old prepared reference render.
- Added final package reports: `final_render_report.json`, `visual_density_report.json`, and `quality_score.json`.
- Kept the final package visual-only: gameplay grids, collision, markers, and routes remain unchanged.


## v0.1.29 -> v0.1.30

- Bumped CMake project and runtime application version to `0.1.30`.
- Clarified final-render runtime logging now that the C++ pipeline writes `prepared_map/final_render.png`.
- Renamed the F4 view label from `final_render_reference` to `final_render_package` while keeping old external final renders as fallback references.
- Kept rendering behavior unchanged: F3 remains the visual intent preview, and F4 displays the generated final render package when available.

## v0.1.30 -> v0.1.31

- Bumped CMake project and runtime application version to `0.1.31`.
- Added runtime loading for the style-locked forest asset pack from `assets/visual/forest`.
- Added asset-driven forest drawing for F3 visual intent preview and F4 final-render package view.
- Kept the forest asset renderer visual-only: gameplay collision, markers, routes, runtime grids, and prepared semantics remain unchanged.

## v0.1.31 -> v0.1.32

- Bumped CMake project and runtime application version to `0.1.32`.
- Tuned forest asset placement so the renderer no longer tiles PNG forest ground and edge textures over every forest cell.
- Changed the forest asset renderer to keep forest depth as the visual base and place trees, bushes, clusters, canopy, and shadows sparsely as sprites.
- Added deterministic jitter and neighborhood gates for forest sprites so forest areas read as organic masses instead of a repeated texture carpet.
- Kept the forest asset renderer visual-only: gameplay collision, markers, routes, runtime grids, and prepared semantics remain unchanged.

## v0.1.32 -> v0.1.33

- Bumped CMake project and runtime application version to `0.1.33`.
- Reworked forest asset composition so forest regions render as denser visual masses instead of sparse trees on a dark mask.
- Added visual-only forest mass underlay blending to soften square tile contours without changing gameplay masks.
- Increased edge/mid/deep tree placement density with depth-aware sprite scaling: smaller trees near edges, larger trees and canopy clusters inside deep forest.
- Kept forest composition visual-only: gameplay collision, markers, routes, runtime grids, and prepared semantics remain unchanged.

## v0.1.34 -> v0.1.35

- Bumped CMake project and runtime application version to `0.1.35`.
- Replaced the rejected large-canopy runtime experiment with a safe forest density pass based on regular tree sprites.
- Added zoom-aware forest rendering budgets so zoomed-out views skip expensive forest details instead of drawing thousands of transparent sprites.
- Increased deep-forest density through deterministic grouped placement while keeping edge trees smaller and sparser.
- Disabled runtime canopy/cluster placement in the forest preview path; large canopy assets remain unused until a baked/chunked renderer exists.
- Kept forest rendering visual-only: gameplay collision, markers, routes, runtime grids, and prepared semantics remain unchanged.

## v0.1.35 -> v0.1.36

- Bumped CMake project and runtime application version to `0.1.36`.
- Added baked forest mass pattern fill for forest interiors so deep and mid forest read as continuous dense canopy instead of individually placed runtime trees.
- Tightened forest depth classification so deep forest starts closer to the forest edge and more interior tiles receive the pattern-fill treatment.
- Changed F4 `final_render_package` to display only baked `prepared_map/final_render.png` without re-drawing the live forest asset overlay on top.
- Reduced runtime tree placement inside deep forest; F3 now relies on cheap visual mass stamps plus fringe/detail instead of thousands of tree sprites.
- Added forest pattern-fill counters to final render and visual density reports.
- Kept forest filling visual-only: gameplay collision, markers, routes, runtime grids, and prepared semantics remain unchanged.
## v0.1.36 -> v0.1.37

- Bumped CMake project and runtime application version to `0.1.37`.
- Replaced the temporary triangle forest mass stamps with layered spruce stamps built from rounded branch lobes, trunk pixels, highlights and shadowed lower boughs.
- Updated baked `final_render.png` forest filling to use the same spruce-shaped stamp logic instead of simple triangular/oval canopy blobs.
- Kept the fill as a cheap visual-only pattern pass: no gameplay grids, collision, routes, markers, places, start/goal or world graph data are changed.

## v0.1.37 -> v0.1.38

- Bumped CMake project and runtime application version to `0.1.38`.
- Added use of `assets/visual/forest/reference_v1` forest reference assets.
- Runtime forest preview now draws reference mass PNGs for deep/mid forest instead of procedural spruce stamps when the reference pack is available.
- Baked `final_render.png` now uses the reference forest PNG pack when available and falls back to the older procedural pattern only if the pack is missing.
- Kept gameplay data, collision, routes, markers, places and runtime grids untouched.
## v0.1.38 -> v0.1.39

- Bumped CMake project and runtime application version to `0.1.39`.
- Added `interior_v1` deep forest asset loading for runtime preview.
- Changed deep forest runtime fill to place only opaque interior tiles fully inside `kDeep` forest regions.
- Changed final baked render to use `interior_v1` for deep forest before any edge/reference overlays.
- Stopped using `reference_v1/mass` as deep forest fill when interior assets are available.

## v0.1.39 -> v0.1.40

- Bumped CMake project and runtime application version to `0.1.40`.
- Removed the failed forest asset replacement path from runtime rendering.
- Removed forest PNG asset catalog loading and renderer ownership.
- Removed procedural spruce/pattern forest fill from baked `final_render.png`.
- Removed forest pattern/canopy-only counters and debug artifacts.
- Kept forest depth, edge, clearing, route influence, and mass group analysis.
- Kept gameplay data, collision, routes, markers, places, world graph, and runtime grids untouched.
## v0.1.40 -> v0.1.41

- Bumped CMake project and runtime application version to `0.1.41`.
- Added `--renderer=2d|3d` CLI runtime renderer selection.
- Added an isolated `src/render3d` module for the first 3D map renderer.
- Added 3D player movement driven by `movement_grid`, `collision_grid`, and `height_grid`.
- Added 3D follow camera with Q/E rotation and mouse-wheel distance control.
- Kept the existing 2D renderer path unchanged and skipped the 2D visual pipeline for `--renderer=3d`.

## v0.1.41 -> v0.1.42

- Bumped CMake project and runtime application version to `0.1.42`.
- Added right-mouse drag orbit control for the 3D camera yaw and pitch.
- Added smoothed 3D camera target follow, pitch, yaw, and distance interpolation.
- Added movement-based camera lookahead and target clamping inside map bounds.
- Kept the 2D renderer path unchanged.

## v0.1.42 -> v0.1.43

- Bumped CMake project and runtime application version to `0.1.43`.
- Removed right-mouse orbit camera control from the 3D mode.
- Ported the Python 3D control model: mouse X rotates player facing/aim, W/S move forward/backward, and A/D strafe relative to facing.
- Changed the 3D follow camera to derive position and target from player facing, smoothed lookahead, distance, and height instead of independent orbit yaw/pitch.
- Added mouse capture for the 3D game view and releases it when returning to menus or shutting down.
- Kept the 2D renderer path unchanged.

## v0.1.43 -> v0.1.44

- Added runtime `movement_multiplier` loading from `movement_grid` so 3D movement uses both player base speed and current tile movement cost.
- Applied movement slowdown to passable slow terrain such as water/swamp and passable forest undergrowth while keeping collision-blocked cells impassable.
- Added event-based 3D movement logs on tile boundary crossing with terrain, elevation, movement multiplier, effective speed and accumulated mouse delta since the previous tile.
- Added mouse capture change logs and throttled blocked-movement logs keyed by blocked tile/reason instead of logging every frame.
- Removed the detailed 3D runtime state overlay from the screen; diagnostics now go to logs.

## v0.1.44 -> v0.1.45

- Bumped CMake project and runtime application version to `0.1.45`.
- Replaced verbose 3D tile movement logs with compact `p3d` event lines and short field names.
- Added configurable 3D movement log throttling through `player3d_tile_log_min_interval_ms` and `player3d_block_log_min_interval_ms` in `config/app_config.json`.
- Added `player3d_log_enabled` and `player3d_log_mouse` switches for 3D movement diagnostics.
- Rendered passable forest-boundary / undergrowth tiles as transparent low forest volumes instead of opaque blocking forest blocks.
- Kept dense collision forest opaque and impassable.

## v0.1.45 -> v0.1.46

- Bumped CMake project and runtime application version to `0.1.46`.
- Added `Space + movement direction` step-up action for the 3D player.
- Changed normal 3D movement so elevation `+1` requires an explicit step jump instead of being crossed automatically.
- Kept same-level movement and one-level descent available through normal movement.
- Added short step-jump transition state with smoothed tile movement and a visual vertical arc.
- Added event-based `p3d` logs for `jump_start`, `jump_land`, and `jump_block` without adding any debug HUD.
- Kept the 2D renderer path unchanged.

## v0.1.46 -> v0.1.47

- Bumped CMake project and runtime application version to `0.1.47`.
- Added configurable 3D render culling settings: `render3d_visible_radius_tiles` and `render3d_culling_deadzone_tiles`.
- Stopped shifting the 3D visible tile range on every tile crossing; the range now recenters only after the player leaves a small culling deadzone.
- Reduced default 3D visible radius from 52 to 48 tiles to lower per-frame draw work without changing gameplay data.
- Kept 2D renderer, gameplay grids, collision, markers, routes, and visual pipeline unchanged.

## v0.1.47 -> v0.1.48

- Bumped CMake project and runtime application version to `0.1.48`.
- Added visible 3D elevation side walls between neighboring surface tiles with different `height_grid` values.
- Kept elevated tile surfaces at their runtime elevation and rendered vertical faces only from higher tiles toward lower neighbors.
- Kept underground `-1` cells hidden from the surface renderer.
- Kept 3D movement, step-jump rules, culling deadzone, and the 2D renderer path unchanged.

## v0.1.48 -> v0.1.49

- Bumped CMake project and runtime application version to `0.1.49`.
- Added optional `elevation_transitions.json` loading from the map package or manifest.
- Added runtime elevation transition records with `step`, `ramp`, `stairs`, and `hatch` types.
- Allowed normal 3D movement through explicit ramp/stairs transitions for one-level elevation changes.
- Kept non-transition `+1` height changes gated behind the existing Space step-jump action.
- Kept surface/underground crossing blocked unless an explicit hatch transition exists.
- Added simple 3D transition markers for visible ramp/stairs/hatch/step connections.
- Added event-based `p3d` transition logs without adding any debug HUD.
- Kept the 2D renderer path unchanged.

## v0.1.49 -> v0.1.50

- Bumped CMake project and runtime application version to `0.1.50`.
- Added regular 3D running jump on `Space` while keeping collision-blocked tiles impassable.
- Kept close-range `Space + direction` step-up for elevation `+1`, but running jumps preserve momentum instead of forcing the scripted step-up transition.
- Allowed airborne movement to keep the jump-start terrain movement multiplier and apply a small configurable horizontal speed multiplier.
- Added configurable 3D player movement and jump tuning in `config/app_config.json`.
- Kept `ramp/stairs` elevation transitions, existing collision rules, and the 2D renderer path unchanged.

## v0.1.50 -> v0.1.51

- Bumped CMake project and runtime application version to `0.1.51`.
- Stabilized 3D running jump landing by keeping base elevation stable during air time and interpolating the visual height toward the landing elevation.
- Added a landing validation guard so invalid landing cells snap back safely and emit a jump block event instead of leaving the player inside collision.
- Smoothed movement multiplier changes to reduce sticky speed jumps when crossing water, undergrowth, road, and open-ground borders.
- Tuned default jump values to a shorter, lower, more controllable running jump.
- Added `player3d_movement_multiplier_smooth_speed` to `config/app_config.json`.

## v0.1.51 -> v0.1.52

- Bumped CMake project and runtime application version to `0.1.52`.
- Fixed 3D running-jump landing so normal jumps preserve continuous X/Y position instead of snapping to the center of the landing tile.
- Stored the continuous jump start position and use it for invalid landing rollback instead of recentering on the start tile.
- Kept scripted step-up jumps unchanged; only free/running jump landing behavior was adjusted.

## v0.1.52 -> v0.1.53

- Bumped CMake project and runtime application version to `0.1.53`.
- Added configurable 3D visibility settings for current visible radius, fog-memory, and seen-tile dimming.
- Added chunk-based 3D render iteration with configurable chunk size and active chunk radius.
- Limited 3D drawing to currently visible or previously seen tiles instead of showing the whole loaded map.
- Kept line-of-sight blockers for a later pass; visibility is currently a circular player-centered foundation.
- Kept the 2D renderer path unchanged and added no debug HUD.
## v0.1.53 -> v0.1.54

- Bumped CMake project and runtime application version to `0.1.54`.
- Added Doxygen comments to public runtime, level, input, window, UI, 3D and visual-pipeline data contracts.
- Clarified comments that diagnostic dumps are for logs where screen overlays are not required.
- Removed unused input fields left from the removed right-mouse/orbit-camera path.
- Removed unused Q/E camera-rotation input fields after switching 3D controls to mouse-facing movement.


## v0.1.54 -> v0.1.55

- Bumped CMake project and runtime application version to `0.1.55`.
- Added configurable 3D intro camera fly-in after `New Game`.
- Added smooth camera interpolation from a wide overview pose to the normal follow-camera pose.
- Added optional intro skip with `Space`, `Enter`, or left mouse click.
- Locked 3D player movement during the intro by default so the camera can finish without gameplay drift.
- Added rare camera lifecycle logs for intro start, skip and finish.

## v0.1.55 -> v0.1.56

- Bumped CMake project and runtime application version to `0.1.56`.
- Oriented the initial 3D player facing direction from the spawn point toward the map center.
- Made the 3D intro camera fly-in use that inward-facing direction so starts near corners no longer look out toward the map edge.
- Added one event-style camera log line with spawn tile, map center and computed facing vector.
- Kept 2D rendering, visibility, chunks and player movement rules unchanged.


## v0.1.56 -> v0.1.57

- Bumped CMake project and runtime application version to `0.1.57`.
- Added optional 3D line-of-sight visibility using `RuntimeCell::blocks_vision` / `vision_block_grid` semantics.
- Kept blocker tiles visible while hiding tiles behind earlier blockers on the player-to-tile ray.
- Kept fog memory behavior: currently visible tiles render normally, previously seen tiles render dimmed, and unknown tiles are skipped.
- Added `render3d_los_enabled` to `config/app_config.json` so LoS can be toggled independently from the visibility radius and memory system.
- Avoided recalculating visibility every frame when player tile, radius and visibility settings have not changed.
- Kept the 2D renderer path and debug HUD unchanged.

## v0.1.57 -> v0.1.58

- Bumped CMake project and runtime application version to `0.1.58`.
- Replaced the direct 3D LoS toggle with `render3d_fog_mode`.
- Added two 3D fog-of-war modes: `circle` for classic radius visibility and `raycast` for radius visibility with `vision_block_grid` / `blocks_vision` LoS blocking.
- Kept fog memory behavior unchanged for both modes.
- Preserved backward compatibility with the legacy `render3d_los_enabled` setting when `render3d_fog_mode` is not present.
- Updated README and default config to make `circle` the safe default mode while keeping `raycast` available for experiments.
- Kept the 2D renderer path, HUD and profiler unchanged.


## v0.1.58 -> v0.1.59

- Bumped CMake project and runtime application version to `0.1.59`.
- Added 3D asset registry foundation for future model-based rendering.
- Added `config/render3d/asset_library.json` as the physical model catalog.
- Added `config/render3d/tileset_dark_forest.json` as the active 3D tileset binding file.
- Added deterministic selector support for `fixed`, `random`, `weighted_random`, `named` and `by_tag` model selection.
- Added startup loading and validation of the 3D registry in renderer=3d mode.
- Kept current debug primitives as the fallback path; no visual replacement is enabled yet.

## v0.1.59 -> v0.1.60

- Bumped CMake project and runtime application version to `0.1.60`.
- Added a full Doxygen-oriented documentation pass across headers and implementation files.
- Added file-level comments for C++ source files and headers.
- Documented private application methods, internal helper functions, and public data contracts without changing runtime behavior.
- Added member comments to data-oriented structs where fields are part of the project data contract.

## v0.1.60 -> v0.1.61

- Bumped CMake project and runtime application version to `0.1.61`.
- Added minimal 3D player HP state with configurable initial HP, max HP, and fall damage per unsafe elevation level.
- Allowed downward movement over larger surface elevation drops while keeping collision and surface/underground hatch rules intact.
- Applied fall damage as `(drop_levels - 1) * player3d_fall_damage_per_level`, so a one-level drop is safe and a two-level drop costs 5 HP by default.
- Added compact HP HUD text under the FPS counter in 3D mode.
- Added event logs for unsafe falls and health changes when 3D player logging is enabled.


## v0.1.61 -> v0.1.62

- Bumped CMake project and runtime application version to `0.1.62`.
- Rendered elevation `-1` as a visible lower 3D plane instead of skipping it as hidden underground space.
- Added plain vertical cutaway walls between open pit level `-1` and neighboring elevations `0..4` without extra bright borders or decorative rims.
- Reinterpreted `-1` as an open pit/cutaway level for movement instead of hatch/underground-only space.
- Allowed normal movement/fall from surface elevation into `-1`, with one-level `0 -> -1` drops remaining damage-free.
- Required `Space` step-up/jump to leave `-1` for elevation `0` while keeping normal upward `+2` movement blocked.
- Kept the v0.1.61 fall-damage formula for all downward elevation drops: `(drop_levels - 1) * player3d_fall_damage_per_level`.
- Added regression tests for entering `-1`, fall damage into `-1`, blocked normal exit, and `Space` step-up exit.

## v0.1.62 -> v0.1.63

- Bumped CMake project and runtime application version to `0.1.63`.
- Fixed 3D rendering of walkable ruins floor tiles so they no longer draw blocking volumes.
- Kept actual collision/vision blockers rendered as vertical volumes.
- Fixed the visual shape of elevation `-1` pit/cutaway areas on the elevation playground map: the lower floor now reads as a lower plane instead of a raised plateau.

## v0.1.63 -> v0.1.64

- Bumped CMake project and runtime application version to `0.1.64`.
- Added a temporary 3D elevation debug overlay toggled by `F6`.
- Added temporary event-based 3D elevation movement diagnostics toggled by `F7`.
- Highlighted active 3D elevation debug cells only while the overlay is enabled, keeping the default runtime path disabled.
- Added current-tile and facing-target tile diagnostics for elevation, collision, movement, transition and Space-step checks.


## v0.1.64 -> v0.1.65

- Bumped CMake project and runtime application version to `0.1.65`.
- Improved 3D elevation visual readability without changing movement rules.
- Added muted lower-floor material colors for elevation `-1` so open pits/trenches read as lower playable planes instead of black holes or raised slabs.
- Reworked vertical elevation cut walls to use earth/stone cut colors, with thicker `0 -> -1` pit cut faces and no bright rim decoration.
- Replaced debug-like transition bars with low-profile gameplay primitives: sloped ramp surfaces, stair treads and small Space-step ledge markers.
- Kept the temporary F6/F7 elevation diagnostics from v0.1.64 available and disabled by default.

## v0.1.65 -> v0.1.67

- Bumped CMake project and runtime application version to `0.1.67` for the active mainline branch.
- Added a temporary 3D renderer performance diagnostics overlay toggled by `F8`.
- Collected active chunk counts, active tile candidates, renderable tile count, ground tile submissions, elevation wall faces, blocking volumes, passable forest boundary volumes, transition counts and debug overlay slabs.
- Added last visibility update timing to the F8 overlay while keeping diagnostics disabled by default.
- Kept existing F6/F7 elevation diagnostics unchanged.


## v0.1.67 -> v0.1.68

- Bumped CMake project and runtime application version to `0.1.68`.
- Added horizontal ground-surface batching to the 3D renderer so long runs of identical visible terrain tiles are submitted as one slab instead of one cube per tile.
- Kept elevation walls, blockers, passable forest boundary volumes, transitions, player rendering and debug overlays on their existing immediate-mode path.
- Extended the F8 performance overlay with both batched ground span count and covered ground tile count, making draw-call reduction visible during runtime checks.
- Preserved current elevation mechanics and temporary F6/F7/F8 diagnostics.
