#include "Vulkan.h"

VkSemaphore createSemaphore(VkDevice device) {
    VkSemaphore result;

    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VKCHECK(vkCreateSemaphore(
        device,
        &createInfo,
        nullptr,
        &result
    ));

    return result;
}
