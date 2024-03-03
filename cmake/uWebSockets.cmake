include(ExternalProject)
find_program(MAKE_EXE NAMES gmake nmake make)

ExternalProject_Add(
  uWebSocket_ext
  PREFIX uWebSocket
  DOWNLOAD_DIR uWebSocket
  STAMP_DIR uWebSocket/stamp
  SOURCE_DIR uWebSocket/src
  GIT_REPOSITORY git@github.com:uNetworking/uWebSockets.git
  GIT_TAG v20.60.0
  GIT_SHALLOW ON
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND ""
  # uWebSocket builds in source Turn off zlib for now too, don't know if WS on
  # browser will automatically handle decompressing data over the websocket. We
  # skip building the examples by just building the non-target capi why is the
  # uWebSockets build system like this!?
  BUILD_COMMAND WITH_ZLIB=0 ${MAKE_EXE} capi
  BUILD_IN_SOURCE ON)

# Get the uSocket static lib built as part of uWebSocket
ExternalProject_Get_Property(uWebSocket_ext SOURCE_DIR)
message("usocket source dir = ${SOURCE_DIR}")
set(UWEBSOCKET_SRC "${SOURCE_DIR}/")
set(USOCKET_LIB "${UWEBSOCKET_SRC}/uSockets/uSockets.a")

add_library(uWebSocket INTERFACE)

add_dependencies(uWebSocket uWebSocket_ext)

target_include_directories(uWebSocket INTERFACE ${SOURCE_DIR}/src/
                                                ${SOURCE_DIR}/uSockets/src/)

target_link_libraries(uWebSocket INTERFACE ${USOCKET_LIB})

target_compile_options(uWebSocket INTERFACE -DLIBUS_NO_SSL=1 -DUWS_NO_ZLIB=1)
