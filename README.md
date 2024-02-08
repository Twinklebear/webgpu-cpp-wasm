# WebGPU C++ -> Wasm Template

Template to get started with WebGPU C++ projects targetting
WebAssembly via Emscripten. Easily build apps and then test
and deploy with Webpack.

## Building

The C++ code is compiled with [Emscripten](https://emscripten.org/) to
WebAssembly, and uses [CMake](https://cmake.org/) for build configuration.
Both need to be installed and in your path to build the project.
You can then run from the repo root directory:

```
mkdir cmake-build
cd cmake-build
emcmake cmake ..
cmake --build .
```

This will build the C++ code with Emscripten to produce the WebAssembly
build of the C++ code, and JavaScript files to import the Wasm into an app
and from a worker thread. The build command will also copy these files into
`./web/src/cpp/` to be used by the web app.

## Running

The app is a small TypeScript shim that sets up the WebGPU device
and calls into the Wasm module to run `main` to start the app.
After building the C++ code, you can run the app:

```
cd web
npm i
npm run serve
```

You can then visit `localhost:8080` in the browser to see the app running.

