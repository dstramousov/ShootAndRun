# ShootAndRunCpp v0.1.60

Первый каркас C++20 + raylib проекта.

## Что есть

- CMake-проект.
- raylib window bootstrap.
- Главное меню.
- Confirm dialog для выхода.
- FPS справа сверху.
- Debug overlay.
- Нормализованный input layer.
- Logger с уровнями, PID, TID, thread name и цветным выводом.
- Чистая логика расчёта окна и `ui_scale`.
- Конфигурационный файл `config/app_config.json`.
- Конфигурируемый UI-шрифт из runtime assets.
- Базовая валидация `TopDownMapGen` map package.

- Debug renderer for loaded terrain maps.
- Separate 3D renderer mode selected from CLI with `--renderer=3d`.
- 3D elevation readability: elevated tiles are drawn at `height_grid` level with visible side walls.
- 3D fog-of-war modes: classic circle visibility, optional raycast LoS visibility, fog memory for previously seen tiles, and chunk-based render iteration.
- Debug marker overlay for `markers.json`, including player spawn markers.
- 3D player-facing follow camera: mouse X turns player aim/facing, WASD moves relative to facing, and mouse wheel changes camera distance.
- 3D new-game intro orients the player and camera toward the map center before the fly-in.
- Базовые структуры `level/` под будущий renderer/gameplay.
- Минимальные unit-тесты без внешнего test framework.

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/SaR
```

Если `raylib` не установлен локально, CMake подтянет его через `FetchContent`
в `.deps/`. Это позволяет удалять `build/` без повторного скачивания raylib.

## Тесты

```bash
ctest --test-dir build --output-on-failure
```

## CLI

Пока поддерживается минимальный набор:

```bash
./build/SaR --log-level=debug
./build/SaR --log-level=trace --no-color
./build/SaR --config=config/app_config.json
./build/SaR --renderer=2d
./build/SaR --renderer=3d
```

## Конфигурация

Минимальный конфиг проекта лежит в `config/app_config.json`:

```json
{
  "map_package_path": "../TopDownMapGen/output/map_package",
  "ui_font_path": "data/fonts/PressStart2P-Regular.ttf",
  "ui_font_size": 16,
  "window": {
    "preferred_width": 1600,
    "preferred_height": 900,
    "fallback_width": 1280,
    "fallback_height": 720,
    "max_monitor_fraction": 0.9,
    "resizable": true
  }
}
```

При старте приложение читает конфиг, применяет настройки окна и пытается
загрузить UI-шрифт. Если шрифт не найден или не загрузился, приложение пишет
предупреждение в лог и использует fallback на стандартный raylib font.

При нажатии `New Game` приложение берёт `map_package_path` из конфига,
проверяет каталог, читает `map.json` manifest, `terrain.json`,
`runtime_grids.json` и опциональный `markers.json`, валидирует размеры базовых
grid-слоёв и только после этого переходит в game screen.

В режиме `--renderer=2d` game screen отображает карту в debug-режиме. Если в `markers.json` есть
`player_spawn` или другой spawn-маркер, камера стартует с него; иначе камера
центрируется по карте. Управление 2D-камерой:

```text
WASD / Arrows  - двигать камеру
Mouse wheel    - zoom in/out
Esc            - вернуться в главное меню
```

В режиме `--renderer=3d` используется отдельный 3D renderer. Он читает тот же
`map_package`, рисует terrain/collision/height_grid в 3D и двигает игрока по
`movement_grid`/`collision_grid` без запуска 2D visual pipeline. В 3D включён fog-of-war: режим `circle` даёт классический круг видимости вокруг игрока, режим `raycast` дополнительно режет обзор через `vision_block_grid`/`blocks_vision`.

```text
WASD / Arrows  - W/S вперёд/назад, A/D стрейф относительно взгляда
Mouse X        - поворот взгляда / прицела игрока
Space + move   - заскок на соседний elevation +1
Mouse wheel    - zoom in/out
F1             - terrain colors
F2             - elevation debug
F3             - collision debug
Esc            - вернуться в главное меню
```

Режим fog-of-war выбирается в `config/app_config.json`:

```json
"render3d_visibility": {
  "render3d_visibility_enabled": true,
  "render3d_visibility_radius_tiles": 22,
  "render3d_visibility_memory_enabled": true,
  "render3d_fog_mode": "circle",
  "render3d_seen_tile_dim_factor": 0.32
}
```

Доступные значения `render3d_fog_mode`:

```text
circle   - классический радиус видимости без LoS-блокеров
raycast  - радиус видимости плюс raycast по vision blockers
```

## 3D asset registry

Для будущих GLB/OBJ-моделей добавлен отдельный слой регистрации 3D-ассетов.
Сейчас он не меняет внешний вид сцены: если модель не подключена или не найдена,
renderer продолжает использовать текущие debug primitives.

Файлы конфигурации:

```text
config/render3d/asset_library.json       - общий каталог физических моделей
config/render3d/tileset_dark_forest.json - активный 3D tileset / theme bindings
```

Минимальный пример модели и binding:

```json
{
  "models": {
    "tree_pine_01": {
      "path": "assets/models/trees/tree_pine_01.glb",
      "tags": ["tree", "pine", "forest"],
      "default_scale": 1.0,
      "vertical_offset": 0.0,
      "fallback": "tree_debug"
    }
  }
}
```

```json
{
  "tileset_id": "dark_forest_3d",
  "bindings": {
    "forest_dense": {
      "placement": "single",
      "selector": "weighted_random",
      "variants": [
        { "model": "tree_pine_01", "weight": 5 }
      ],
      "random_rotation": true,
      "scale_range": [0.9, 1.15],
      "offset_range": [-0.15, 0.15],
      "fallback": "tree_debug"
    }
  }
}
```

Поддерживаемые selector-режимы фундамента: `fixed`, `random`,
`weighted_random`, `named`, `by_tag`. Выбор random/weighted_random
детерминированный: seed + semantic key + координаты tile дают один и тот же
результат между запусками.
