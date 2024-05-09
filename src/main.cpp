#include <iostream>
#include <stdexcept>
#include <SDL.h>
#include "arcball_camera.h"
#include "sdl2webgpu.h"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include <webgpu/webgpu_cpp.h>

#include "embedded_files.h"

struct AppState {
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue queue;

    wgpu::Surface surface;
    wgpu::SwapChain swap_chain;
    wgpu::RenderPipeline render_pipeline;

    ArcballCamera camera;
    glm::mat4 proj;

    bool done = false;
    bool camera_changed = true;
    glm::vec2 prev_mouse = glm::vec2(-2.f);
};

uint32_t win_width = 1280;
uint32_t win_height = 720;

void app_loop(void *_app_state);

#ifndef __EMSCRIPTEN__
// TODO: move to another file
wgpu::Adapter request_adapter(wgpu::Instance &instance,
                              const wgpu::RequestAdapterOptions &options)
{
    struct Result {
        WGPUAdapter adapter = nullptr;
        bool success = false;
    };

    Result result;
    instance.RequestAdapter(
        &options,
        [](WGPURequestAdapterStatus status,
           WGPUAdapter adapter,
           const char *msg,
           void *user_data) {
            Result *res = reinterpret_cast<Result *>(user_data);
            if (status == WGPURequestAdapterStatus_Success) {
                res->adapter = adapter;
                res->success = true;
            } else {
                std::cerr << "Failed to get WebGPU adapter: " << msg << std::endl;
            }
        },
        &result);

    if (!result.success) {
        throw std::runtime_error("Failed to get WebGPU adapter");
    }

    return wgpu::Adapter::Acquire(result.adapter);
}

wgpu::Device request_device(wgpu::Adapter &adapter, const wgpu::DeviceDescriptor &options)
{
    struct Result {
        WGPUDevice device = nullptr;
        bool success = false;
    };

    Result result;
    adapter.RequestDevice(
        &options,
        [](WGPURequestDeviceStatus status,
           WGPUDevice device,
           const char *msg,
           void *user_data) {
            Result *res = reinterpret_cast<Result *>(user_data);
            if (status == WGPURequestDeviceStatus_Success) {
                res->device = device;
                res->success = true;
            } else {
                std::cerr << "Failed to get WebGPU device: " << msg << std::endl;
            }
        },
        &result);

    if (!result.success) {
        throw std::runtime_error("Failed to get WebGPU device");
    }

    return wgpu::Device::Acquire(result.device);
}
#endif

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

    app_state->instance = wgpu::CreateInstance();
    app_state->surface =
        wgpu::Surface::Acquire(sdl2GetWGPUSurface(app_state->instance.Get(), window));

#ifdef EMSCRIPTEN
    // The adapter/device request has already been done for us in the TypeScript code
    // when running in Emscripten
    app_state->device = wgpu::Device::Acquire(emscripten_webgpu_get_device());
#else
    wgpu::RequestAdapterOptions adapter_options = {};
    adapter_options.compatibleSurface = app_state->surface;
    adapter_options.powerPreference = wgpu::PowerPreference::HighPerformance;
    app_state->adapter = request_adapter(app_state->instance, adapter_options);

    wgpu::DeviceDescriptor device_options = {};
    app_state->device = request_device(app_state->adapter, device_options);
#endif

    app_state->device.SetUncapturedErrorCallback(
        [](WGPUErrorType type, const char *msg, void *data) {
            std::cout << "WebGPU Error: " << msg << "\n" << std::flush;
#ifdef EMSCRIPTEN
            emscripten_cancel_main_loop();
            emscripten_force_exit(1);
#endif
            std::exit(1);
        },
        nullptr);

    app_state->queue = app_state->device.GetQueue();

    wgpu::SwapChainDescriptor swap_chain_desc;
    swap_chain_desc.format = wgpu::TextureFormat::BGRA8Unorm;
    swap_chain_desc.usage = wgpu::TextureUsage::RenderAttachment;
    swap_chain_desc.presentMode = wgpu::PresentMode::Fifo;
    swap_chain_desc.width = win_width;
    swap_chain_desc.height = win_height;

    app_state->swap_chain =
        app_state->device.CreateSwapChain(app_state->surface, &swap_chain_desc);

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

    wgpu::RenderPassColorAttachment color_attachment;
    color_attachment.view = app_state->swap_chain.GetCurrentTextureView();
    color_attachment.clearValue.r = 0.f;
    color_attachment.clearValue.g = 0.f;
    color_attachment.clearValue.b = 1.f;
    color_attachment.clearValue.a = 1.f;
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor pass_desc;
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &color_attachment;

    wgpu::CommandEncoder encoder = app_state->device.CreateCommandEncoder();

    // Just clear the image
    wgpu::RenderPassEncoder render_pass_enc = encoder.BeginRenderPass(&pass_desc);
    render_pass_enc.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    app_state->queue.Submit(1, &commands);

#ifndef __EMSCRIPTEN__
    app_state->swap_chain.Present();
#endif
}
