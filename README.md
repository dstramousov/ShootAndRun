# ShootAndRunCpp v0.1.10

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
- Free camera for map inspection with WASD/arrows and mouse-wheel zoom.
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
проверяет каталог, читает `map.json` manifest, `terrain.json` и
`runtime_grids.json`, валидирует размеры базовых grid-слоёв и только после этого
переходит в game screen.

В game screen карта отображается в debug-режиме. Управление камерой:

```text
WASD / Arrows  - двигать камеру
Mouse wheel    - zoom in/out
Esc            - вернуться в главное меню
```
