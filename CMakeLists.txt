cmake_minimum_required(VERSION 3.27)
project(wgpu-cpp-wasm)

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(ExternalProject)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/EmbedFile.cmake)

include(cmake/glm.cmake)

if (NOT EMSCRIPTEN)
  include(cmake/FetchDawn.cmake)
endif()

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

# TODO building the source code
if (EMSCRIPTEN)
  add_subdirectory(src)
endif()
