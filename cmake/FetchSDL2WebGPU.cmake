include(FetchContent)

FetchContent_Declare(
  sdl2webgpu
  GIT_REPOSITORY https://github.com/Twinklebear/wgpu-cpp-starter.git
  GIT_TAG main)

FetchContent_MakeAvailable(sdl2webgpu)
