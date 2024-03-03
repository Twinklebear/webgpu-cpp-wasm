#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "arcball_camera.h"
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include <emscripten/websocket.h>
#include "webgpu_cpp.h"

const std::string WGSL_SHADER = R"(
alias float4 = vec4<f32>;

struct VertexInput {
    @location(0) position: float4,
    @location(1) color: float4,
};

struct VertexOutput {
    @builtin(position) position: float4,
    @location(0) color: float4,
};

struct ViewParams {
    view_proj: mat4x4<f32>,
};

@group(0) @binding(0)
var<uniform> view_params: ViewParams;

@vertex
fn vertex_main(vert: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.color = vert.color;
    out.position = view_params.view_proj * vert.position;
    return out;
};

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) float4 {
    return float4(in.color);
}
)";

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

int win_width = 1280;
int win_height = 720;

glm::vec2 transform_mouse(glm::vec2 in)
{
    return glm::vec2(in.x * 2.f / win_width - 1.f, 1.f - 2.f * in.y / win_height);
}

int mouse_move_callback(int type, const EmscriptenMouseEvent *event, void *_app_state);
int mouse_wheel_callback(int type, const EmscriptenWheelEvent *event, void *_app_state);
void loop_iteration(void *_app_state);

// Callbacks for trying out websockets
EM_BOOL ws_open_callback(int event_type,
                         const EmscriptenWebSocketOpenEvent *event __attribute__((nonnull)),
                         void *userData);

EM_BOOL ws_message_callback(int event_type,
                            const EmscriptenWebSocketMessageEvent *event
                            __attribute__((nonnull)),
                            void *userData);

EM_BOOL ws_error_callback(int event_type,
                          const EmscriptenWebSocketErrorEvent *event __attribute__((nonnull)),
                          void *userData);

EM_BOOL ws_close_callback(int event_type,
                          const EmscriptenWebSocketCloseEvent *event __attribute__((nonnull)),
                          void *userData);

int main(int argc, const char **argv)
{
    AppState *app_state = new AppState;

    app_state->device = wgpu::Device::Acquire(emscripten_webgpu_get_device());

    wgpu::Instance instance = wgpu::CreateInstance();

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
        shader_module_wgsl.code = WGSL_SHADER.c_str();

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

    // Set up the websocket test
    EmscriptenWebSocketCreateAttributes ws_attr;
    emscripten_websocket_init_create_attributes(&ws_attr);
    // Oatpp test server
    ws_attr.url = "ws://localhost:8000/ws";
    // ws_attr.protocols = "binary,base64";
    //  We're already on the main thread anyways
    ws_attr.createOnMainThread = false;

    auto socket = emscripten_websocket_new(&ws_attr);
    if (socket < 0) {
        std::cerr << "Failed to create websocket\n";
    }

    emscripten_websocket_set_onopen_callback(socket, nullptr, ws_open_callback);
    emscripten_websocket_set_onerror_callback(socket, nullptr, ws_error_callback);
    emscripten_websocket_set_onclose_callback(socket, nullptr, ws_close_callback);
    emscripten_websocket_set_onmessage_callback(socket, nullptr, ws_message_callback);
    return 0;
}

int mouse_move_callback(int type, const EmscriptenMouseEvent *event, void *_app_state)
{
    AppState *app_state = reinterpret_cast<AppState *>(_app_state);

    // TODO: missing a scaling factor here
    const glm::vec2 cur_mouse = transform_mouse(glm::vec2(event->clientX, event->clientY));

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

    app_state->camera.zoom(-event->deltaY * 0.005f);
    app_state->camera_changed = true;
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

EM_BOOL ws_open_callback(int event_type,
                         const EmscriptenWebSocketOpenEvent *event __attribute__((nonnull)),
                         void *userData)
{
    std::cout << "Open websocket: " << event->socket << "\n";
    std::cout << "Event type: " << event_type << "\n";
    uint16_t ready_state = 0;
    emscripten_websocket_get_ready_state(event->socket, &ready_state);
    std::cout << "Ready state = " << ready_state << "\n";

    // Now we can send some data
    emscripten_websocket_send_utf8_text(event->socket, "test message");
    return 0;
}

EM_BOOL ws_message_callback(int event_type,
                            const EmscriptenWebSocketMessageEvent *event
                            __attribute__((nonnull)),
                            void *userData)
{
    std::cout << "Onmessage callback " << event->socket << " event type: " << event_type
              << "\n";
    std::cout << "Received " << event->numBytes << "b of "
              << (event->isText ? "text" : "binary") << " data\n";
    if (event->isText) {
        std::cout << "Text: " << event->data << "\n";
    } else {
        std::cout << "Binary message: [";
        for (int i = 0; i < event->numBytes; ++i) {
            std::cout << event->data[i] << ",";
        }
        std::cout << "]\n";
    }

    // Note: Emscripten free's the data for us after calling this callback
    return 0;
}

EM_BOOL ws_error_callback(int event_type,
                          const EmscriptenWebSocketErrorEvent *event __attribute__((nonnull)),
                          void *userData)
{
    std::cerr << "Onerror callback: " << event->socket << " event type " << event_type << "\n";
    // The error reason will be given to us in the close callback
    return 0;
}

EM_BOOL ws_close_callback(int event_type,
                          const EmscriptenWebSocketCloseEvent *event __attribute__((nonnull)),
                          void *userData)
{
    std::cout << "Onclose callback: " << event->socket << " event type " << event_type << "\n";
    std::cout << "Close reason: " << event->reason << " was clean ? "
              << (event->wasClean ? "true" : "false") << "\n";

    emscripten_websocket_delete(event->socket);
    return 0;
}
