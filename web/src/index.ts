import runCpp from "./cpp/wgpu_app";
import wasm from "./cpp/wgpu_app.wasm";

(async () =>
{
    if (navigator.gpu === undefined) {
        document.getElementById("webgpu-canvas").setAttribute("style", "display:none;");
        document.getElementById("no-webgpu").setAttribute("style", "display:block;");
        return;
    }

    // Get a GPU device to render with
    let adapter = await navigator.gpu.requestAdapter();
    let device = await adapter.requestDevice();

    // We set no initial run here and call main ourselves because something
    // within the promise -> call directly chain was gobbling exceptions
    // making it hard to debug
    let cpp = await runCpp({
        noInitialRun: true,
        preinitializedWebGPUDevice: device,
        // We bundle the wasm using webpack so return the bundled file name
        locateFile: function (_f: string)
        {
            return wasm;
        }
    })

    try {
        cpp.callMain();
    } catch (e) {
        console.error(e.stack);
    }
})();

