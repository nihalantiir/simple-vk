# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.3.0] - 2026-08-26

### Added

- Dear ImGui debug overlay (`src/debug/DebugUi`), vendored via CMake
  FetchContent into `external/` and pinned to an exact commit. Shows frame
  time/FPS, swapchain extent and present mode, device name, and live
  clear-color/vertex-color editors. Renders into the same dynamic
  rendering pass as the triangle. Demo window off by default
- `external/README.md` explaining the vendoring policy

### Changed

- Triangle recolored to red-orange-gold (deep rust, ember, gold) on a
  near-black warm clear color, with slightly taller proportions
- Vertex buffers are now one per frame in flight instead of one shared
  buffer, since colors are now live-edited every frame and a shared
  buffer would race between an in-flight frame's GPU read and the next
  frame's CPU write
- `.github/banner.svg` rewritten: plain gold-orange triangle, no card or
  frame, quiet wordmark
- Trimmed narration-style comments across `src/` and `CMakeLists.txt`

## [0.2.0] - 2026-08-26

### Added

- Vulkan 1.3 dynamic rendering (`vkCmdBeginRendering`/`vkCmdEndRendering`),
  replacing the clear-only render path with an actual graphics pipeline
  that draws a single triangle
- `core::ShaderModule`, a small RAII `VkShaderModule` wrapper, loading
  compiled SPIR-V relative to the executable's own directory
- `shaders/triangle.vert` / `shaders/triangle.frag`, compiled to SPIR-V by
  CMake at build time (`Vulkan::glslc`, output written to the build tree)
- VMA-backed vertex buffer for the triangle's geometry
- Debug object naming via `VK_EXT_debug_utils` for key Vulkan objects
  (device, queues, swapchain, image views, command pool/buffers, pipeline,
  shader modules, vertex buffer) in Debug builds
- Window title now shows live frame time and FPS
- `CMakePresets.json` (debug/release/relwithdebinfo, Ninja)
- `.github/workflows/ci.yml`, configure + build (compile-only) on Windows
  and Linux
- `shaders/README.md` and `assets/README.md` explaining what belongs there
- Documentation section in the README linking the project wiki
- Repo banner (`.github/banner.svg`)

### Changed

- Swapchain images are created with `COLOR_ATTACHMENT` usage only (the
  `TRANSFER_DST` usage needed for the old clear-only path is gone)
- `VulkanContext` now requires and enables the `dynamicRendering` 1.3
  feature, and verifies physical device support for it
- Windows builds now copy `SDL3.dll` next to the executable after building

## [0.1.0] - 2026-08-26

### Added

- Initial release: SDL3 window, Vulkan instance/device/queues via volk,
  VMA allocator, resize-safe swapchain, a render loop that clears the
  screen and presents, and clean, ordered shutdown
- `core/` (Vulkan + SDL bootstrap), `renderer/`, and `game/` layers
- Out-of-source CMake + Ninja builds for Windows and Linux
- `scripts/build.*` / `scripts/clean.*` for both platforms
- MIT license
