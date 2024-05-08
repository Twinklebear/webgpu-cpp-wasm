#pragma once

#include <SDL.h>
#include <webgpu/webgpu.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! Get a WGPUSurface from an SDL2 window. In Emscripten builds, your canvas
 * should have id="canvas"
 */
WGPUSurface sdl2GetWGPUSurface(WGPUInstance instance, SDL_Window *window);

#ifdef __cplusplus
}
#endif
