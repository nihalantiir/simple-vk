#pragma once

#include <volk.h>

#include <cstdint>
#include <vector>

namespace core {
class VulkanContext;
class Swapchain;
class Window;
} // namespace core

namespace renderer {

// Minimal per-frame renderer: acquires a swapchain image, clears it, and
// presents it. This is the extension point for real rendering later -
// replace recordCommandBuffer() with pipeline binds and draw calls once a
// graphics pipeline is introduced.
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
};

} // namespace renderer
