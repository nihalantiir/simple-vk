#include "Renderer.h"

#include "../core/Swapchain.h"
#include "../core/VkCheck.h"
#include "../core/VulkanContext.h"
#include "../core/Window.h"

#include <stdexcept>

namespace renderer {

Renderer::Renderer(core::VulkanContext& context, core::Swapchain& swapchain, core::Window& window)
    : context_(context), swapchain_(swapchain), window_(window) {
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(context_.device());
    destroySyncObjects();
    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(context_.device(), commandPool_, nullptr);
    }
}

void Renderer::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = context_.queueFamilies().graphicsFamily.value();

    core::vkCheck(vkCreateCommandPool(context_.device(), &poolInfo, nullptr, &commandPool_),
                  "Failed to create command pool");
}

void Renderer::createCommandBuffers() {
    commandBuffers_.resize(kFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = kFramesInFlight;

    core::vkCheck(vkAllocateCommandBuffers(context_.device(), &allocInfo, commandBuffers_.data()),
                  "Failed to allocate command buffers");
}

void Renderer::createSyncObjects() {
    imageAvailableSemaphores_.resize(kFramesInFlight);
    inFlightFences_.resize(kFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so the first wait doesn't block forever

    for (int i = 0; i < kFramesInFlight; ++i) {
        core::vkCheck(vkCreateSemaphore(context_.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]),
                      "Failed to create image-available semaphore");
        core::vkCheck(vkCreateFence(context_.device(), &fenceInfo, nullptr, &inFlightFences_[i]),
                      "Failed to create in-flight fence");
    }

    // One render-finished semaphore per swapchain image (not per frame in
    // flight): a semaphore can't be re-signaled while a previous signal is
    // still unconsumed, which per-frame sizing can violate when present
    // takes longer than a frame.
    renderFinishedSemaphores_.resize(swapchain_.imageCount());
    for (auto& semaphore : renderFinishedSemaphores_) {
        core::vkCheck(vkCreateSemaphore(context_.device(), &semaphoreInfo, nullptr, &semaphore),
                      "Failed to create render-finished semaphore");
    }

    imagesInFlight_.assign(swapchain_.imageCount(), VK_NULL_HANDLE);
}

void Renderer::recreateSyncObjectsForSwapchain() {
    for (VkSemaphore semaphore : renderFinishedSemaphores_) {
        vkDestroySemaphore(context_.device(), semaphore, nullptr);
    }

    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    renderFinishedSemaphores_.assign(swapchain_.imageCount(), VK_NULL_HANDLE);
    for (auto& semaphore : renderFinishedSemaphores_) {
        core::vkCheck(vkCreateSemaphore(context_.device(), &semaphoreInfo, nullptr, &semaphore),
                      "Failed to recreate render-finished semaphore");
    }

    imagesInFlight_.assign(swapchain_.imageCount(), VK_NULL_HANDLE);
}

void Renderer::destroySyncObjects() {
    for (VkSemaphore semaphore : imageAvailableSemaphores_) {
        vkDestroySemaphore(context_.device(), semaphore, nullptr);
    }
    for (VkSemaphore semaphore : renderFinishedSemaphores_) {
        vkDestroySemaphore(context_.device(), semaphore, nullptr);
    }
    for (VkFence fence : inFlightFences_) {
        vkDestroyFence(context_.device(), fence, nullptr);
    }
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    core::vkCheck(vkBeginCommandBuffer(cmd, &beginInfo), "Failed to begin command buffer");

    const VkImage image = swapchain_.images()[imageIndex];

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkImageMemoryBarrier toTransferDst{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    toTransferDst.srcAccessMask = 0;
    toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toTransferDst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferDst.image = image;
    toTransferDst.subresourceRange = range;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                          nullptr, 1, &toTransferDst);

    VkClearColorValue clearColor{{0.02f, 0.02f, 0.05f, 1.0f}};
    vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

    VkImageMemoryBarrier toPresent{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    toPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toPresent.dstAccessMask = 0;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.image = image;
    toPresent.subresourceRange = range;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                          nullptr, 1, &toPresent);

    core::vkCheck(vkEndCommandBuffer(cmd), "Failed to end command buffer");
}

void Renderer::drawFrame() {
    const VkDevice device = context_.device();

    vkWaitForFences(device, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        device, swapchain_.handle(), UINT64_MAX, imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        swapchain_.recreate();
        recreateSyncObjectsForSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    // If this image is still being read by a previous frame's submission,
    // wait for that frame to finish before reusing it.
    if (imagesInFlight_[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight_[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight_[imageIndex] = inFlightFences_[currentFrame_];

    vkResetFences(device, 1, &inFlightFences_[currentFrame_]);
    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

    const VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
    const VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[imageIndex]};

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    core::vkCheck(vkQueueSubmit(context_.graphicsQueue(), 1, &submitInfo, inFlightFences_[currentFrame_]),
                  "Failed to submit draw command buffer");

    const VkSwapchainKHR swapchains[] = {swapchain_.handle()};
    VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    const VkResult presentResult = vkQueuePresentKHR(context_.presentQueue(), &presentInfo);

    // consumeResizedFlag() must run unconditionally (not as an || operand)
    // so a pending resize is never left unconsumed by short-circuiting.
    const bool windowResized = window_.consumeResizedFlag();
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || windowResized) {
        swapchain_.recreate();
        recreateSyncObjectsForSwapchain();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image");
    }

    currentFrame_ = (currentFrame_ + 1) % kFramesInFlight;
}

} // namespace renderer
