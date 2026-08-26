#pragma once

#include <volk.h>

#include <cstdint>
#include <vector>

namespace core {

class VulkanContext;
class Window;

// Owns the swapchain and its image views. Call recreate() after the window
// is resized or when acquire/present report the swapchain out of date or
// suboptimal.
class Swapchain {
public:
    Swapchain(VulkanContext& context, Window& window);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    void recreate();

    VkSwapchainKHR handle() const { return swapchain_; }
    VkFormat imageFormat() const { return imageFormat_; }
    VkExtent2D extent() const { return extent_; }
    const std::vector<VkImage>& images() const { return images_; }
    const std::vector<VkImageView>& imageViews() const { return imageViews_; }
    uint32_t imageCount() const { return static_cast<uint32_t>(images_.size()); }

private:
    void create();
    void destroy();

    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available) const;
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& available) const;
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    VulkanContext& context_;
    Window& window_;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat imageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
};

} // namespace core
