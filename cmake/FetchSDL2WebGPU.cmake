include(FetchContent)

FetchContent_Declare(
  sdl2webgpu
  GIT_REPOSITORY https://github.com/Twinklebear/sdl2webgpu.git
  GIT_TAG claude/update-surface-api)

FetchContent_MakeAvailable(sdl2webgpu)

if(EMSCRIPTEN)
  target_compile_options(sdl2webgpu PUBLIC --use-port=emdawnwebgpu)
endif()
