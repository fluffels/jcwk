#pragma warning(disable: 4018)
#pragma warning(disable: 4267)

#include <stdexcept>

#include "Vulkan.h"

using std::runtime_error;
using std::string;

const char**
stringVectorToC(const vector<string>& v) {
    auto count = v.size();
    auto strings = new const char*[count];
    for (unsigned i = 0; i < count; i++) {
        strings[i] = v[i].c_str();
    }
    return strings;
}

void checkVersion(uint32_t version) {
    auto major = VK_VERSION_MAJOR(version);
    auto minor = VK_VERSION_MINOR(version);
    auto patch = VK_VERSION_PATCH(version);

    INFO("Instance version: %d.%d.%d", major, minor, patch);
    
    if ((major < 1) || (minor < 2) || (patch < 141)) {
        throw runtime_error("you need at least Vulkan 1.2.141");
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char *layerPrefix,
    const char *msg,
    void *userData
) {
    if (flags == VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        LOG("ERROR", "[%s] %s", layerPrefix, msg);
    } else if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        LOG("WARNING", "[%s] %s", layerPrefix, msg);
    } else if (flags == VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        LOG("DEBUG", "[%s] %s", layerPrefix, msg);
    } else {
        LOG("INFO", "[%s] %s", layerPrefix, msg);
    }
    return VK_FALSE;
}

void computeSampleCounts(Vulkan& vk) {
    // TODO(jan): make this configurable
    auto maxSampleCount = VK_SAMPLE_COUNT_2_BIT;
    auto sampleCountFilter = 0;
    for (int i = 0; i < maxSampleCount; i++) {
        sampleCountFilter |= (1 << i);
    }
    VkPhysicalDeviceProperties props = {};
    vkGetPhysicalDeviceProperties(vk.gpu, &props);
    auto counts = props.limits.framebufferColorSampleCounts &
        props.limits.framebufferDepthSampleCounts &
        sampleCountFilter;
    if (counts & VK_SAMPLE_COUNT_64_BIT) {
        vk.sampleCountFlags = VK_SAMPLE_COUNT_64_BIT;
        vk.sampleCount = 64;
    } else if (counts & VK_SAMPLE_COUNT_32_BIT) {
        vk.sampleCountFlags = VK_SAMPLE_COUNT_32_BIT;
        vk.sampleCount = 32;
    } else if (counts & VK_SAMPLE_COUNT_16_BIT) {
        vk.sampleCountFlags = VK_SAMPLE_COUNT_16_BIT;
        vk.sampleCount = 16;
    } else if (counts & VK_SAMPLE_COUNT_8_BIT) {
        vk.sampleCountFlags = VK_SAMPLE_COUNT_8_BIT;
        vk.sampleCount = 8;
    } else if (counts & VK_SAMPLE_COUNT_4_BIT) {
        vk.sampleCountFlags = VK_SAMPLE_COUNT_4_BIT;
        vk.sampleCount = 4;
    } else if (counts & VK_SAMPLE_COUNT_2_BIT) {
        vk.sampleCountFlags = VK_SAMPLE_COUNT_2_BIT;
        vk.sampleCount = 2;
    } else {
        vk.sampleCountFlags = VK_SAMPLE_COUNT_1_BIT;
        vk.sampleCountFlags = 1;
    }
}

void createDebugCallback(Vulkan& vk) {
    VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
    debugReportCallbackCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugReportCallbackCreateInfo.flags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debugReportCallbackCreateInfo.pfnCallback = debugCallback;
    auto create =
        (PFN_vkCreateDebugReportCallbackEXT)
        vkGetInstanceProcAddr(vk.handle, "vkCreateDebugReportCallbackEXT");
    if (create == nullptr) {
        WARN("couldn't load debug callback creation function");
    } else {
        VKCHECK(
            create(
                vk.handle,
                &debugReportCallbackCreateInfo,
                nullptr,
                &vk.debugCallback
            )
        );
    }
}

