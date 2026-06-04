cmake_minimum_required(VERSION 3.20)

project(ShootAndRun
  VERSION 0.1.10
  DESCRIPTION "ShootAndRun C++ raylib bootstrap"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SAR_BUILD_TESTS "Build project unit tests" ON)
option(SAR_FETCH_RAYLIB "Fetch raylib when it is not installed locally" ON)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.deps" CACHE PATH
    "Directory used by CMake FetchContent for downloaded dependencies")

include(cmake/dependencies.cmake)

add_executable(shoot_and_run
  src/main.cpp
  src/app/application.cpp
  src/app/project_config.cpp
  src/game/game_session.cpp
  src/input/input_system.cpp
  src/level/level_loader.cpp
  src/level/terrain_type.cpp
  src/logging/logger.cpp
  src/logging/log_level.cpp
  src/logging/thread_context.cpp
  src/platform/memory_info.cpp
  src/platform/process_info.cpp
  src/platform/terminal.cpp
  src/render/debug_overlay.cpp
  src/render/level_renderer.cpp
  src/render/menu_renderer.cpp
  src/render/renderer.cpp
  src/render/ui_font.cpp
  src/ui/confirm_dialog.cpp
  src/ui/main_menu.cpp
  src/ui/ui_layout.cpp
  src/window/window_layout.cpp
)

set_target_properties(shoot_and_run PROPERTIES
  OUTPUT_NAME SaR
)

target_include_directories(shoot_and_run PRIVATE src)
target_compile_definitions(shoot_and_run PRIVATE
  SAR_APP_VERSION="${PROJECT_VERSION}"
)
target_link_libraries(shoot_and_run PRIVATE raylib)

target_compile_options(shoot_and_run PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

if(UNIX AND NOT APPLE)
  target_link_libraries(shoot_and_run PRIVATE m pthread dl)
endif()

if(SAR_BUILD_TESTS)
  enable_testing()
  add_executable(shoot_and_run_tests
    tests/test_main.cpp
    src/app/project_config.cpp
    src/level/level_loader.cpp
    src/level/terrain_type.cpp
    src/ui/main_menu.cpp
    src/window/window_layout.cpp
  )

  target_include_directories(shoot_and_run_tests PRIVATE src)
  target_compile_options(shoot_and_run_tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
  )

  add_test(NAME shoot_and_run_tests COMMAND shoot_and_run_tests)
endif()
