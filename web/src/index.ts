import runCpp from "./cpp/wgpu_app";
import wasm from "./cpp/wgpu_app.wasm";

function sharedArrayBufferSupport() {
    try {
        var s = new SharedArrayBuffer(1024);
        if (s === undefined) {
            return false;
        }
    } catch (e) {
        return false;
    }
    return true;
}

(async () =>
{
    if (navigator.gpu === undefined) {
        document.getElementById("webgpu-canvas").setAttribute("style", "display:none;");
        document.getElementById("no-webgpu").setAttribute("style", "display:block;");
        return;
    }

    if (!sharedArrayBufferSupport()) {
        document.getElementById("webgpu-canvas").setAttribute("style", "display:none;");
        document.getElementById("no-shared-array-buffer").setAttribute("style", "display:block;");
        return;
    }

    // Get a GPU device to render with
    let adapter = await navigator.gpu.requestAdapter();
    let device = await adapter.requestDevice();

    // We set -sINVOKE_RUN=0 when building and call main ourselves because something
    // within the promise -> call directly chain was gobbling exceptions
    // making it hard to debug
    let cpp = await runCpp({
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

