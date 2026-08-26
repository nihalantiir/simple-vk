#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <vector>

namespace core {
class VulkanContext;
class Swapchain;
class Window;
} // namespace core

namespace renderer {

// Per-frame renderer: acquires a swapchain image, draws a single hardcoded
// triangle into it via Vulkan 1.3 dynamic rendering (no render pass /
// framebuffer objects), and presents it. This is the extension point for
// real rendering later - grow recordCommandBuffer() and the pipeline/vertex
// data it uses, or add more of both, once there's more to draw.
class Renderer {
public:
    Renderer(core::VulkanContext& context, core::Swapchain& swapchain, core::Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void drawFrame();

private:
    static constexpr int kFramesInFlight = 2;

    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void recreateSyncObjectsForSwapchain();
    void destroySyncObjects();
    void createVertexBuffer();
    void createPipeline();
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);

    core::VulkanContext& context_;
    core::Swapchain& swapchain_;
    core::Window& window_;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;

    std::vector<VkSemaphore> imageAvailableSemaphores_; // one per frame in flight
    std::vector<VkSemaphore> renderFinishedSemaphores_; // one per swapchain image
    std::vector<VkFence> inFlightFences_;               // one per frame in flight
    std::vector<VkFence> imagesInFlight_;               // tracks which fence currently guards each image

    uint32_t currentFrame_ = 0;

    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VmaAllocation vertexBufferAllocation_ = VK_NULL_HANDLE;

    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
};

} // namespace renderer
