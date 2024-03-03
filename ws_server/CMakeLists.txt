cmake_minimum_required(VERSION 3.27)

include(../cmake/uWebSockets.cmake)

add_executable(ws_server ws_server.cpp)

set_target_properties(ws_server PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED
                                                           ON)

target_link_libraries(ws_server PUBLIC uWebSocket)
