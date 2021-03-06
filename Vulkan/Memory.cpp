#include <stdexcept>

#include "Vulkan.h"

using std::runtime_error;

VkPhysicalDeviceMemoryProperties getMemories(VkPhysicalDevice gpu) {
    VkPhysicalDeviceMemoryProperties memories;
    vkGetPhysicalDeviceMemoryProperties(
        gpu,
        &memories
    );
    return memories;
}

VkMemoryRequirements getBufferMemoryRequirements(VkDevice device, VkBuffer buffer) {
    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(
        device,
        buffer,
        &requirements
    );
    return requirements;
}

VkMemoryRequirements getImageMemoryRequirements(VkDevice device, VkImage image) {
    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(
        device,
        image,
        &requirements
    );
    return requirements;
}

uint32_t selectMemoryTypeIndex(
    VkPhysicalDeviceMemoryProperties& memories,
    VkMemoryRequirements requirements,
    VkMemoryPropertyFlags extraFlags
) {
    uint32_t typeIndex = 0;
    bool found = false;
    for (; typeIndex < memories.memoryTypeCount; typeIndex++) {
        if (requirements.memoryTypeBits & (1 << typeIndex)) {
            auto flags = memories.memoryTypes[typeIndex].propertyFlags;
            flags &= extraFlags; 
            if (flags == extraFlags) {
                found = true;
                break;
            }
        }
    }
    if (!found) {
        throw runtime_error("could not find memory type");
    }
    return typeIndex;
}

void* mapMemory(
    VkDevice device,
    VkDeviceMemory memory
) {
    void* data = 0;
    auto result = vkMapMemory(
        device,
        memory,
        0,
        VK_WHOLE_SIZE,
        0,
        &data
    );

    if (result != VK_SUCCESS) {
        throw runtime_error("could not map vertex memory");
    }
    return data;
}

void unMapMemory(VkDevice device, VkDeviceMemory memory) {
    vkUnmapMemory(device, memory);
}