void createVKInstance(Vulkan& vk, vector<string>* appExtensions) {
    uint32_t version;

    vkEnumerateInstanceVersion(&version);
    checkVersion(version);

    uint32_t layerCount = 0;
    vector<VkLayerProperties> layers;
    VKCHECK(
        vkEnumerateInstanceLayerProperties(
            &layerCount,
            NULL
        )
    );
    layers.resize(layerCount);
    VKCHECK(
        vkEnumerateInstanceLayerProperties( 
            &layerCount,
            layers.data()
        )
    );
    for (auto& layer: layers) {
        INFO("available layer: %s", layer.layerName);
    }

    uint32_t extensionCount = 0;
    vector<VkExtensionProperties> availableExtensions;
    VKCHECK(
        vkEnumerateInstanceExtensionProperties(
            NULL,
            &extensionCount,
            NULL
        )
    )
    availableExtensions.resize(extensionCount);
    VKCHECK(
        vkEnumerateInstanceExtensionProperties(
            NULL,
            &extensionCount,
            availableExtensions.data()
        )
    )

    vk.extensions.insert(vk.extensions.begin(), VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    vk.extensions.insert(vk.extensions.begin(), VK_KHR_SURFACE_EXTENSION_NAME);
    vk.extensions.insert(
        vk.extensions.begin(),
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    );

    if (appExtensions != nullptr) {
        for (auto& appExtension: *appExtensions) {
            for (auto& extension: vk.extensions) {
                if (appExtension == extension) {
                    goto OUTER;
                }
            }
            vk.extensions.push_back(appExtension);
            OUTER: ;
        }
    }

    for (auto& requestedExtension: vk.extensions) {
        bool found = false;
        for (auto& availableExtension: availableExtensions) {
            if (requestedExtension == availableExtension.extensionName) {
                found = true;
                break;
            }
        }
        if (!found) {
            FATAL("extension %s not available", requestedExtension.c_str());
        }
    }

    vk.layers.push_back("VK_LAYER_KHRONOS_validation");

    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &app;

    createInfo.enabledLayerCount = vk.layers.size();
    auto enabledLayerNames = stringVectorToC(vk.layers);
    createInfo.ppEnabledLayerNames = enabledLayerNames;

    createInfo.enabledExtensionCount = vk.extensions.size();
    auto enabledExtensionNames = stringVectorToC(vk.extensions);
    createInfo.ppEnabledExtensionNames = enabledExtensionNames;
    
    VkResult result = vkCreateInstance(&createInfo, nullptr, &vk.handle);
    if (result == VK_ERROR_INITIALIZATION_FAILED) {
        ERR("VK initialization failed");
    } else if (result == VK_ERROR_LAYER_NOT_PRESENT) {
        ERR("layer not present");
    } else if (result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        ERR("extension not present");
    } else if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ERR("driver not compatible");
    }
    if (result != VK_SUCCESS) {
        ERR("vkCreateInstance failed");
        exit(-1);
    }

    createDebugCallback(vk);

    delete[] enabledLayerNames;
    delete[] enabledExtensionNames;
}

