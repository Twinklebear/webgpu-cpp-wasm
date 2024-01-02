#include <array>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include "arcball_camera.h"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include "webgpu_cpp.h"

#include "embedded_files.h"

struct AppState {
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

double css_w = 0.0;
double css_h = 0.0;

int win_width = 1280;
int win_height = 720;
float dpi = 2.f;

glm::vec2 transform_mouse(glm::vec2 in)
{
    return glm::vec2(in.x * 2.f / css_w - 1.f, 1.f - 2.f * in.y / css_h);
}

int mouse_move_callback(int type, const EmscriptenMouseEvent *event, void *_app_state);
int mouse_wheel_callback(int type, const EmscriptenWheelEvent *event, void *_app_state);

void loop_iteration(void *_app_state);

int main(int argc, const char **argv)
{
    AppState *app_state = new AppState;

    // TODO: Need a way to call these while also linking threads

    // TODO: we can't call this because we also load this same wasm module into a worker
    // which doesn't have access to the window APIs
    dpi = emscripten_get_device_pixel_ratio();
    emscripten_get_element_css_size("#webgpu-canvas", &css_w, &css_h);
    std::cout << "Canvas element size = " << css_w << "x" << css_h << "\n";

    emscripten_get_canvas_element_size("#webgpu-canvas", &win_width, &win_height);
    std::cout << "Canvas size: " << win_width << "x" << win_height << "\n";
    win_width = css_w * dpi;
    win_height = css_h * dpi;
    std::cout << "Setting canvas size: " << win_width << "x" << win_height << "\n";

    emscripten_set_canvas_element_size("#webgpu-canvas", win_width, win_height);

    app_state->device = wgpu::Device::Acquire(emscripten_webgpu_get_device());

    wgpu::InstanceDescriptor instance_desc;
    wgpu::Instance instance = wgpu::CreateInstance(&instance_desc);

    app_state->device.SetUncapturedErrorCallback(
        [](WGPUErrorType type, const char *msg, void *data) {
            std::cout << "WebGPU Error: " << msg << "\n" << std::flush;
            emscripten_cancel_main_loop();
            emscripten_force_exit(1);
            std::exit(1);
        },
        nullptr);

    /*
    app_state->device.SetLoggingCallback(
        [](WGPULoggingType type, const char *msg, void *data) {
            std::cout << "WebGPU Log: " << msg << "\n" << std::flush;
        },
        nullptr);
        */

    app_state->queue = app_state->device.GetQueue();

    wgpu::SurfaceDescriptorFromCanvasHTMLSelector selector;
    selector.selector = "#webgpu-canvas";

    wgpu::SurfaceDescriptor surface_desc;
    surface_desc.nextInChain = &selector;

    app_state->surface = instance.CreateSurface(&surface_desc);

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
        shader_module_wgsl.code = reinterpret_cast<const char*>(triangle_wgsl);

        wgpu::ShaderModuleDescriptor shader_module_desc;
        shader_module_desc.nextInChain = &shader_module_wgsl;
        shader_module = app_state->device.CreateShaderModule(&shader_module_desc);

        // This is coming in Emscripten 3.1.51
        /*
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
                        case WGPUCompilationMessageType_Force32:
                            std::cout << "force32";
                            break;
                        default:
                            break;
                        }

                        std::cout << ": " << m.message << "\n";
                    }
                }
            },
            nullptr);
            */
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

    emscripten_set_mousemove_callback("#webgpu-canvas", app_state, true, mouse_move_callback);
    emscripten_set_wheel_callback("#webgpu-canvas", app_state, true, mouse_wheel_callback);

    emscripten_set_main_loop_arg(loop_iteration, app_state, -1, 0);

#if 1
    int x = 0;
    std::thread test_thread([&]() { x = 10; });
    // Note: in a real app, would not join a thread like this on the main thread,
    // should detach the thread and likely let it just run as a persistent worker thread
    // or just do joins on other threads. But we do it here to make sure
    // that we should see the changed value of x on the main thread now to test
    // shared memory.
    test_thread.join();
    std::cout << "X = " << x << "\n";
#endif

    return 0;
}

int mouse_move_callback(int type, const EmscriptenMouseEvent *event, void *_app_state)
{
    AppState *app_state = reinterpret_cast<AppState *>(_app_state);

    const glm::vec2 cur_mouse = transform_mouse(glm::vec2(event->targetX, event->targetY));

    if (app_state->prev_mouse != glm::vec2(-2.f)) {
        if (event->buttons & 1) {
            app_state->camera.rotate(app_state->prev_mouse, cur_mouse);
            app_state->camera_changed = true;
        } else if (event->buttons & 2) {
            app_state->camera.pan(cur_mouse - app_state->prev_mouse);
            app_state->camera_changed = true;
        }
    }
    app_state->prev_mouse = cur_mouse;

    return true;
}

int mouse_wheel_callback(int type, const EmscriptenWheelEvent *event, void *_app_state)
{
    AppState *app_state = reinterpret_cast<AppState *>(_app_state);

    // Pinch events on the touchpad the ctrl key set
    // TODO: this likely breaks scroll on a scroll wheel, so we need a way to detect if the
    // user has a mouse and change the behavior. Need to test on a real mouse
    if (true) {//event->mouse.ctrlKey) {
        app_state->camera.zoom(-event->deltaY * 0.005f * dpi);
        app_state->camera_changed = true;
    } else {
        glm::vec2 prev_mouse(css_w / 2.f, css_h / 2.f);

        const auto cur_mouse =
            transform_mouse(prev_mouse - glm::vec2(event->deltaX, event->deltaY));
        prev_mouse = transform_mouse(prev_mouse);

        app_state->camera.rotate(prev_mouse, cur_mouse);
        app_state->camera_changed = true;
    }

    return true;
}

void loop_iteration(void *_app_state)
{
    AppState *app_state = reinterpret_cast<AppState *>(_app_state);
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
    // Here the # refers to the number of command buffers being submitted
    app_state->queue.Submit(1, &commands);

    app_state->camera_changed = false;
}
