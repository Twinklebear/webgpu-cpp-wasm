import WGPUApp from "./cpp/wgpu_app.js";

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

(async () => {
  let canvas = document.getElementById("canvas");
  if (navigator.gpu === undefined) {
    canvas.setAttribute("style", "display:none;");
    document
      .getElementById("no-webgpu")
      .setAttribute("style", "display:block;");
    return;
  }

  /*
  if (!sharedArrayBufferSupport()) {
    canvas.setAttribute("style", "display:none;");
    document
      .getElementById("no-shared-array-buffer")
      .setAttribute("style", "display:block;");
    return;
  }
  */

  // Block right click so we can use right click + drag to pan
  canvas.addEventListener("contextmenu", (evt: Event) => {
    evt.preventDefault();
  });

  // Get a GPU device to render with
  let adapter = await navigator.gpu.requestAdapter();
  let device = await adapter.requestDevice();

  // We set -sINVOKE_RUN=0 when building and call main ourselves because something
  // within the promise -> call directly chain was gobbling exceptions
  // making it hard to debug
  let app = await WGPUApp({
    preinitializedWebGPUDevice: device,
    canvas,
  });

  try {
    app.callMain();
  } catch (e) {
    console.error(e.stack);
  }
})();
