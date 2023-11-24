add_executable(wgpu_app main.cpp arcball_camera.cpp)

set_target_properties(wgpu_app PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED
                                                          ON)

target_link_libraries(wgpu_app PRIVATE glm)

target_link_options(
  wgpu_app
  PRIVATE
  -sUSE_WEBGPU=1
  -sMODULARIZE=1
  -sALLOW_MEMORY_GROWTH=1
  -sASSERTIONS=1
  -sEXIT_RUNTIME=1
  -sINVOKE_RUN=0
  # Debug build flags
  $<$<CONFIG:DEBUG>:-fwasm-exceptions>
  $<$<CONFIG:DEBUG>:-g>
  $<$<CONFIG:DEBUG>:-gsource-map>
  $<$<CONFIG:DEBUG>:-O0>
  # Exported Emscripten runtime methods
  "SHELL:-sEXPORTED_RUNTIME_METHODS='[\"callMain\"]'")

set(WEB_OUT_DIR ${PROJECT_SOURCE_DIR}/web/src/cpp)

add_custom_command(
  DEPENDS wgpu_app
  OUTPUT ${WEB_OUT_DIR}/wgpu_app.js ${WEB_OUT_DIR}/wgpu_app.wasm
         $<$<CONFIG:DEBUG>:${WEB_OUT_DIR}/wgpu_app.wasm.map>
  COMMAND cmake -E make_directory ${WEB_OUT_DIR}
  COMMAND
    cmake -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.js
    ${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.wasm
    $<$<CONFIG:DEBUG>:${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.wasm.map>
    ${WEB_OUT_DIR})

add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/web/dbg
  # For debug builds make a symlink from web/dbg -> src so that Chrome's
  # devtools can find our C++ files
  COMMAND cmake -E create_symlink ${CMAKE_CURRENT_LIST_DIR}
          ${PROJECT_SOURCE_DIR}/web/dbg)

add_custom_target(
  copy_wasm_to_app ALL
  DEPENDS ${WEB_OUT_DIR}/wgpu_app.js ${WEB_OUT_DIR}/wgpu_app.wasm
          $<$<CONFIG:DEBUG>:${WEB_OUT_DIR}/wgpu_app.wasm.map>
          $<$<CONFIG:DEBUG>:${PROJECT_SOURCE_DIR}/web/dbg>
  COMMENT "Copying wasm build to ${WEB_OUT_DIR}")
