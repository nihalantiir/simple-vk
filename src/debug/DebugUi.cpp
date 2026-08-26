#include "DebugUi.h"

#include "../core/Swapchain.h"
#include "../core/VkCheck.h"
#include "../core/VulkanContext.h"
#include "../core/Window.h"
#include "../renderer/Renderer.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <stdexcept>

namespace debug {

namespace {

void checkVkResult(VkResult result) {
    core::vkCheck(result, "Dear ImGui Vulkan backend error");
}

const char* presentModeLabel(VkPresentModeKHR mode) {
    switch (mode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return "Immediate";
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return "Mailbox";
        case VK_PRESENT_MODE_FIFO_KHR:
            return "Fifo";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return "Fifo relaxed";
        default:
            return "Other";
    }
}

} // namespace

DebugUi::DebugUi(core::VulkanContext& context, core::Swapchain& swapchain, core::Window& window,
                  renderer::Renderer& renderer)
    : context_(context), swapchain_(swapchain), renderer_(renderer), colorFormat_(swapchain.imageFormat()) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(context_.physicalDevice(), &props);
    deviceName_ = props.deviceName;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForVulkan(window.handle())) {
        throw std::runtime_error("Failed to initialize ImGui SDL3 backend");
    }

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_3;
    initInfo.Instance = context_.instance();
    initInfo.PhysicalDevice = context_.physicalDevice();
    initInfo.Device = context_.device();
    initInfo.QueueFamily = context_.queueFamilies().graphicsFamily.value();
    initInfo.Queue = context_.graphicsQueue();
    initInfo.DescriptorPoolSize = 64;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = swapchain_.imageCount();
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat_;
    initInfo.MinAllocationSize = 1024 * 1024;
    initInfo.CheckVkResultFn = checkVkResult;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
    }
}

DebugUi::~DebugUi() {
    vkDeviceWaitIdle(context_.device());
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void DebugUi::processEvent(const SDL_Event& event) {
    ImGui_ImplSDL3_ProcessEvent(&event);
}

void DebugUi::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    drawOverlay();

    ImGui::Render();
}

void DebugUi::render(VkCommandBuffer cmd) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void DebugUi::drawOverlay() {
    ImGui::Begin("simple-vk");

    const ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("%.2f ms (%.0f FPS)", 1000.0f / io.Framerate, io.Framerate);

    const VkExtent2D extent = swapchain_.extent();
    ImGui::Text("Swapchain %ux%u, %s", extent.width, extent.height, presentModeLabel(swapchain_.presentMode()));
    ImGui::Text("Device: %s", deviceName_.c_str());

    ImGui::Separator();
    ImGui::ColorEdit3("Clear color", renderer_.clearColor());
    ImGui::ColorEdit3("Vertex 1", renderer_.vertexColor(0));
    ImGui::ColorEdit3("Vertex 2", renderer_.vertexColor(1));
    ImGui::ColorEdit3("Vertex 3", renderer_.vertexColor(2));

    ImGui::Separator();
    ImGui::Checkbox("Show ImGui demo window", &showDemoWindow_);

    ImGui::End();

    if (showDemoWindow_) {
        ImGui::ShowDemoWindow(&showDemoWindow_);
    }
}

} // namespace debug
