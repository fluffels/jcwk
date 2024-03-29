#include "Vulkan.h"
#include "vulkan/vulkan_core.h"

void findSwapFormats(Vulkan& vk) {
    VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vk.gpu,
        vk.swap.surface,
        &vk.swap.surfaceCapabilities
    ));

    // TODO(jan): More safely pick format &c here.
    if (!(vk.swap.surfaceCapabilities.supportedUsageFlags &
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        throw runtime_error("surface does not support color attachment");
    }
    if (!(vk.swap.surfaceCapabilities.supportedCompositeAlpha &
          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
        throw runtime_error("surface does not support opaque compositing");
    }

    uint32_t surfaceFormatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk.gpu,
        vk.swap.surface,
        &surfaceFormatCount,
        nullptr
    );
    vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vk.gpu,
        vk.swap.surface,
        &surfaceFormatCount,
        surfaceFormats.data()
    );

    vk.swap.format = surfaceFormats[0].format;
    vk.swap.colorSpace = surfaceFormats[0].colorSpace;
    for (auto format: surfaceFormats) {
        if ((format.colorSpace == VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT) &&
                (format.format == VK_FORMAT_B8G8R8A8_SRGB)) {
            vk.swap.format = format.format;
            vk.swap.colorSpace = format.colorSpace;
            break;
        }
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk.gpu,
        vk.swap.surface,
        &presentModeCount,
        nullptr
    );
    vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vk.gpu,
        vk.swap.surface,
        &presentModeCount,
        presentModes.data()
    );
    vk.swap.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto availablePresentMode: presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            vk.swap.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

    vk.swap.extent = vk.swap.surfaceCapabilities.currentExtent;
}

void createSwapChain(Vulkan& vk) {
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vk.swap.surface;
    createInfo.minImageCount = vk.swap.surfaceCapabilities.minImageCount;
    createInfo.imageExtent = vk.swap.surfaceCapabilities.currentExtent;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageFormat = vk.swap.format;
    createInfo.imageColorSpace = vk.swap.colorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.presentMode = vk.swap.presentMode;
    createInfo.preTransform = vk.swap.surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.clipped = VK_FALSE;

    VKCHECK(vkCreateSwapchainKHR(
        vk.device, &createInfo, nullptr, &vk.swap.handle
    ));
}

void getImages(Vulkan& vk) {
    uint32_t count = 0;
    vkGetSwapchainImagesKHR(vk.device, vk.swap.handle, &count, nullptr);
    vector<VkImage> handles(count);
    vk.swap.images.resize(count);
    VKCHECK(vkGetSwapchainImagesKHR(
        vk.device,
        vk.swap.handle,
        &count,
        handles.data()
    ));
    for (int i = 0; i < handles.size(); i++) {
        vk.swap.images[i].handle = handles[i];
    }
}

void createSwapImageView(Vulkan& vk, VkImage image, VkImageView& view) {
    createView(
        vk.device,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        vk.swap.format,
        VK_IMAGE_ASPECT_COLOR_BIT,
        view
    );
}

void createViews(Vulkan& vk) {
    for (auto& image: vk.swap.images) {
        createSwapImageView(vk, image.handle, image.view);
    }
}

void createFramebuffers(Vulkan& vk) {
    for (auto& image: vk.swap.images) {
        VkImageView imageViews[] = { vk.color.view, vk.depth.view, image.view };
        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.attachmentCount = 3;
        createInfo.pAttachments = imageViews;
        createInfo.renderPass = vk.renderPass;
        createInfo.height = vk.swap.extent.height;
        createInfo.width = vk.swap.extent.width;
        createInfo.layers = 1;
        auto& fb = vk.swap.framebuffers.emplace_back();
        VKCHECK(vkCreateFramebuffer(vk.device, &createInfo, nullptr, &fb));
    }
}

void createSemaphores(Vulkan& vk) {
    vk.swap.imageReady = createSemaphore(vk.device);
    vk.swap.cmdBufferDone = createSemaphore(vk.device);
}

void initVKSwapChain(Vulkan& vk) {
    findSwapFormats(vk);
    createSwapChain(vk);
    getImages(vk);
    createViews(vk);
    createSemaphores(vk);
}
