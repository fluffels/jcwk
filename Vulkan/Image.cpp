#include <stdexcept>

#include "Vulkan.h"
#include "vulkan/vulkan_core.h"

using std::runtime_error;

void allocateVulkanImage(
    VkDevice device,
    VkDeviceSize size,
    uint32_t memoryType,
    VkImage image,
    VkDeviceMemory& memory
) {
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = size;
    allocateInfo.memoryTypeIndex = memoryType;
    
    auto result = vkAllocateMemory(
        device,
        &allocateInfo,
        nullptr,
        &memory
    );
    VKCHECK(result);

    vkBindImageMemory(device, image, memory, 0);
}

void createImage(
    VkDevice device,
    VkImageType type,
    VkExtent2D extent,
    uint32_t layerCount,
    uint32_t family,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkSampleCountFlagBits samples,
    VkImage& image
) {
    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.flags = flags;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent = { extent.width, extent.height, 1 };
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = layerCount;
    createInfo.format = format;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &family;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.usage = usage;
    createInfo.samples = samples;

    auto code = vkCreateImage(device, &createInfo, nullptr, &image);
    VKCHECK(code);
}

void createSampler(
    VkDevice device,
    VkSampler& sampler
) {
    VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // TODO(jan): Enable anisotropic filtering.
    createInfo.anisotropyEnable = VK_FALSE;
    // createInfo.maxAnisotropy = ;
    createInfo.compareEnable = VK_FALSE;
    // createInfo.minLod = ;
    // createInfo.maxLod = ;
    // createInfo.mipLodBias = ;

    auto code = vkCreateSampler(device, &createInfo, nullptr, &sampler);
    VKCHECK(code);
}

void createView(
    VkDevice device,
    VkImage image,
    VkImageViewType type,
    VkFormat format,
    VkImageAspectFlags aspectMask,
    VkImageView& view
) {
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.image = image;
    createInfo.format = format;
    createInfo.viewType = type;
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    createInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

    auto code = vkCreateImageView(device, &createInfo, nullptr, &view);
    VKCHECK(code);
}

void createVulkanImage(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkImageType type,
    VkImageViewType viewType,
    VkExtent2D extent,
    uint32_t layerCount,
    uint32_t family,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectMask,
    bool hostVisible,
    VkImageCreateFlags imageCreateFlags,
    VkSampleCountFlagBits samples,
    VulkanImage& image
) {
    createImage(
        device,
        type,
        extent,
        layerCount,
        family,
        format,
        usage,
        imageCreateFlags,
        samples,
        image.handle
    );

    auto reqs = getImageMemoryRequirements(device, image.handle);
    auto flags =
        hostVisible? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0;
    auto memType = selectMemoryTypeIndex(memories, reqs, flags);
    allocateVulkanImage(device, reqs.size, memType, image.handle, image.memory);

    createView(
        device,
        image.handle,
        viewType,
        format,
        aspectMask,
        image.view
    );
}

void destroyVulkanImage(VkDevice device, VulkanImage image) {
    vkDestroyImageView(device, image.view, nullptr);
    vkFreeMemory(device, image.memory, nullptr);
    vkDestroyImage(device, image.handle, nullptr);
}

void destroyVulkanSampler(VkDevice device, VulkanSampler sampler) {
    vkDestroySampler(device, sampler.handle, nullptr);
    destroyVulkanImage(device, sampler.image);
}

void createVulkanColorBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family,
    VkFormat format,
    VkSampleCountFlagBits samples,
    VulkanImage& image
) {
    createVulkanImage(
        device,
        memories,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        extent,
        1,
        family,
        format,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        false,
        0,
        samples,
        image
    );
}

void createVulkanDepthBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family,
    VkSampleCountFlagBits samples,
    VulkanImage& image
) {
    createVulkanImage(
        device,
        memories,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        extent,
        1,
        family,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        false,
        0,
        samples,
        image
    );
}

void createVulkanSampler(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkImageType type,
    VkImageViewType viewType,
    VkExtent2D extent,
    uint32_t layerCount,
    uint32_t family,
    VkImageCreateFlags imageCreateFlags,
    VkSampleCountFlagBits samples,
    VulkanSampler& sampler
) {
    createVulkanImage(
        device,
        memories,
        type,
        viewType,
        extent,
        layerCount,
        family,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        false,
        imageCreateFlags,
        samples,
        sampler.image
    );
    createSampler(
        device,
        sampler.handle
    );
}

void createVulkanSamplerCube(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family,
    VulkanSampler& sampler
) {
    createVulkanSampler(
        device,
        memories,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_CUBE,
        extent,
        6,
        family,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        VK_SAMPLE_COUNT_1_BIT,
        sampler
    );
}

void createPrepassImage(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family,
    VkFormat format,
    VulkanSampler& sampler
) {
    createVulkanImage(
        device,
        memories,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        extent,
        1,
        family,
        format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        false,
        0,
        VK_SAMPLE_COUNT_1_BIT,
        sampler.image
    );
    createSampler(
        device,
        sampler.handle
    );
}

void copyBufferToTexture(
    Vulkan vk,
    uint32_t width,
    uint32_t height,
    VulkanBuffer& buffer,
    VulkanImage& image
) {
    VkExtent2D extent = { width, height };

    auto cmd = allocateCommandBuffer(vk.device, vk.cmdPoolTransient);
    beginOneOffCommandBuffer(cmd);

    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = image.handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    {
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageSubresource.mipLevel = 0;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { extent.width, extent.height, 1 };

        vkCmdCopyBufferToImage(
            cmd,
            buffer.handle,
            image.handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region
        );
    }

    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = image.handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    endCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(vk.queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void uploadTexture(
    Vulkan& vk,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    void* data,
    uint32_t size,
    VulkanSampler& sampler
) {
    VulkanBuffer staging;
    createStagingBuffer(
        vk.device,
        vk.memories,
        vk.queueFamily,
        size,
        staging
    );

    void* dst = mapMemory(vk.device, staging.memory);
        memcpy(dst, data, size);
    unMapMemory(vk.device, staging.memory);

    createVulkanImage(
        vk.device,
        vk.memories,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        { width, height},
        1,
        vk.queueFamily,
        format,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        false,
        0,
        VK_SAMPLE_COUNT_1_BIT,
        sampler.image
    );
    createSampler(
        vk.device,
        sampler.handle
    );

    copyBufferToTexture(
        vk,
        width, height,
        staging, sampler.image
    );
}

void destroySampler(Vulkan& vk, VulkanSampler& sampler) {
    vkFreeMemory(vk.device, sampler.image.memory, nullptr);
    vkDestroySampler(vk.device, sampler.handle, nullptr);
    vkDestroyImageView(vk.device, sampler.image.view, nullptr);
    vkDestroyImage(vk.device, sampler.image.handle, nullptr);
}