#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "easylogging++.h"
#include "SPIRV-Reflect/spirv_reflect.h"

using std::string;
using std::runtime_error;
using std::vector;

#define checkSuccess(r) \
    if (r != VK_SUCCESS) { \
        LOG(ERROR) << __FILE__ << ":" << __LINE__; \
        exit(-1); \
    }

struct VulkanBuffer {
    VkBuffer handle;
    VkDeviceMemory memory;
    VkBufferView view;
};

struct VulkanImage {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
};

struct VulkanSampler {
    VkSampler handle;
    VulkanImage image;
};

struct VulkanSwapChain {
    VkPresentModeKHR presentMode;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkSwapchainKHR handle;
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    vector<VulkanImage> images;
    vector<VkFramebuffer> framebuffers;
    VkSurfaceKHR surface;
    VkSemaphore imageReady;
    VkSemaphore cmdBufferDone;
};

struct Vulkan {
    VkDebugReportCallbackEXT debugCallback;
    VkDevice device;
    VkInstance handle;
    VkPhysicalDevice gpu;
    vector<string> extensions;
    vector<string> layers;
    VkQueue queue;
    uint32_t queueFamily;
    VkPhysicalDeviceMemoryProperties memories;

// TODO(jan): more flexible handling of multiple render passes
    VkRenderPass renderPass;
    VkRenderPass renderPassNoClear;
    VulkanSwapChain swap;

    VulkanImage depth;
    VulkanBuffer uniforms;

    VkCommandPool cmdPool;
    VkCommandPool cmdPoolTransient;

    bool supportsMeshShaders;
    bool supportsQueryPools;

    PFN_vkCmdDrawMeshTasksNV cmdDrawMeshTasksNV;
};

struct VulkanShader {
    VkShaderModule module;
    SpvReflectShaderModule reflect;
    vector<SpvReflectDescriptorSet*> sets;
};

struct VulkanPipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VulkanShader vertexShader;
    VulkanShader fragmentShader;
    VkDescriptorSetLayout descriptorLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkVertexInputBindingDescription inputBinding;
    vector<VkVertexInputAttributeDescription> inputAttributes;
    bool needsTexCoords;
    bool needsNormals;
    bool needsColor;
};

struct VulkanMesh {
    VulkanBuffer vBuff;
    uint32_t vCount;
    VulkanBuffer iBuff;
    uint32_t idxCount;
};

// API Initialization
void createFramebuffers(Vulkan&);
void createVKInstance(Vulkan&);
void initVK(Vulkan&);
void initVKSwapChain(Vulkan&);

// Memory Types & Allocation
VkPhysicalDeviceMemoryProperties getMemories(VkPhysicalDevice gpu);
uint32_t selectMemoryTypeIndex(
    VkPhysicalDeviceMemoryProperties&,
    VkMemoryRequirements,
    VkMemoryPropertyFlags
);
VkMemoryRequirements getMemoryRequirements(VkDevice, VkBuffer);
VkMemoryRequirements getMemoryRequirements(VkDevice, VkImage);
void* mapMemory(VkDevice, VkMemoryRequirements, VkDeviceMemory);
void* mapMemory(VkDevice, VkImage, VkDeviceMemory);
void* mapMemory(VkDevice, VkBuffer, VkDeviceMemory);
void unMapMemory(VkDevice, VkDeviceMemory);

// Synchronization
VkSemaphore createSemaphore(VkDevice device);

// Render Pass
void createRenderPass(
    Vulkan& vk,
    bool clear,
    bool prepass,
    VkRenderPass& renderPass
);

// Buffers
void createUniformBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
);
void createVertexBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
);
void createIndexBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
);
void createStagingBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
);
void createStorageBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
);
void createTexelBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    uint32_t size,
    VulkanBuffer& buffer
);
void updateBuffer(
    Vulkan& vk,
    VulkanBuffer& buffer,
    void* data,
    size_t length
);
void updateUniforms(
    Vulkan& vk,
    void* data,
    size_t length
);
void uploadIndexBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* data,
    uint32_t size,
    VulkanBuffer& buffer
);
void uploadStorageBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* data,
    uint32_t size,
    VulkanBuffer& buffer
);
void uploadTexelBuffer(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* data,
    uint32_t size,
    VulkanBuffer& buffer
);

// Command Buffers
VkCommandPool createCommandPool(
    VkDevice device,
    uint32_t queueFamily,
    bool transient=false
);
VkCommandBuffer allocateCommandBuffer(
    VkDevice device,
    VkCommandPool commandPool
);
void beginOneOffCommandBuffer(
    VkCommandBuffer buffer
);
void beginFrameCommandBuffer(
    VkCommandBuffer buffer
);
void endCommandBuffer(
    VkCommandBuffer buffer
);
void createCommandBuffers(
    VkDevice device,
    VkCommandPool pool,
    uint32_t count,
    vector<VkCommandBuffer>& buffers
);

// Images & Samplers
void createPrepassImage(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family,
    VkFormat format,
    VulkanSampler& sampler
);
void createVulkanDepthBuffer(
    VkDevice,
    VkPhysicalDeviceMemoryProperties&,
    VkExtent2D,
    uint32_t,
    VulkanImage&
);
void createVulkanSampler2D(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family,
    VulkanSampler&
);
void createVulkanSamplerCube(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkExtent2D extent,
    uint32_t family,
    VulkanSampler&
);
void uploadTexture(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    VkQueue queue,
    uint32_t queueFamily,
    VkCommandPool cmdPoolTransient,
    uint32_t width,
    uint32_t height,
    void* data,
    uint32_t size,
    VulkanSampler& sampler
);
void destroyVulkanImage(
    VkDevice device,
    VulkanImage image
);
void destroyVulkanSampler(
    VkDevice device,
    VulkanSampler sampler
);

// Pipeline
void initVKPipelineCCW(
    Vulkan& vk,
    char* name,
    VulkanPipeline&
);
void initVKPipeline(
    Vulkan& vk,
    char* name,
    VulkanPipeline&
);

// Descriptors
void updateCombinedImageSampler(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VulkanSampler* samplers,
    uint32_t count
);
void updateStorageBuffer(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VkBuffer buffer
);
void updateUniformBuffer(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VkBuffer buffer
);
void updateUniformTexelBuffer(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    uint32_t binding,
    VkBufferView view
);

void uploadMesh(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* data,
    uint32_t size,
    VulkanMesh& mesh
);
void uploadMesh(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties& memories,
    uint32_t queueFamily,
    void* vertices,
    uint32_t verticesSize,
    void* indices,
    uint32_t indicesSize,
    VulkanMesh& mesh
);

// Present
void present(
    Vulkan& vk,
    vector<vector<VkCommandBuffer>>& cmdss
);
