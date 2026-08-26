# assets/

Reserved for runtime content this project doesn't yet load: textures,
models, audio, fonts, and similar data. Nothing here is consumed by v0.2.0 —
the triangle's geometry and color are inline in `renderer/Renderer.cpp`.

When you add real asset loading, prefer resolving paths the same way
`core::ShaderModule` resolves shaders: relative to the executable's own
directory (`SDL_GetBasePath()`), not the process's current working
directory.
