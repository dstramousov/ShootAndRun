find_package(raylib QUIET)

if(NOT raylib_FOUND)
  if(NOT SAR_FETCH_RAYLIB)
    message(FATAL_ERROR "raylib was not found. Install raylib or enable SAR_FETCH_RAYLIB.")
  endif()

  include(FetchContent)
  message(STATUS "raylib will be fetched into: ${FETCHCONTENT_BASE_DIR}")
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
  )
  FetchContent_MakeAvailable(raylib)
endif()
