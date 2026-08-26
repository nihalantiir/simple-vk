# simple-vk

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/nihalantiir/simple-vk)](https://github.com/nihalantiir/simple-vk/releases)
[![Platforms](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-informational)](#prerequisites)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](#stack)

A clean, minimal, cross-platform Vulkan boilerplate for C++20 projects on
Windows and Linux. It sets up a window, a Vulkan instance/device, a
swapchain, and a render loop that clears the screen and presents — nothing
more. Copy it as a starting point for real projects.

## Stack

- **C++20**
- **SDL3** — windowing, input, surface creation
- **Volk** — Vulkan function loading (no static linking against the loader)
- **VMA** (Vulkan Memory Allocator) — GPU memory allocation
- **GLM** — math

All four libraries ship inside the Vulkan SDK, so there is nothing to fetch
or vendor. `CMakeLists.txt` locates them by pointing at `$VULKAN_SDK`: SDL3
via its bundled CMake config, and volk/VMA/glm (which the SDK ships as raw
headers/libs rather than CMake packages) via manually wired imported
targets, with a `find_package(... CONFIG)` attempt tried first in case a
given SDK version does provide full configs for them.

## Prerequisites

- [Vulkan SDK](https://vulkan.lunarg.com/) installed, with the `VULKAN_SDK`
  environment variable set (the SDK installer does this for you on both
  platforms; verify with `echo $VULKAN_SDK` / `$env:VULKAN_SDK`)
- CMake >= 3.24
- [Ninja](https://ninja-build.org/)
- A C++20 compiler (MSVC, Clang, or GCC)

## Building

```
# Windows (PowerShell)
./scripts/build.ps1 [Debug|Release|RelWithDebInfo]

# Linux (bash)
./scripts/build.sh [Debug|Release|RelWithDebInfo]
```

Both scripts configure an out-of-source build in `build/` with the Ninja
generator and build it. Default config is `Debug`, which also enables
Vulkan validation layers (see `SVK_DEBUG` in `CMakeLists.txt`).

The built binary lands in `build/bin/`.

## Cleaning

```
./scripts/clean.ps1   # Windows
./scripts/clean.sh    # Linux
```

Removes the `build/` directory.

## Project structure

```
simple-vk/
├── CMakeLists.txt
├── scripts/            # build/clean helpers for both platforms
├── src/
│   ├── main.cpp         # wires everything together, owns the run loop
│   ├── core/             # Vulkan + SDL bootstrap — reusable across projects
│   │   ├── Window          SDL3 window, surface creation, event polling
│   │   ├── VulkanContext   instance, debug messenger, device, queues, VMA allocator
│   │   ├── Swapchain       swapchain + image views, resize-safe
│   │   ├── VkCheck.h       VkResult -> exception helper
│   │   └── VmaImpl.cpp     VMA_IMPLEMENTATION translation unit
│   ├── renderer/         # rendering code — swap this out per project
│   │   └── Renderer        per-frame command recording, sync objects, present
│   └── game/             # your game/engine logic goes here
│       └── Game            placeholder, called once per frame from main.cpp
├── shaders/             # reserved for future GLSL/HLSL + compiled SPIR-V
└── assets/              # reserved for textures, models, etc.
```

The `core/` layer is intentionally generic — it doesn't know anything about
what gets drawn. `renderer/` builds on top of it to actually record and
submit frames. `game/` is where application-specific logic lives, isolated
from both. This split is what makes the project easy to reuse: drop in a
new `renderer/` and `game/` for a new project and keep `core/` as-is.

## What v1 does

- Creates an SDL3 window and a Vulkan 1.3 instance (with validation layers
  in Debug builds)
- Picks a suitable physical device (preferring discrete GPUs) and creates a
  logical device + graphics/present queues
- Creates a VMA allocator (wired up, not yet used — the next thing you'll
  want when adding real rendering)
- Creates a swapchain + image views, recreating them on resize or when the
  swapchain reports out-of-date/suboptimal
- Each frame: acquires an image, clears it via `vkCmdClearColorImage`
  (no render pass / pipeline yet), and presents it
- Cleans up every Vulkan/SDL object in the correct order on exit

## Extending it

- Add a graphics pipeline + render pass (or dynamic rendering) in
  `renderer/` once you have shaders to draw with; `shaders/` is ready for
  GLSL/HLSL sources and their compiled SPIR-V output
- Use `VulkanContext::allocator()` (VMA) to create buffers/images
- Build out `game::Game` with real scene/input/update logic — `main.cpp`
  already calls `update()` once per frame before `drawFrame()`

## License

MIT — see [LICENSE](LICENSE).
