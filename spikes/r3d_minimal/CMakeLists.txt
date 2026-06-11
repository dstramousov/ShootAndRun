cmake_minimum_required(VERSION 3.20)

project(SarR3DMinimal
  VERSION 0.2.0
  DESCRIPTION "Standalone R3D forest/map slice spike for ShootAndRun"
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SAR_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(SAR_R3D_ROOT "${SAR_PROJECT_ROOT}/external/r3d")

set(R3D_RAYLIB_VENDORED ON CACHE BOOL "Use R3D vendored raylib" FORCE)
set(R3D_SUPPORT_ASSIMP ON CACHE BOOL "Enable Assimp so the spike can load ShootAndRun GLB tree models" FORCE)
set(R3D_ASSIMP_VENDORED ON CACHE BOOL "Use R3D vendored Assimp for deterministic GLB loading" FORCE)
set(R3D_BUILD_EXAMPLES OFF CACHE BOOL "Do not build upstream R3D examples" FORCE)
set(R3D_BUILD_DOCS OFF CACHE BOOL "Do not build upstream R3D docs" FORCE)

if(NOT EXISTS "${SAR_R3D_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR
    "R3D submodule was not found at ${SAR_R3D_ROOT}.\n"
    "Run from the ShootAndRun repository root:\n"
    "  git submodule update --init --recursive external/r3d")
endif()

if(NOT EXISTS "${SAR_R3D_ROOT}/external/assimp/CMakeLists.txt")
  message(FATAL_ERROR
    "R3D vendored Assimp was not found.\n"
    "Run from the ShootAndRun repository root:\n"
    "  git submodule update --init --recursive external/r3d")
endif()

add_subdirectory("${SAR_R3D_ROOT}" "${CMAKE_BINARY_DIR}/r3d")

# Work around missing standard C includes in the pinned R3D pre-release sources.
# Keep this in the isolated spike instead of patching files inside external/r3d.
if(TARGET r3d AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(r3d PRIVATE "SHELL:-include stddef.h" "SHELL:-include string.h")
endif()

add_executable(sar_r3d_minimal
  src/main.cpp
  "${SAR_PROJECT_ROOT}/src/level/level_loader.cpp"
  "${SAR_PROJECT_ROOT}/src/level/terrain_type.cpp"
  "${SAR_PROJECT_ROOT}/src/level/elevation_transition.cpp"
  "${SAR_PROJECT_ROOT}/src/render3d/model_registry.cpp"
)

target_include_directories(sar_r3d_minimal PRIVATE
  "${SAR_PROJECT_ROOT}/src"
  "${SAR_R3D_ROOT}/include"
  "${SAR_R3D_ROOT}/external/raylib/src"
)

target_compile_definitions(sar_r3d_minimal PRIVATE
  SAR_PROJECT_ROOT="${SAR_PROJECT_ROOT}"
)

target_link_libraries(sar_r3d_minimal PRIVATE
  r3d
  raylib
)

target_compile_options(sar_r3d_minimal PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion>
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
)

if(UNIX AND NOT APPLE)
  target_link_libraries(sar_r3d_minimal PRIVATE m pthread dl)
endif()
