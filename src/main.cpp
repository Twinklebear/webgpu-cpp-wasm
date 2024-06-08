#include <array>
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

    wgpu::Buffer vertex_buf;
    wgpu::Buffer view_param_buf;
    wgpu::BindGroup bind_group;

    ArcballCamera camera;
    glm::mat4 proj;

    bool done = false;
    bool camera_changed = true;
    glm::vec2 prev_mouse = glm::vec2(-2.f);
};

uint32_t win_width = 1280;
uint32_t win_height = 720;

glm::vec2 transform_mouse(glm::vec2 in)
{
    return glm::vec2(in.x * 2.f / win_width - 1.f, 1.f - 2.f * in.y / win_height);
}

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

    SDL_Window *window = SDL_CreateWindow("SDL2 + WebGPU",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          win_width,
                                          win_height,
                                          0);

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

    wgpu::ShaderModule shader_module;
    {
        wgpu::ShaderModuleWGSLDescriptor shader_module_wgsl;
        shader_module_wgsl.code = reinterpret_cast<const char *>(triangle_wgsl);

        wgpu::ShaderModuleDescriptor shader_module_desc;
        shader_module_desc.nextInChain = &shader_module_wgsl;
        shader_module = app_state->device.CreateShaderModule(&shader_module_desc);

        shader_module.GetCompilationInfo(
            [](WGPUCompilationInfoRequestStatus status,
               WGPUCompilationInfo const *info,
               void *) {
                if (info->messageCount != 0) {
                    std::cout << "Shader compilation info:\n";
                    for (uint32_t i = 0; i < info->messageCount; ++i) {
                        const auto &m = info->messages[i];
                        std::cout << m.lineNum << ":" << m.linePos << ": ";
                        switch (m.type) {
                        case WGPUCompilationMessageType_Error:
                            std::cout << "error";
                            break;
                        case WGPUCompilationMessageType_Warning:
                            std::cout << "warning";
                            break;
                        case WGPUCompilationMessageType_Info:
                            std::cout << "info";
                            break;
                        default:
                            break;
                        }

                        std::cout << ": " << m.message << "\n";
                    }
                }
            },
            nullptr);
    }

    // Upload vertex data
    const std::vector<float> vertex_data = {
        1,  -1, 0, 1,  // position
        1,  0,  0, 1,  // color
        -1, -1, 0, 1,  // position
        0,  1,  0, 1,  // color
        0,  1,  0, 1,  // position
        0,  0,  1, 1,  // color
    };
    wgpu::BufferDescriptor buffer_desc;
    buffer_desc.mappedAtCreation = true;
    buffer_desc.size = vertex_data.size() * sizeof(float);
    buffer_desc.usage = wgpu::BufferUsage::Vertex;
    app_state->vertex_buf = app_state->device.CreateBuffer(&buffer_desc);
    std::memcpy(app_state->vertex_buf.GetMappedRange(), vertex_data.data(), buffer_desc.size);
    app_state->vertex_buf.Unmap();

    std::array<wgpu::VertexAttribute, 2> vertex_attributes;
    vertex_attributes[0].format = wgpu::VertexFormat::Float32x4;
    vertex_attributes[0].offset = 0;
    vertex_attributes[0].shaderLocation = 0;

    vertex_attributes[1].format = wgpu::VertexFormat::Float32x4;
    vertex_attributes[1].offset = 4 * 4;
    vertex_attributes[1].shaderLocation = 1;

    wgpu::VertexBufferLayout vertex_buf_layout;
    vertex_buf_layout.arrayStride = 2 * 4 * 4;
    vertex_buf_layout.attributeCount = vertex_attributes.size();
    vertex_buf_layout.attributes = vertex_attributes.data();

    wgpu::VertexState vertex_state;
    vertex_state.module = shader_module;
    vertex_state.entryPoint = "vertex_main";
    vertex_state.bufferCount = 1;
    vertex_state.buffers = &vertex_buf_layout;

    wgpu::ColorTargetState render_target_state;
    render_target_state.format = wgpu::TextureFormat::BGRA8Unorm;

    wgpu::FragmentState fragment_state;
    fragment_state.module = shader_module;
    fragment_state.entryPoint = "fragment_main";
    fragment_state.targetCount = 1;
    fragment_state.targets = &render_target_state;

    wgpu::BindGroupLayoutEntry view_param_layout_entry = {};
    view_param_layout_entry.binding = 0;
    view_param_layout_entry.buffer.hasDynamicOffset = false;
    view_param_layout_entry.buffer.type = wgpu::BufferBindingType::Uniform;
    view_param_layout_entry.visibility = wgpu::ShaderStage::Vertex;

    wgpu::BindGroupLayoutDescriptor view_params_bg_layout_desc = {};
    view_params_bg_layout_desc.entryCount = 1;
    view_params_bg_layout_desc.entries = &view_param_layout_entry;

    wgpu::BindGroupLayout view_params_bg_layout =
        app_state->device.CreateBindGroupLayout(&view_params_bg_layout_desc);

    wgpu::PipelineLayoutDescriptor pipeline_layout_desc = {};
    pipeline_layout_desc.bindGroupLayoutCount = 1;
    pipeline_layout_desc.bindGroupLayouts = &view_params_bg_layout;

    wgpu::PipelineLayout pipeline_layout =
        app_state->device.CreatePipelineLayout(&pipeline_layout_desc);

    wgpu::RenderPipelineDescriptor render_pipeline_desc;
    render_pipeline_desc.vertex = vertex_state;
    render_pipeline_desc.fragment = &fragment_state;
    render_pipeline_desc.layout = pipeline_layout;
    // Default primitive state is what we want, triangle list, no indices

    app_state->render_pipeline = app_state->device.CreateRenderPipeline(&render_pipeline_desc);

    // Create the UBO for our bind group
    wgpu::BufferDescriptor ubo_buffer_desc;
    ubo_buffer_desc.mappedAtCreation = false;
    ubo_buffer_desc.size = 16 * sizeof(float);
    ubo_buffer_desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    app_state->view_param_buf = app_state->device.CreateBuffer(&ubo_buffer_desc);

    wgpu::BindGroupEntry view_param_bg_entry = {};
    view_param_bg_entry.binding = 0;
    view_param_bg_entry.buffer = app_state->view_param_buf;
    view_param_bg_entry.size = ubo_buffer_desc.size;

    wgpu::BindGroupDescriptor bind_group_desc = {};
    bind_group_desc.layout = view_params_bg_layout;
    bind_group_desc.entryCount = 1;
    bind_group_desc.entries = &view_param_bg_entry;

    app_state->bind_group = app_state->device.CreateBindGroup(&bind_group_desc);

    app_state->proj = glm::perspective(
        glm::radians(50.f), static_cast<float>(win_width) / win_height, 0.1f, 100.f);
    app_state->camera = ArcballCamera(glm::vec3(0, 0, -2.5), glm::vec3(0), glm::vec3(0, 1, 0));

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
        } else if (event.type == SDL_MOUSEMOTION) {
            const glm::vec2 cur_mouse =
                transform_mouse(glm::vec2(event.motion.x, event.motion.y));
            if (event.motion.state & SDL_BUTTON_LMASK) {
                app_state->camera.rotate(app_state->prev_mouse, cur_mouse);
                app_state->camera_changed = true;
            } else if (event.motion.state & SDL_BUTTON_RMASK) {
                app_state->camera.pan(cur_mouse - app_state->prev_mouse);
                app_state->camera_changed = true;
            }
            app_state->prev_mouse = cur_mouse;
        } else if (event.type == SDL_MOUSEWHEEL) {
#ifdef EMSCRIPTEN
            // We get wheel values at ~10x smaller values in Emscripten environment,
            // so just apply a scale factor to make it match native scroll
            float wheel_scale = 10.f;
#else
            float wheel_scale = 1.f;
#endif
            app_state->camera.zoom(-event.wheel.preciseY * 0.005f * wheel_scale);
            app_state->camera_changed = true;
        }
    }

    wgpu::Buffer upload_buf;
    if (app_state->camera_changed) {
        wgpu::BufferDescriptor upload_buffer_desc;
        upload_buffer_desc.mappedAtCreation = true;
        upload_buffer_desc.size = 16 * sizeof(float);
        upload_buffer_desc.usage = wgpu::BufferUsage::CopySrc;
        upload_buf = app_state->device.CreateBuffer(&upload_buffer_desc);

        const glm::mat4 proj_view = app_state->proj * app_state->camera.transform();

        std::memcpy(
            upload_buf.GetMappedRange(), glm::value_ptr(proj_view), 16 * sizeof(float));
        upload_buf.Unmap();
    }

    wgpu::RenderPassColorAttachment color_attachment;
    color_attachment.view = app_state->swap_chain.GetCurrentTextureView();
    color_attachment.clearValue.r = 0.f;
    color_attachment.clearValue.g = 0.f;
    color_attachment.clearValue.b = 0.f;
    color_attachment.clearValue.a = 1.f;
    color_attachment.loadOp = wgpu::LoadOp::Clear;
    color_attachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor pass_desc;
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &color_attachment;

    wgpu::CommandEncoder encoder = app_state->device.CreateCommandEncoder();

    if (app_state->camera_changed) {
        encoder.CopyBufferToBuffer(
            upload_buf, 0, app_state->view_param_buf, 0, 16 * sizeof(float));
    }

    wgpu::RenderPassEncoder render_pass_enc = encoder.BeginRenderPass(&pass_desc);
    render_pass_enc.SetPipeline(app_state->render_pipeline);
    render_pass_enc.SetVertexBuffer(0, app_state->vertex_buf);
    render_pass_enc.SetBindGroup(0, app_state->bind_group);
    render_pass_enc.Draw(3);
    render_pass_enc.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    app_state->queue.Submit(1, &commands);

#ifndef __EMSCRIPTEN__
    app_state->swap_chain.Present();
#endif
    app_state->camera_changed = false;
}
