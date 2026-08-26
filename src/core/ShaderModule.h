#pragma once

#include <volk.h>

#include <string>

namespace core {

class VulkanContext;

// RAII wrapper around a VkShaderModule. Loads compiled SPIR-V from disk,
// resolved against the executable's directory (SDL_GetBasePath()) rather
// than the process's current working directory.
class ShaderModule {
public:
    // relativePath e.g. "shaders/triangle.vert.spv".
    ShaderModule(VulkanContext& context, const std::string& relativePath, const char* debugName);
    ~ShaderModule();

    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;

    VkShaderModule handle() const { return module_; }

private:
    VulkanContext& context_;
    VkShaderModule module_ = VK_NULL_HANDLE;
};

} // namespace core
