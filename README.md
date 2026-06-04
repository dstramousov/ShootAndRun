# ShootAndRunCpp v0.1.4

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
- Базовые структуры `level/` под будущий `TopDownMapGen` output package.
- Конфигурационный файл `config/app_config.json` с путём к map package.
- Минимальные unit-тесты без внешнего test framework.

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/shoot_and_run
```

Если `raylib` не установлен локально, CMake подтянет его через `FetchContent` в `.deps/`. Это позволяет удалять `build/` без повторного скачивания raylib.

## Тесты

```bash
ctest --test-dir build --output-on-failure
```

## CLI

Пока поддерживается минимальный набор:

```bash
./build/shoot_and_run --log-level=debug
./build/shoot_and_run --log-level=trace --no-color
./build/shoot_and_run --config=config/app_config.json
```

## Конфигурация

Минимальный конфиг проекта лежит в `config/app_config.json`:

```json
{
  "map_package_path": "data/maps/sample_level"
}
```

При нажатии `New Game` приложение читает этот файл, получает путь к
map package и проверяет, что такой каталог существует.
