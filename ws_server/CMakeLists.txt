cmake_minimum_required(VERSION 3.27)

find_package(oatpp 1.3.0 REQUIRED)
find_package(oatpp-websocket 1.3.0 REQUIRED)

add_executable(ws_server ws_server.cpp oatpp_ws_listener.cpp)

set_target_properties(ws_server PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED
                                                           ON)

target_link_libraries(ws_server PUBLIC oatpp::oatpp oatpp::oatpp-test
                                       oatpp::oatpp-websocket)
