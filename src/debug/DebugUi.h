#pragma once

#include <volk.h>

#include <string>

union SDL_Event;

namespace core {
class VulkanContext;
class Swapchain;
class Window;
} // namespace core

namespace renderer {
class Renderer;
}

namespace debug {

class DebugUi {
public:
    DebugUi(core::VulkanContext& context, core::Swapchain& swapchain, core::Window& window,
            renderer::Renderer& renderer);
    ~DebugUi();

    DebugUi(const DebugUi&) = delete;
    DebugUi& operator=(const DebugUi&) = delete;

    void processEvent(const SDL_Event& event);

    // Builds this frame's UI. Call once per frame before Renderer::drawFrame().
    void beginFrame();

    // Records the finalized draw data into an already-active dynamic
    // rendering pass.
    void render(VkCommandBuffer cmd);

private:
    void drawOverlay();

    core::VulkanContext& context_;
    core::Swapchain& swapchain_;
    renderer::Renderer& renderer_;

    std::string deviceName_;
    VkFormat colorFormat_ = VK_FORMAT_UNDEFINED;
    bool showDemoWindow_ = false;
};

} // namespace debug
