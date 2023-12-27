embed_files(embedded_files ./triangle.wgsl)

add_executable(wgpu_app main.cpp arcball_camera.cpp)

set_target_properties(wgpu_app PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED
                                                          ON)

target_link_libraries(wgpu_app PRIVATE glm embedded_files)

target_compile_options(wgpu_app PRIVATE -pthread)

target_link_options(
  wgpu_app
  PRIVATE
  "SHELL: -sENVIRONMENT='web,worker'"
  -pthread
  -sPTHREAD_POOL_SIZE=1
  -sEXPORT_ES6
  -sEXPORT_NAME=WGPUApp
  -sUSE_WEBGPU=1
  -sMODULARIZE=1
  -sEXPORT_ES6
  -sEXPORT_NAME=CPPApp
  -sALLOW_MEMORY_GROWTH=1
  -sASSERTIONS=1
  -sEXIT_RUNTIME=1
  -sINVOKE_RUN=0
  # Debug build flags
  $<$<CONFIG:DEBUG>:-fwasm-exceptions>
  $<$<CONFIG:DEBUG>:-g>
  $<$<CONFIG:DEBUG>:-O0>
  # Exported Emscripten runtime methods
  "SHELL:-sEXPORTED_RUNTIME_METHODS='[\"callMain\"]'")

option(ENABLE_SOURCE_MAP
       "Enable source map for debug builds (breaks Chrome DWARF debugging)" OFF)
if(ENABLE_SOURCE_MAP)
  target_link_options(wgpu_app PRIVATE $<$<CONFIG:DEBUG>:-gsource-map>)
endif()

option(ENABLE_MEMORY_PROFILER "Enable Emscripten's memory profiler" OFF)
if(ENABLE_MEMORY_PROFILER)
  target_link_options(wgpu_app PRIVATE --memoryprofiler)
endif()

option(ENABLE_CPU_PROFILER "Enable Emscripten's CPU profiler" OFF)
if(ENABLE_CPU_PROFILER)
  target_link_options(wgpu_app PRIVATE --cpuprofiler)
endif()

option(ENABLE_THREAD_PROFILER "Enable Emscripten's thread profiler" OFF)
if(ENABLE_THREAD_PROFILER)
  target_link_options(wgpu_app PRIVATE --threadprofiler)
endif()

if(ENABLE_MEMORY_PROFILER
   OR ENABLE_CPU_PROFILER
   OR ENABLE_THREAD_PROFILER)
  target_compile_options(wgpu_app PRIVATE $<$<CONFIG:RELEASE>:--profiling>)
  target_link_options(wgpu_app PRIVATE $<$<CONFIG:RELEASE>:--profiling>)
endif()

set(WEB_OUT_DIR ${PROJECT_SOURCE_DIR}/web/src/cpp)

set(COPY_SRC_MAP $<AND:$<BOOL:${ENABLE_SOURCE_MAP}>,$<CONFIG:DEBUG>>)

add_custom_command(
  DEPENDS wgpu_app
  OUTPUT ${WEB_OUT_DIR}/wgpu_app.js ${WEB_OUT_DIR}/wgpu_app.worker.js
         ${WEB_OUT_DIR}/wgpu_app.wasm
         $<${COPY_SRC_MAP}:${WEB_OUT_DIR}/wgpu_app.wasm.map>
  COMMAND cmake -E make_directory ${WEB_OUT_DIR}
  COMMAND
    cmake -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.js
    ${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.worker.js
    ${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.wasm
    $<${COPY_SRC_MAP}:${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.wasm.map>
    ${WEB_OUT_DIR})

add_custom_target(
  copy_wasm_to_app ALL
  DEPENDS ${WEB_OUT_DIR}/wgpu_app.js ${WEB_OUT_DIR}/wgpu_app.worker.js
          ${WEB_OUT_DIR}/wgpu_app.wasm
          $<${COPY_SRC_MAP}:${WEB_OUT_DIR}/wgpu_app.wasm.map>
  COMMENT "Copying wasm build to ${WEB_OUT_DIR}")