void pickGPU(Vulkan& vk) {
    uint32_t gpuCount = 0;
    VKCHECK(vkEnumeratePhysicalDevices(vk.handle, &gpuCount, nullptr));
    INFO("%d physical device(s)", gpuCount);

    vector<VkPhysicalDevice> gpus(gpuCount);
    VKCHECK(vkEnumeratePhysicalDevices(vk.handle, &gpuCount, gpus.data()));

    for (auto gpu: gpus) {
        bool hasGraphicsQueue = false;
#ifdef VULKAN_COMPUTE
        bool hasComputeQueue = false;
#endif

        VkPhysicalDeviceProperties props = {};
        vkGetPhysicalDeviceProperties(gpu, &props);

        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(
            gpu,
            nullptr,
            &extensionCount,
            nullptr
        );
        vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            gpu,
            nullptr,
            &extensionCount,
            extensions.data()
        );
        bool hasSwapChain = false;
        string swapExtensionName(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        for (auto extension: extensions) {
            string name(extension.extensionName);
            if (name == swapExtensionName) {
                hasSwapChain = true;
            }
        }
        if (!hasSwapChain) {
            continue;
        }

#ifdef VULKAN_MESH_SHADER
        {
            VkPhysicalDeviceMeshShaderFeaturesNV meshFeatures = {};
            meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;

            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
            features.pNext = &meshFeatures;

            vkGetPhysicalDeviceFeatures2(gpu, &features);
            vk.supportsMeshShaders = meshFeatures.meshShader && meshFeatures.taskShader;
            if (vk.supportsMeshShaders) {
                LOG(INFO) << "gpu supports mesh shaders";
            } else {
                LOG(INFO) << "gpu does not support mesh shaders";
            }
        }
#endif
#ifdef VULKAN_PERFORMANCE_COUNTERS
        {
            VkPhysicalDevicePerformanceQueryFeaturesKHR queryFeatures = {};
            queryFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR;
            
            VkPhysicalDeviceFeatures2 features = {};
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
            features.pNext = &queryFeatures;

            vkGetPhysicalDeviceFeatures2(gpu, &features);
            vk.supportsQueryPools =
                queryFeatures.performanceCounterQueryPools
                && queryFeatures.performanceCounterMultipleQueryPools;
            if (vk.supportsQueryPools) {
                LOG(INFO) << "gpu supports query pools";
            } else {
                LOG(INFO) << "gpu does not support query pools";
            }
        }
#endif

        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &familyCount, nullptr);
        vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            gpu,
            &familyCount,
            families.data()
        );

        for (uint32_t index = 0; index < familyCount; index++) {
            auto& queue = families[index];
            if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 isPresentQueue;
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    gpu,
                    index,
                    vk.swap.surface,
                    &isPresentQueue
                );
                if (isPresentQueue) {
                    hasGraphicsQueue = true;
                    vk.queueFamily = index;
                }
            }
#ifdef VULKAN_COMPUTE
            if (queue.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                hasComputeQueue = true;
                vk.computeQueueFamily = index;
            }
#endif
        }

#ifdef VULKAN_COMPUTE
        if (hasGraphicsQueue && hasComputeQueue) {
#else
        if (hasGraphicsQueue) {
#endif
            vk.gpu = gpu;
            INFO("selected physical device %s", props.deviceName);
            return;
        }
    }

    throw runtime_error("no suitable physical device found");
}

PFN_vkVoidFunction getFunction(Vulkan& vk, const char* name) {
    auto result = vkGetDeviceProcAddr(vk.device, name);
    if (result == NULL) {
        ERR("could not find %s", name);
        exit(-1);
    }
    return result;
}

void getFunctions(Vulkan& vk) {
    vk.cmdDrawMeshTasksNV = 
        (PFN_vkCmdDrawMeshTasksNV)
        getFunction(vk, "vkCmdDrawMeshTasksNV");
}

void createDevice(Vulkan& vk) {
    vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    {
        VkDeviceQueueCreateInfo q = {};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueCount = 1;
        q.queueFamilyIndex = vk.queueFamily;
        float prio = 1.f;
        q.pQueuePriorities = &prio;
        queueCreateInfos.push_back(q);
    }

#ifdef VULKAN_COMPUTE
    {
        VkDeviceQueueCreateInfo q = {};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.pNext = nullptr;
        q.flags = 0;
        float prio = 1.f;
        q.pQueuePriorities = &prio;
        q.queueCount = 1;
        q.queueFamilyIndex = vk.computeQueueFamily;
        queueCreateInfos.push_back(q);
    }
#endif

    vector<char*> extensions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

#ifdef VULKAN_MESH_SHADER
//TODO(jan): enabling this flag breaks RenderDoc
    if (vk.supportsMeshShaders) {
        extensions.push_back(VK_NV_MESH_SHADER_EXTENSION_NAME);
    }

    VkPhysicalDeviceMeshShaderFeaturesNV meshFeatures = {};
    meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
    meshFeatures.meshShader = true;
    meshFeatures.taskShader = true;
#endif

    VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
    indexingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexingFeatures.descriptorBindingPartiallyBound = true;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = (void*)(&indexingFeatures);
#ifdef VULKAN_MESH_SHADER
    if (vk.supportsMeshShaders) {
        indexingFeatures.pNext = (void*)(&meshFeatures);
    }
#endif
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(
        queueCreateInfos.size()
    );
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = (uint32_t)extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    VKCHECK(vkCreateDevice(vk.gpu, &createInfo, nullptr, &vk.device));
    vkGetDeviceQueue(vk.device, vk.queueFamily, 0, &vk.queue);
#ifdef VULKAN_COMPUTE
    vkGetDeviceQueue(vk.device, vk.computeQueueFamily, 0, &vk.computeQueue);
#endif
}

