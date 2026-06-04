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
