cmake_minimum_required(VERSION 3.24)
project(shadow_asset_loader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(shadow_assets
  src/FileIO.cpp
  src/gc/Prs.cpp
  src/gc/OneArchive.cpp
  src/gc/Fst.cpp
  ../renderware/platform/gamecube/RenderWare.cpp
  src/gc/Motion.cpp
  src/gc/Containers.cpp
  src/pc/Decode.cpp
  src/pc/Animation.cpp
  src/pc/Skinning.cpp
  src/pc/ResourceSystem.cpp)
target_include_directories(shadow_assets PUBLIC include ../renderware/include)
if(MSVC)
  target_compile_options(shadow_assets PRIVATE /W4 /permissive-)
else()
  target_compile_options(shadow_assets PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(asset_tests tests/test_loader.cpp)
target_link_libraries(asset_tests PRIVATE shadow_assets)
if(MSVC)
  target_compile_options(asset_tests PRIVATE /W4 /permissive-)
else()
  target_compile_options(asset_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(SHADOW_CONTENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../files" CACHE PATH
    "Path to the user's extracted game files (never committed)")
set(SHADOW_FST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../sys/fst.bin" CACHE FILEPATH
    "Path to the user's extracted FST (never committed)")
target_compile_definitions(asset_tests PRIVATE
  SHADOW_CONTENT_DIR="${SHADOW_CONTENT_DIR}"
  SHADOW_FST_PATH="${SHADOW_FST_PATH}")

enable_testing()
if(EXISTS "${SHADOW_CONTENT_DIR}/nukkoro2.inf" AND EXISTS "${SHADOW_FST_PATH}")
  add_test(NAME asset_loader COMMAND asset_tests)
endif()
