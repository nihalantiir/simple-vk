#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <cstdint>
#include <optional>

namespace core {

class Window;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

// Owns the core Vulkan bootstrap objects: instance, debug messenger,
// surface, physical/logical device, queues, and the VMA allocator. Every
// other Vulkan-facing class (Swapchain, Renderer, ...) is built on top of
// this and takes a reference to it rather than owning any of these handles
// itself.
class VulkanContext {
public:
    explicit VulkanContext(Window& window);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    VkInstance instance() const { return instance_; }
    VkSurfaceKHR surface() const { return surface_; }
    VkPhysicalDevice physicalDevice() const { return physicalDevice_; }
    VkDevice device() const { return device_; }
    VkQueue graphicsQueue() const { return graphicsQueue_; }
    VkQueue presentQueue() const { return presentQueue_; }
    VmaAllocator allocator() const { return allocator_; }
    const QueueFamilyIndices& queueFamilies() const { return queueFamilyIndices_; }

private:
    void createInstance(Window& window);
    void setupDebugMessenger();
    void createSurface(Window& window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createAllocator();

    bool isDeviceSuitable(VkPhysicalDevice device) const;
    int ratePhysicalDevice(VkPhysicalDevice device) const;
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VmaAllocator allocator_ = VK_NULL_HANDLE;

    QueueFamilyIndices queueFamilyIndices_;
    bool validationEnabled_ = false;
};

} // namespace core
