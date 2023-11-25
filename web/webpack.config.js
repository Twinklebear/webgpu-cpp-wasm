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
        filename: "main.js",
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
    resolve: {
        extensions: [".tsx", ".ts", ".js"],
        fallback: {
            "path": false,
            "fs": false,
            "crypto": false,
        }
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
