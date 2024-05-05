#include <iostream>
#include <SDL.h>
#include "arcball_camera.h"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include "embedded_files.h"

struct AppState {
    ArcballCamera camera;
    glm::mat4 proj;

    bool done = false;
    bool camera_changed = true;
    glm::vec2 prev_mouse = glm::vec2(-2.f);
};

uint32_t win_width = 1280;
uint32_t win_height = 720;

void app_loop(void *_app_state);

int main(int argc, const char **argv)
{
    AppState *app_state = new AppState;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << "\n";
        return -1;
    }
    std::cout << "SDL2 initialized\n";

    SDL_Window *window = SDL_CreateWindow("SDL2 + WebGPU",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          win_width,
                                          win_height,
                                          0);
    std::cout << "SDL window = " << window << "\n";

    // TODO: set up webgpu device

#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(app_loop, app_state, -1, 0);
#else
    while (!app_state->done) {
        app_loop(app_state);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    return 0;
}

void app_loop(void *_app_state)
{
    AppState *app_state = reinterpret_cast<AppState *>(_app_state);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            app_state->done = true;
        }
    }
}
