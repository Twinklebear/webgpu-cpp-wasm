# SDL2 + WebGPU C++ Template

This is a template repo to get started with WebGPU C++ projects
targetting both native and WebAssembly builds. Easily build apps
and then test and deploy native binaries or to the web with Webpack.
The web build is live [here!](https://www.willusher.io/webgpu-cpp-wasm/)

## Building for Web

The C++ code is compiled with [Emscripten](https://emscripten.org/) to
WebAssembly, and uses [CMake](https://cmake.org/) for build configuration.
Both need to be installed and in your path to build the project.
You can then run from the repo root directory:

```
mkdir cmake-build
cd cmake-build
emcmake cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j 8
```

This will build the C++ code with Emscripten to produce the WebAssembly
build of the C++ code, and JavaScript files to import the Wasm into an app
and from a worker thread. The build command will also copy these files into
`./web/src/cpp/` to be used by the web app.

See the [web build CI job](.github/workflows/deploy-page.yml) for more information
if you encounter issues.

### Running the Web App

The app is a small TypeScript shim that sets up the WebGPU device
and calls into the Wasm module to run `main` to start the app.
After building the C++ code, you can run the app:

```
cd web
npm i
npm run serve
```

You can then visit `localhost:8080` in the browser to see the app running.

## Building for Native

Native builds use Dawn to provide an implementation of WebGPU,
Dawn is fetched and built using a modified version of
[Elie Michel's FetchDawn CMake script](https://github.com/eliemichel/WebGPU-distribution).
The [sdl2webgpu](https://github.com/Twinklebear/sdl2webgpu) library
is used to setup a cross platform WebGPU context for SDL2. You can build
the native application via CMake. After installing SDL2
(and libx11-xcb-dev on Ubuntu for Dawn), you can build the native app via:

```
mkdir cmake-build-native
cd cmake-build-native
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j 8
```

The app can then be run:

```
./src/wgpu_app
```

On Windows, if you've instaled SDL2 through vcpkg you can build as shown
below. The var `${env:VCPKG_INSTALLATION_ROOT}` should be set to the root
directory of your vcpk install.

```
mkdir cmake-build-native
cd cmake-build-native
cmake -A x64 .. `
  -G "Visual Studio 17 2022" `
  -DCMAKE_TOOLCHAIN_FILE="${env:VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake"
cmake --build . --config relwithdebinfo -j 8
```

The app can then be run:

```
.\src\RelWithDebInfo\wgpu_app.exe
```

See the [native build CI job](.github/workflows/build-native.yml) for more information
if you encounter issues.
