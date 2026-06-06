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
