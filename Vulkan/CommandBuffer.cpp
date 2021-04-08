#include <stdexcept>

#include "Vulkan.h"

using std::runtime_error;

VkCommandPool createCommandPool(
    VkDevice device,
    uint32_t family,
    bool transient
) {
    VkCommandPool result = {};

    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    if (transient) {
        createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    }
    createInfo.queueFamilyIndex = family;

    auto code = vkCreateCommandPool(device, &createInfo, nullptr, &result);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not create command pool");
    }
    return result;
}

VkCommandBuffer allocateCommandBuffer(
    VkDevice device,
    VkCommandPool pool
) {
    VkCommandBuffer result = {};

    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandBufferCount = 1;
    allocateInfo.commandPool = pool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    auto code = vkAllocateCommandBuffers(device, &allocateInfo, &result);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not allocate command buffer");
    }
    return result;
}

void beginCommandBuffer(
    VkCommandBuffer buffer,
    VkCommandBufferUsageFlags flags
) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    auto code = vkBeginCommandBuffer(buffer, &beginInfo);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not begin command buffer");
    }
}

void beginOneOffCommandBuffer(VkCommandBuffer buffer) {
    beginCommandBuffer(buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void beginFrameCommandBuffer(VkCommandBuffer buffer) {
    beginCommandBuffer(buffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
}

void endCommandBuffer(
    VkCommandBuffer buffer
) {
    auto code = vkEndCommandBuffer(buffer);
    if (code != VK_SUCCESS) {
        throw runtime_error("could not end command buffer");
    }
}

void createCommandBuffers(
    VkDevice device,
    VkCommandPool pool,
    uint32_t count,
    VkCommandBuffer* buffers
) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = count;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VKCHECK(vkAllocateCommandBuffers(
        device,
        &allocInfo,
        buffers
    ));
}

void submitCommandBuffer(
    VkCommandBuffer& cmd,
    VkQueue& queue
) {
    VkSubmitInfo i = {};
    i.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    i.pNext = nullptr;
    i.commandBufferCount = 1;
    i.pCommandBuffers = &cmd;
    i.signalSemaphoreCount = 0;
    i.pSignalSemaphores = nullptr;
    i.waitSemaphoreCount = 0;
    i.pWaitSemaphores = 0;
    i.pWaitDstStageMask = nullptr;

    auto result = vkQueueSubmit(
        queue,
        1,
        &i,
        VK_NULL_HANDLE
    );
    VKCHECK(result);
}

void releaseBufferOwnership(
    VkDevice device,
    VkCommandPool pool,
    VkQueue queue,
    VkBuffer buffer,
    uint32_t srcQueueFamily,
    uint32_t dstQueueFamily,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask
) {
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.buffer = buffer;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcQueueFamilyIndex = srcQueueFamily;
    barrier.dstQueueFamilyIndex = dstQueueFamily;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = 0;

    VkCommandBuffer cmd = allocateCommandBuffer(device, pool);
    beginOneOffCommandBuffer(cmd);
    vkCmdPipelineBarrier(
        cmd,
        srcStageMask, dstStageMask,
        0,
        0, nullptr,
        1, &barrier,
        0, nullptr
    );
    endCommandBuffer(cmd);
    submitCommandBuffer(cmd, queue);
}

void acceptBufferOwnership(
    VkDevice device,
    VkCommandPool pool,
    VkQueue queue,
    VkBuffer buffer,
    uint32_t srcQueueFamily,
    uint32_t dstQueueFamily,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask
) {
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.buffer = buffer;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcQueueFamilyIndex = srcQueueFamily;
    barrier.dstQueueFamilyIndex = dstQueueFamily;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkCommandBuffer cmd = allocateCommandBuffer(device, pool);
    beginOneOffCommandBuffer(cmd);
    vkCmdPipelineBarrier(
        cmd,
        srcStageMask, dstStageMask,
        0,
        0, nullptr,
        1, &barrier,
        0, nullptr
    );
    endCommandBuffer(cmd);
    submitCommandBuffer(cmd, queue);
}
