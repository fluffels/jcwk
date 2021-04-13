#include <limits>

#include "Vulkan.h"

#undef max

void present(Vulkan& vk, VkCommandBuffer* cmds, u32 cmdCount) {
    uint32_t imageIndex = 0;
    auto result = vkAcquireNextImageKHR(
        vk.device,
        vk.swap.handle,
        std::numeric_limits<uint64_t>::max(),
        vk.swap.imageReady,
        VK_NULL_HANDLE,
        &imageIndex
    );
    if ((result == VK_SUBOPTIMAL_KHR) ||
            (result == VK_ERROR_OUT_OF_DATE_KHR)) {
        // TODO(jan): implement resize
        throw std::runtime_error("could not acquire next image");
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("could not acquire next image");
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = cmdCount;
    submitInfo.pCommandBuffers = cmds + imageIndex;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vk.swap.imageReady;
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vk.swap.cmdBufferDone;
    vkQueueSubmit(vk.queue, 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vk.swap.handle;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &vk.swap.cmdBufferDone;
    presentInfo.pImageIndices = &imageIndex;
    VKCHECK(vkQueuePresentKHR(vk.queue, &presentInfo));

    vkDeviceWaitIdle(vk.device);
}
