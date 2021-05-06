#include "Vulkan.h"

void uploadMesh(
    Vulkan& vk,
    void* data,
    uint32_t size,
    VulkanMesh& mesh
) {
    createVertexBuffer(
        vk.device,
        vk.memories,
        vk.queueFamily,
        size,
        mesh.vBuff
    );

    void* memory = mapMemory(vk.device, mesh.vBuff.memory);
        memcpy(memory, data, size);
    unMapMemory(vk.device, mesh.vBuff.memory);
}

void uploadMesh(
    Vulkan& vk,
    void* vertices,
    uint32_t verticesSize,
    void* indices,
    uint32_t indicesSize,
    VulkanMesh& mesh
) {
    createVertexBuffer(
        vk.device,
        vk.memories,
        vk.queueFamily,
        verticesSize,
        mesh.vBuff
    );

    void* memory = mapMemory(vk.device, mesh.vBuff.memory);
        memcpy(memory, vertices, verticesSize);
    unMapMemory(vk.device, mesh.vBuff.memory);

    createIndexBuffer(
        vk.device,
        vk.memories,
        vk.queueFamily,
        indicesSize,
        mesh.iBuff
    );

    memory = mapMemory(vk.device, mesh.iBuff.memory);
        memcpy(memory, indices, indicesSize);
    unMapMemory(vk.device, mesh.iBuff.memory);
}

void destroyMesh(
    Vulkan& vk,
    VulkanMesh& mesh
) {
    destroyBuffer(vk, mesh.vBuff);
    destroyBuffer(vk, mesh.iBuff);
}
