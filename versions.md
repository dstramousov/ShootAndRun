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
