const HtmlWebpackPlugin = require("html-webpack-plugin");
const CopyWebpackPlugin = require("copy-webpack-plugin");
const path = require("path");

module.exports = {
    entry: "./src/index.ts",
    mode: "development",
    devtool: "inline-source-map",
    devServer: {
        headers: [
            {
                "key": "Cross-Origin-Embedder-Policy",
                "value": "require-corp"
            },
            {
                "key": "Cross-Origin-Opener-Policy",
                "value": "same-origin"
            }
        ]
    },
    output: {
        filename: "[contenthash].js",
        path: path.resolve(__dirname, "dist"),
    },
    module: {
        rules: [
            {
                test: /\.(png|jpg|jpeg|glb)$/i,
                type: "asset/resource",
            },
            {
                test: /\.wasm$/i,
                type: "asset/resource",
            },
            {
                // Embed your WGSL files as strings
                test: /\.wgsl$/i,
                type: "asset/source",
            },
            {
                test: /\.tsx?$/,
                use: "ts-loader",
                exclude: /node_modules/,
            }
        ]
    },
    ignoreWarnings: [
        // These warnings are output for the Emscripten generated code, however they
        // are not issues for us.
        //
        // The criticl dependency issue will not be a problem as we do not
        // set data.urlOrBlob for the worker import, so it will use the constant string
        // import path for wgpu_app.js
        /Critical dependency: the request of a dependency is an expression/,
        // The circular dependency warning is ok, because both files are output by the compiler
        // and will be changed as a pair or not at all.
        /Circular dependency between chunks with runtime \(main, src_cpp_wgpu_app_worker_js\)/
    ],
    resolve: {
        extensions: [".tsx", ".ts", ".js"],
    },
    plugins: [new HtmlWebpackPlugin({
        template: "./index.html",
    }),
    new CopyWebpackPlugin({
        patterns: [
            {
                from: "./src/cpp/*.wasm.map",
                to() {
                    return "[name][ext]";
                },
                noErrorOnMissing: true
            },
            {
                from: "./dbg/**/*.cpp",
                to(f) {
                    const regex = /.*\/web\/dbg\//
                    return f.absoluteFilename.replace(regex, "src/");
                },
                noErrorOnMissing: true
            },
        ]
    })],
};
