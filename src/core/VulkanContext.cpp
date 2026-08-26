#include "VulkanContext.h"

#include "DebugUtils.h"
#include "VkCheck.h"
#include "Window.h"

#include <array>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

namespace core {

namespace {

constexpr const char* kValidationLayerName = "VK_LAYER_KHRONOS_validation";

constexpr std::array<const char*, 1> kRequiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

bool checkValidationLayerSupport() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    for (const auto& layer : layers) {
        if (std::strcmp(layer.layerName, kValidationLayerName) == 0) {
            return true;
        }
    }
    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                              VkDebugUtilsMessageTypeFlagsEXT /*type*/,
                                              const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                              void* /*userData*/) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "[Vulkan] " << callbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    return createInfo;
}

} // namespace

VulkanContext::VulkanContext(Window& window) {
    createInstance(window);
    setupDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
}

VulkanContext::~VulkanContext() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
    if (allocator_ != VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator_);
    }
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
    }
    if (debugMessenger_ != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
    }
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
}

void VulkanContext::createInstance(Window& window) {
    vkCheck(volkInitialize(), "Failed to initialize volk (is a Vulkan loader installed?)");

#ifdef SVK_DEBUG
    validationEnabled_ = checkValidationLayerSupport();
    if (!validationEnabled_) {
        std::cerr << "Validation layer requested but not available; continuing without it.\n";
    }
#endif

    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "simple-vk";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "simple-vk";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> extensions = window.getRequiredInstanceExtensions();
    if (validationEnabled_) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Chaining a messenger here also covers vkCreateInstance/vkDestroyInstance;
    // setupDebugMessenger() installs the persistent one for everything else.
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (validationEnabled_) {
        static const char* layer = kValidationLayerName;
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &layer;

        debugCreateInfo = makeDebugMessengerCreateInfo();
        createInfo.pNext = &debugCreateInfo;
    }

    vkCheck(vkCreateInstance(&createInfo, nullptr, &instance_), "Failed to create Vulkan instance");

    volkLoadInstance(instance_);
}

void VulkanContext::setupDebugMessenger() {
    if (!validationEnabled_) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo = makeDebugMessengerCreateInfo();
    vkCheck(vkCreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_),
            "Failed to set up debug messenger");
}

void VulkanContext::createSurface(Window& window) {
    surface_ = window.createSurface(instance_);
}

QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
    }

    return indices;
}

bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());

    std::set<std::string> required(kRequiredDeviceExtensions.begin(), kRequiredDeviceExtensions.end());
    for (const auto& ext : available) {
        required.erase(ext.extensionName);
    }
    return required.empty();
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device) const {
    if (!findQueueFamilies(device).isComplete()) {
        return false;
    }
    if (!checkDeviceExtensionSupport(device)) {
        return false;
    }

    uint32_t formatCount = 0;
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
    if (formatCount == 0 || presentModeCount == 0) {
        return false;
    }

    // dynamicRendering is a mandatory core feature for any device claiming
    // Vulkan 1.3 support, but it's still an explicit feature bit to enable -
    // check it defensively rather than assuming.
    VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features2.pNext = &features13;
    vkGetPhysicalDeviceFeatures2(device, &features2);

    return features13.dynamicRendering == VK_TRUE;
}

int VulkanContext::ratePhysicalDevice(VkPhysicalDevice device) const {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    int score = 0;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        score += 100;
    }
    score += static_cast<int>(props.limits.maxImageDimension2D);
    return score;
}

void VulkanContext::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (count == 0) {
        throw std::runtime_error("No Vulkan-capable GPU found");
    }

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance_, &count, devices.data());

    VkPhysicalDevice best = VK_NULL_HANDLE;
    int bestScore = -1;
    for (VkPhysicalDevice device : devices) {
        if (!isDeviceSuitable(device)) {
            continue;
        }
        const int score = ratePhysicalDevice(device);
        if (score > bestScore) {
            bestScore = score;
            best = device;
        }
    }

    if (best == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable GPU found (needs graphics+present queues and swapchain support)");
    }

    physicalDevice_ = best;
    queueFamilyIndices_ = findQueueFamilies(physicalDevice_);
}

void VulkanContext::createLogicalDevice() {
    const std::set<uint32_t> uniqueFamilies = {queueFamilyIndices_.graphicsFamily.value(),
                                                queueFamilyIndices_.presentFamily.value()};

    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        info.queueFamilyIndex = family;
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        queueCreateInfos.push_back(info);
    }

    VkPhysicalDeviceFeatures features10{}; // no optional 1.0 features needed

    VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = VK_TRUE; // lets the renderer skip render passes/framebuffers entirely

    VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.pNext = &features13;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &features10;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(kRequiredDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();

    vkCheck(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_), "Failed to create logical device");

    volkLoadDevice(device_); // switch to fast device-level dispatch

    vkGetDeviceQueue(device_, queueFamilyIndices_.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilyIndices_.presentFamily.value(), 0, &presentQueue_);

    setDebugObjectName(device_, VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uint64_t>(device_), "simple-vk device");
    if (graphicsQueue_ == presentQueue_) {
        setDebugObjectName(device_, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(graphicsQueue_),
                            "graphics/present queue");
    } else {
        setDebugObjectName(device_, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(graphicsQueue_),
                            "graphics queue");
        setDebugObjectName(device_, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(presentQueue_),
                            "present queue");
    }
}

void VulkanContext::createAllocator() {
    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo info{};
    info.vulkanApiVersion = VK_API_VERSION_1_3;
    info.physicalDevice = physicalDevice_;
    info.device = device_;
    info.instance = instance_;
    info.pVulkanFunctions = &functions;

    vkCheck(vmaCreateAllocator(&info, &allocator_), "Failed to create VMA allocator");
}

} // namespace core
