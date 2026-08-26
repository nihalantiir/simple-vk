# shaders/

GLSL sources, compiled to SPIR-V by CMake at build time (see the shader
compilation block in the top-level `CMakeLists.txt`). Output `.spv` files are
written to the build tree (`build/bin/shaders/`) next to the executable —
nothing under this directory is a build artifact, and nothing here needs to
be copied or installed manually.

Naming convention: `<name>.<stage>`, e.g. `triangle.vert` / `triangle.frag`,
compiled to `<name>.<stage>.spv`. `core::ShaderModule` loads compiled
modules by that relative path (e.g. `"shaders/triangle.vert.spv"`).

Add a new shader by dropping the source file here and adding it to the
`SHADER_SOURCES` list in `CMakeLists.txt`.
