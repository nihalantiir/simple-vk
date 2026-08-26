cmake_minimum_required(VERSION 3.24)
project(simple-vk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(NOT DEFINED ENV{VULKAN_SDK})
  message(FATAL_ERROR "VULKAN_SDK environment variable is not set. Install the "
                       "Vulkan SDK (the installer sets this for you) and "
                       "reopen your shell before configuring.")
endif()
set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")

# The SDK exposes some bundled packages as proper CMake configs (SDL3, at a
# nonstandard <sdk>/cmake path on Windows) and others (volk, VMA, glm) only
# as raw headers/libs under Include/ and Lib/. find_package() is tried first
# for everything so this keeps working if a given SDK version/platform ships
# full configs; anything not found that way falls back to manually wired
# imported targets built from the raw SDK layout below.
list(APPEND CMAKE_PREFIX_PATH
  "${VULKAN_SDK_ROOT}"
  "${VULKAN_SDK_ROOT}/cmake"
  "${VULKAN_SDK_ROOT}/lib/cmake"
)

find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(volk CONFIG QUIET)
find_package(VulkanMemoryAllocator CONFIG QUIET)
find_package(glm CONFIG QUIET)

# --- volk: function loader (prebuilt static lib in the SDK) ---
if(NOT TARGET volk::volk)
  find_path(VOLK_INCLUDE_DIR NAMES volk.h
    PATHS "${VULKAN_SDK_ROOT}/Include/Volk" "${VULKAN_SDK_ROOT}/include/volk" "${VULKAN_SDK_ROOT}/include/Volk"
    NO_DEFAULT_PATH)
  find_library(VOLK_LIBRARY_RELEASE NAMES volk
    PATHS "${VULKAN_SDK_ROOT}/Lib" "${VULKAN_SDK_ROOT}/lib" NO_DEFAULT_PATH)
  find_library(VOLK_LIBRARY_DEBUG NAMES volkd
    PATHS "${VULKAN_SDK_ROOT}/Lib" "${VULKAN_SDK_ROOT}/lib" NO_DEFAULT_PATH)

  if(NOT VOLK_INCLUDE_DIR OR NOT VOLK_LIBRARY_RELEASE)
    message(FATAL_ERROR "Could not find volk under ${VULKAN_SDK_ROOT}. "
                         "Expected volk.h plus a prebuilt volk/volkd library.")
  endif()

  add_library(volk::volk STATIC IMPORTED)
  set_target_properties(volk::volk PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${VOLK_INCLUDE_DIR}"
    IMPORTED_LOCATION "${VOLK_LIBRARY_RELEASE}"
  )
  if(VOLK_LIBRARY_DEBUG)
    set_target_properties(volk::volk PROPERTIES IMPORTED_LOCATION_DEBUG "${VOLK_LIBRARY_DEBUG}")
  endif()
endif()

# --- Vulkan Memory Allocator: header-only ---
if(NOT TARGET GPUOpen::VulkanMemoryAllocator)
  find_path(VMA_INCLUDE_DIR NAMES vk_mem_alloc.h
    PATHS "${VULKAN_SDK_ROOT}/Include/vma" "${VULKAN_SDK_ROOT}/include/vma"
    NO_DEFAULT_PATH)
  if(NOT VMA_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find vk_mem_alloc.h under ${VULKAN_SDK_ROOT}")
  endif()

  add_library(VulkanMemoryAllocator INTERFACE)
  target_include_directories(VulkanMemoryAllocator INTERFACE "${VMA_INCLUDE_DIR}")
  add_library(GPUOpen::VulkanMemoryAllocator ALIAS VulkanMemoryAllocator)
endif()

# --- GLM: header-only ---
if(NOT TARGET glm::glm)
  find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp
    PATHS "${VULKAN_SDK_ROOT}/Include" "${VULKAN_SDK_ROOT}/include"
    NO_DEFAULT_PATH)
  if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find glm/glm.hpp under ${VULKAN_SDK_ROOT}")
  endif()

  add_library(glm INTERFACE)
  target_include_directories(glm INTERFACE "${GLM_INCLUDE_DIR}")
  add_library(glm::glm ALIAS glm)
endif()

add_executable(simple-vk
  src/main.cpp
  src/core/Window.cpp
  src/core/VulkanContext.cpp
  src/core/Swapchain.cpp
  src/core/VmaImpl.cpp
  src/renderer/Renderer.cpp
  src/game/Game.cpp
)

target_include_directories(simple-vk PRIVATE src)

target_compile_definitions(simple-vk PRIVATE
  VK_NO_PROTOTYPES               # all Vulkan entry points come from volk
  VMA_STATIC_VULKAN_FUNCTIONS=0
  VMA_DYNAMIC_VULKAN_FUNCTIONS=1
  $<$<CONFIG:Debug>:SVK_DEBUG>    # enables validation layers + debug messenger
  $<$<PLATFORM_ID:Windows>:NOMINMAX>
  $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
)

target_compile_options(simple-vk PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

target_link_libraries(simple-vk PRIVATE
  volk::volk
  GPUOpen::VulkanMemoryAllocator
  SDL3::SDL3
  glm::glm
)

set_target_properties(simple-vk PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
