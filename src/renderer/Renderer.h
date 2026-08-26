#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <array>
#include <cstdint>
#include <vector>

namespace core {
class VulkanContext;
class Swapchain;
class Window;
} // namespace core

namespace debug {
class DebugUi;
}

namespace renderer {

struct Vertex {
    float position[2];
    float color[3];
};

// Per-frame renderer: acquires a swapchain image, draws a hardcoded
// triangle via Vulkan 1.3 dynamic rendering, optionally records a debug UI
// into the same pass, and presents.
class Renderer {
public:
    Renderer(core::VulkanContext& context, core::Swapchain& swapchain, core::Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void drawFrame(debug::DebugUi* debugUi = nullptr);

    // Live-editable by the debug UI; re-uploaded to the GPU every frame.
    float* clearColor() { return clearColor_; }
    float* vertexColor(int index) { return vertices_[static_cast<size_t>(index)].color; }

private:
    static constexpr int kFramesInFlight = 2;

    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void recreateSyncObjectsForSwapchain();
    void destroySyncObjects();
    void createVertexBuffers();
    void updateVertexBuffer(int frameIndex);
    void createPipeline();
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, debug::DebugUi* debugUi);

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

    // One vertex buffer per frame in flight: colors are live-edited every
    // frame, and a single shared buffer would race between a frame still
    // in flight on the GPU and the next frame's CPU write.
    std::array<VkBuffer, kFramesInFlight> vertexBuffers_{};
    std::array<VmaAllocation, kFramesInFlight> vertexBufferAllocations_{};
    std::array<void*, kFramesInFlight> vertexBuffersMapped_{};

    float clearColor_[3] = {0.035f, 0.018f, 0.010f};
    std::array<Vertex, 3> vertices_ = {{
        {{0.0f, -0.65f}, {0.96f, 0.74f, 0.24f}},
        {{0.45f, 0.45f}, {0.90f, 0.40f, 0.10f}},
        {{-0.45f, 0.45f}, {0.62f, 0.16f, 0.07f}},
    }};

    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
};

} // namespace renderer
