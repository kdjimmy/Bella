#pragma once

#include "VulkanCommon.h"
#include <optional>
#include <vector>

struct VulkanSwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchain
{
    public:
        VulkanSwapchain() = default;
        ~VulkanSwapchain();

        void create(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    VkSurfaceKHR surface,
                    const VulkanQueueInfo& queues,
                    uint32_t preferredWidth,
                    uint32_t preferredHeight,
                    uint32_t arrayLayers = 1,
                    std::optional<VkPresentModeKHR> preferredPresentMode = std::nullopt,
                    std::optional<VkFormat> preferredFormat = std::nullopt,
                    std::optional<VkColorSpaceKHR> preferredColorSpace = std::nullopt);
        void recreate(uint32_t preferredWidth,
                      uint32_t preferredHeight,
                      uint32_t arrayLayers = 1,
                      std::optional<VkPresentModeKHR> preferredPresentMode = std::nullopt,
                      std::optional<VkFormat> preferredFormat = std::nullopt,
                      std::optional<VkColorSpaceKHR> preferredColorSpace = std::nullopt);
        void cleanUp();

        VkSwapchainKHR getHandle() const;
        VkFormat getFormat() const;
        VkExtent2D getExtent() const;
        const std::vector<VkImage>& getImages() const;
        const std::vector<VkImageView>& getImageViews() const;

    private:
        VulkanSwapchainSupportDetails querySupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const;
        VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats,
                                               std::optional<VkFormat> preferredFormat,
                                               std::optional<VkColorSpaceKHR> preferredColorSpace) const;
        VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes,
                                           std::optional<VkPresentModeKHR> preferredPresentMode) const;
        VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                uint32_t preferredWidth,
                                uint32_t preferredHeight) const;
        uint32_t chooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities) const;
        VkSurfaceTransformFlagBitsKHR choosePreTransform(const VkSurfaceCapabilitiesKHR& capabilities) const;
        VkCompositeAlphaFlagBitsKHR chooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) const;

        VkDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VulkanQueueInfo m_queues;
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        VkExtent2D m_extent{};
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
};