void createRenderPass(
    Vulkan& vk,
    bool clear,
    bool prepass,
    VkRenderPass& renderPass
) {
    vector<VkAttachmentDescription> attachments;
    VkAttachmentDescription& color = attachments.emplace_back();
    color.format = vk.swap.format;
    color.samples = (VkSampleCountFlagBits)vk.sampleCountFlags;
    color.loadOp = clear
        ? VK_ATTACHMENT_LOAD_OP_CLEAR
        : VK_ATTACHMENT_LOAD_OP_LOAD;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = clear
        ? VK_IMAGE_LAYOUT_UNDEFINED
        : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.finalLayout = prepass
        ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentDescription& depth = attachments.emplace_back();
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.samples = (VkSampleCountFlagBits)vk.sampleCountFlags;
    depth.loadOp = clear
        ? VK_ATTACHMENT_LOAD_OP_CLEAR
        : VK_ATTACHMENT_LOAD_OP_LOAD;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = clear
        ? VK_IMAGE_LAYOUT_UNDEFINED
        : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription& resolve = attachments.emplace_back();
    resolve.format = vk.swap.format;
    resolve.samples = VK_SAMPLE_COUNT_1_BIT;
    resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

// TODO(jan): can this vector be removed
    vector<VkAttachmentReference> colorReferences;
    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorReferences.push_back(colorReference);

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveReference = {};
    resolveReference.attachment = 2;
    resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vector<VkSubpassDescription> subpasses;
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (uint32_t)colorReferences.size();
    subpass.pColorAttachments = colorReferences.data();
    subpass.pDepthStencilAttachment = &depthReference;
    subpass.pResolveAttachments = &resolveReference;
    subpasses.push_back(subpass);

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = (uint32_t)attachments.size();
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = (uint32_t)subpasses.size();
    createInfo.pSubpasses = subpasses.data();
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    VKCHECK(vkCreateRenderPass(
        vk.device, &createInfo, nullptr, &renderPass
    ));
}

void initVK(Vulkan& vk) {
    pickGPU(vk);
    computeSampleCounts(vk);
    createDevice(vk);
#ifdef VULKAN_MESH_SHADER
    getFunctions(vk);
#endif
    initVKSwapChain(vk);
    vk.memories = getMemories(vk.gpu);
    createUniformBuffer(vk.device, vk.memories, vk.queueFamily, 1024, vk.uniforms);
    createRenderPass(vk, true, false, vk.renderPass);
    createRenderPass(vk, false, false, vk.renderPassNoClear);
    createVulkanColorBuffer(
        vk.device,
        vk.memories,
        vk.swap.extent,
        vk.queueFamily,
        vk.swap.format,
        (VkSampleCountFlagBits)vk.sampleCountFlags,
        vk.color
    );
    createVulkanDepthBuffer(
        vk.device,
        vk.memories,
        vk.swap.extent,
        vk.queueFamily,
        (VkSampleCountFlagBits)vk.sampleCountFlags,
        vk.depth
    );
    createFramebuffers(vk);
    vk.cmdPool = createCommandPool(vk.device, vk.queueFamily);
    vk.cmdPoolTransient = createCommandPool(vk.device, vk.queueFamily, true);
}

#include "Vulkan/Buffer.cpp"
#include "Vulkan/CommandBuffer.cpp"
#include "Vulkan/Descriptors.cpp"
#include "Vulkan/Image.cpp"
#include "Vulkan/Memory.cpp"
#include "Vulkan/Mesh.cpp"
#include "Vulkan/Pipeline.cpp"
#include "Vulkan/Present.cpp"
#include "Vulkan/SwapChain.cpp"
#include "Vulkan/Synch.cpp"
