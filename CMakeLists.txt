cmake_minimum_required(VERSION 3.27)
project(wgpu-cpp-wasm)

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

if(EMSCRIPTEN)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")
  include(ExternalProject)

  include(cmake/glm.cmake)

  add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

  add_subdirectory(src)
else()
  add_subdirectory(ws_server)
endif()
