cmake_minimum_required(VERSION 3.27)
project(wgpu-cpp-wasm)

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(ExternalProject)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/EmbedFile.cmake)

include(cmake/glm.cmake)

if(NOT EMSCRIPTEN)
  include(cmake/FetchDawn.cmake)
  find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
  find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)
endif()

include(cmake/FetchSDL2WebGPU.cmake)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

add_subdirectory(src)
