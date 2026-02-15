declare module "*.wasm"
{
    const content: any;
    export default content;
}

declare module "*/wgpu_app.js" {
    interface EmscriptenModule {
        callMain(args?: string[]): void;
    }
    export default function(moduleOverrides?: Record<string, unknown>): Promise<EmscriptenModule>;
}
