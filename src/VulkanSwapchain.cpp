#include "VulkanSwapchain.h"

#include <algorithm>
#include <stdexcept>

VulkanSwapchain::~VulkanSwapchain()
{
    cleanUp();
}

void VulkanSwapchain::create(VkDevice device,
                             VkPhysicalDevice physicalDevice,
                             VkSurfaceKHR surface,
                             const VulkanQueueInfo& queues,
                             uint32_t preferredWidth,
                             uint32_t preferredHeight,
                             uint32_t arrayLayers,
                             std::optional<VkPresentModeKHR> preferredPresentMode,
                             std::optional<VkFormat> preferredFormat,
                             std::optional<VkColorSpaceKHR> preferredColorSpace)
{
    if (device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to create swapchain: Vulkan device is null.");
    }
    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to create swapchain: Vulkan physical device is null.");
    }
    if (surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to create swapchain: Vulkan surface is null.");
    }

    cleanUp();

    m_device = device;
    m_physicalDevice = physicalDevice;
    m_surface = surface;
    m_queues = queues;

    const VulkanSwapchainSupportDetails support = querySupport(m_physicalDevice, m_surface);
    if (support.formats.empty())
    {
        throw std::runtime_error("failed to create swapchain: no surface formats available.");
    }
    if (support.presentModes.empty())
    {
        throw std::runtime_error("failed to create swapchain: no present modes available.");
    }

    const VkSurfaceFormatKHR surfaceFormat =
        chooseSurfaceFormat(support.formats, preferredFormat, preferredColorSpace);
    const VkPresentModeKHR presentMode =
        choosePresentMode(support.presentModes, preferredPresentMode);
    const VkExtent2D extent =
        chooseExtent(support.capabilities, preferredWidth, preferredHeight);
    const uint32_t imageCount =
        chooseImageCount(support.capabilities);
    const VkSurfaceTransformFlagBitsKHR preTransform =
        choosePreTransform(support.capabilities);
    const VkCompositeAlphaFlagBitsKHR compositeAlpha =
        chooseCompositeAlpha(support.capabilities);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = arrayLayers;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {
        static_cast<uint32_t>(queues.graphicFamily),
        static_cast<uint32_t>(queues.presentFamily)
    };
    if (queues.graphicFamily != queues.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = preTransform;
    createInfo.compositeAlpha = compositeAlpha;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain));

    m_format = surfaceFormat.format;
    m_colorSpace = surfaceFormat.colorSpace;
    m_extent = extent;

    uint32_t actualImageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualImageCount, nullptr));
    if (actualImageCount == 0)
    {
        throw std::runtime_error("failed to create swapchain: no images returned.");
    }
    m_images.resize(actualImageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &actualImageCount, m_images.data()));

    m_imageViews.resize(actualImageCount);
    for (size_t i = 0; i < m_images.size(); i++)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = m_images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = m_format;

        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_imageViews[i]));
    }
}

void VulkanSwapchain::recreate(uint32_t preferredWidth,
                               uint32_t preferredHeight,
                               uint32_t arrayLayers,
                               std::optional<VkPresentModeKHR> preferredPresentMode,
                               std::optional<VkFormat> preferredFormat,
                               std::optional<VkColorSpaceKHR> preferredColorSpace)
{
    if (m_device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to recreate swapchain: Vulkan device is null.");
    }
    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to recreate swapchain: Vulkan physical device is null.");
    }
    if (m_surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to recreate swapchain: Vulkan surface is null.");
    }

    vkDeviceWaitIdle(m_device);

    create(
        m_device,
        m_physicalDevice,
        m_surface,
        m_queues,
        preferredWidth,
        preferredHeight,
        arrayLayers,
        preferredPresentMode,
        preferredFormat,
        preferredColorSpace);
}

void VulkanSwapchain::cleanUp()
{
    if(m_device != VK_NULL_HANDLE)
    {
        for (VkImageView imageView : m_imageViews)
        {
            if (imageView != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_device, imageView, nullptr);
            }
        }
    }

    m_imageViews.clear();
    m_images.clear();

    if (m_swapchain != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    }

    m_swapchain = VK_NULL_HANDLE;
    m_format = VK_FORMAT_UNDEFINED;
    m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    m_extent = {};
    m_surface = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_queues = {};
}

VkSwapchainKHR VulkanSwapchain::getHandle() const
{
    return m_swapchain;
}

VkFormat VulkanSwapchain::getFormat() const
{
    return m_format;
}

VkExtent2D VulkanSwapchain::getExtent() const
{
    return m_extent;
}

const std::vector<VkImage>& VulkanSwapchain::getImages() const
{
    return m_images;
}

const std::vector<VkImageView>& VulkanSwapchain::getImageViews() const
{
    return m_imageViews;
}

VulkanSwapchainSupportDetails VulkanSwapchain::querySupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const
{
    VulkanSwapchainSupportDetails details;

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities));

    uint32_t formatCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
    if (formatCount > 0)
    {
        details.formats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data()));
    }

    uint32_t presentModeCount = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
    if (presentModeCount > 0)
    {
        details.presentModes.resize(presentModeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data()));
    }

    return details;
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats,
                                                        std::optional<VkFormat> preferredFormat,
                                                        std::optional<VkColorSpaceKHR> preferredColorSpace) const
{
    const VkFormat targetFormat = preferredFormat.value_or(VK_FORMAT_B8G8R8A8_SRGB);
    const VkColorSpaceKHR targetColorSpace = preferredColorSpace.value_or(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == targetFormat &&
            availableFormat.colorSpace == targetColorSpace)
        {
            return availableFormat;
        }
    }

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == targetFormat)
        {
            return availableFormat;
        }
    }

    return availableFormats.front();
}

VkPresentModeKHR VulkanSwapchain::choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
                                                    std::optional<VkPresentModeKHR> preferredPresentMode) const
{
    if (preferredPresentMode.has_value())
    {
        for (VkPresentModeKHR availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == preferredPresentMode.value())
            {
                return availablePresentMode;
            }
        }
    }

    for (VkPresentModeKHR availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    for (VkPresentModeKHR availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR)
        {
            return availablePresentMode;
        }
    }

    return availablePresentModes.front();
}

VkExtent2D VulkanSwapchain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                         uint32_t preferredWidth,
                                         uint32_t preferredHeight) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {preferredWidth, preferredHeight};
    actualExtent.width = std::clamp(actualExtent.width,
                                    capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
                                     capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);
    return actualExtent;
}

uint32_t VulkanSwapchain::chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }
    return imageCount;
}

VkSurfaceTransformFlagBitsKHR VulkanSwapchain::choosePreTransform(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if ((capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) != 0)
    {
        return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    return capabilities.currentTransform;
}

VkCompositeAlphaFlagBitsKHR VulkanSwapchain::chooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    constexpr VkCompositeAlphaFlagBitsKHR candidates[] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (VkCompositeAlphaFlagBitsKHR candidate : candidates)
    {
        if ((capabilities.supportedCompositeAlpha & candidate) != 0)
        {
            return candidate;
        }
    }

    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}
