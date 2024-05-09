embed_files(embedded_files ./triangle.wgsl)

add_library(sdl2webgpu STATIC sdl2webgpu.c)

if(NOT EMSCRIPTEN)
  target_link_libraries(sdl2webgpu PUBLIC SDL2::SDL2 webgpu)
  if(APPLE)
    target_compile_options(sdl2webgpu PRIVATE -x objective-c)
    target_link_libraries(
      sdl2webgpu PRIVATE "-framework Cocoa" "-framework CoreVideo"
                         "-framework IOKit" "-framework QuartzCore")
  endif()
elseif(EMSCRIPTEN)
  target_compile_options(sdl2webgpu PUBLIC --use-port=sdl2)
  target_link_options(sdl2webgpu PUBLIC -sUSE_SDL=2)
endif()

add_executable(wgpu_app main.cpp arcball_camera.cpp)

set_target_properties(wgpu_app PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED
                                                          ON)

target_link_libraries(wgpu_app PRIVATE glm embedded_files sdl2webgpu)

if(EMSCRIPTEN)
  target_compile_options(wgpu_app PRIVATE --use-port=sdl2)

  target_link_options(
    wgpu_app
    PRIVATE
    "SHELL: -sENVIRONMENT='web,worker'"
    -sEXPORT_ES6
    -sEXPORT_NAME=WGPUApp
    -sUSE_WEBGPU=1
    -sMODULARIZE=1
    -sALLOW_MEMORY_GROWTH=1
    -sASSERTIONS=1
    -sEXIT_RUNTIME=1
    -sINVOKE_RUN=0
    -sUSE_SDL=2
    # Debug build flags
    $<$<CONFIG:DEBUG>:-fwasm-exceptions>
    $<$<CONFIG:DEBUG>:-g>
    $<$<CONFIG:DEBUG>:-O0>
    # Exported Emscripten runtime methods
    "SHELL:-sEXPORTED_RUNTIME_METHODS='[\"callMain\"]'")

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

  add_custom_command(
    DEPENDS wgpu_app
    OUTPUT ${WEB_OUT_DIR}/wgpu_app.js ${WEB_OUT_DIR}/wgpu_app.wasm
    COMMAND cmake -E make_directory ${WEB_OUT_DIR}
    COMMAND cmake -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.js
            ${CMAKE_CURRENT_BINARY_DIR}/wgpu_app.wasm ${WEB_OUT_DIR})

  add_custom_target(
    copy_wasm_to_app ALL
    DEPENDS ${WEB_OUT_DIR}/wgpu_app.js ${WEB_OUT_DIR}/wgpu_app.wasm
    COMMENT "Copying wasm build to ${WEB_OUT_DIR}")
else()
  if(TARGET SDL2::SDL2main)
    target_link_libraries(wgpu_app PUBLIC SDL2::SDL2main)
  endif()
  target_link_libraries(wgpu_app PUBLIC SDL2::SDL2 webgpu)
endif()
