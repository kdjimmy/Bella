#pragma once

#include "VulkanCommon.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

struct ResourceHandle
{
    uint32_t kind = 0;         // 0 = buffer, 1 = image
    uint32_t descIndex = 0;    // index into the descriptor vector
    uint32_t resourceIndex = 0;// index into the resource pool for that descriptor
};

struct VmaAllocationObject
{
    VmaAllocation alloc = VK_NULL_HANDLE;
    VmaAllocationInfo allocInfo{};
};

struct SamplerDesc
{
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkFilter minFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkBool32 anisotropyEnable = VK_FALSE;
    float maxAnisotropy = 1.0f;
    VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    VkBool32 unnormalizedCoordinates = VK_FALSE;
    float mipLodBias = 0.0f;
    float minLod = 0.0f;
    float maxLod = 0.0f;
    VkBool32 compareEnable = VK_FALSE;
    VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;

    bool operator==(const SamplerDesc& other) const
    {
        return magFilter == other.magFilter &&
               minFilter == other.minFilter &&
               mipmapMode == other.mipmapMode &&
               addressModeU == other.addressModeU &&
               addressModeV == other.addressModeV &&
               addressModeW == other.addressModeW &&
               anisotropyEnable == other.anisotropyEnable &&
               maxAnisotropy == other.maxAnisotropy &&
               borderColor == other.borderColor &&
               unnormalizedCoordinates == other.unnormalizedCoordinates &&
               mipLodBias == other.mipLodBias &&
               minLod == other.minLod &&
               maxLod == other.maxLod &&
               compareEnable == other.compareEnable &&
               compareOp == other.compareOp;
    }
};

struct SamplerResource
{
    SamplerDesc desc;
    VkSampler sampler = VK_NULL_HANDLE;
    bool inUse = false;
};

struct BufferResourceDesc
{
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO;
    VkMemoryPropertyFlags requiredFlags = 0;
    VkMemoryPropertyFlags preferredFlags = 0;
    bool needMapped = false;

    bool operator==(const BufferResourceDesc& other) const
    {
        return size == other.size &&
               usage == other.usage &&
               memoryUsage == other.memoryUsage &&
               requiredFlags == other.requiredFlags &&
               preferredFlags == other.preferredFlags &&
               needMapped == other.needMapped;
    }
};

struct BufferResource
{
    BufferResourceDesc bufferDesc;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo{};
    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
    uint64_t timelineSemaphoreValue = 0;
    bool inUse = false;
    uint32_t frameId = 0;
};

struct ImageResourceDesc
{
    VkImageType imageType = VK_IMAGE_TYPE_2D;
    VkFormat format = VK_FORMAT_UNDEFINED;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags usage = 0;
    VkImageCreateFlags flags = 0;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO;
    VkMemoryPropertyFlags requiredFlags = 0;
    VkMemoryPropertyFlags preferredFlags = 0;

    bool operator==(const ImageResourceDesc& other) const
    {
        return imageType == other.imageType &&
               format == other.format &&
               width == other.width &&
               height == other.height &&
               depth == other.depth &&
               mipLevels == other.mipLevels &&
               arrayLayers == other.arrayLayers &&
               samples == other.samples &&
               tiling == other.tiling &&
               usage == other.usage &&
               flags == other.flags &&
               memoryUsage == other.memoryUsage &&
               requiredFlags == other.requiredFlags &&
               preferredFlags == other.preferredFlags;
    }

    static ImageResourceDesc fromCreateInfo(const VkImageCreateInfo& createInfo,
                                            const VmaAllocationCreateInfo& allocCreateInfo)
    {
        ImageResourceDesc desc{};
        desc.imageType = createInfo.imageType;
        desc.format = createInfo.format;
        desc.width = createInfo.extent.width;
        desc.height = createInfo.extent.height;
        desc.depth = createInfo.extent.depth;
        desc.mipLevels = createInfo.mipLevels;
        desc.arrayLayers = createInfo.arrayLayers;
        desc.samples = createInfo.samples;
        desc.tiling = createInfo.tiling;
        desc.usage = createInfo.usage;
        desc.flags = createInfo.flags;
        desc.memoryUsage = allocCreateInfo.usage;
        desc.requiredFlags = allocCreateInfo.requiredFlags;
        desc.preferredFlags = allocCreateInfo.preferredFlags;
        return desc;
    }
};

struct ImageResource
{
    ImageResourceDesc imageDesc;
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo{};
    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore timelineSemaphore = VK_NULL_HANDLE;
    uint64_t timelineSemaphoreValue = 0;
    bool inUse = false;
    uint32_t frameId = 0;
};

class VulkanResourceManager
{
    public:
        VulkanResourceManager() = default;
        VulkanResourceManager(VkPhysicalDevice physicalDevice, VkDevice device, VkInstance instance);
        VulkanResourceManager(VkPhysicalDevice* physicalDevice, VkDevice* device, VkInstance* instance);
        ~VulkanResourceManager();

        bool init(VmaAllocatorCreateFlags flags = 0);
        void cleanUp();

        VmaAllocationObject allocateMemory(const VkMemoryRequirements& memReq,
                                           VmaAllocationCreateFlags flags,
                                           VmaMemoryUsage usage,
                                           VmaPool* pool = nullptr,
                                           float priority = 0.0f);

        ResourceHandle createBuffer(const VkBufferCreateInfo* bufferCreateInfo,
                                    const VmaAllocationCreateInfo* allocationCreateInfo,
                                    VkBuffer* outBuffer = nullptr,
                                    VmaAllocation* outAllocation = nullptr,
                                    VmaAllocationInfo* outAllocationInfo = nullptr);

        void releaseBuffer(const ResourceHandle& handle);
        VkBuffer getBuffer(const ResourceHandle& handle);
        VmaAllocation getBufferAllocation(const ResourceHandle& handle);
        BufferResource* getBufferResource(const ResourceHandle& handle);
        const BufferResource* getBufferResource(const ResourceHandle& handle) const;

        ResourceHandle createImage(const VkImageCreateInfo* imageCreateInfo,
                                   const VmaAllocationCreateInfo* allocationCreateInfo,
                                   VkImage* outImage = nullptr,
                                   VmaAllocation* outAllocation = nullptr,
                                   VmaAllocationInfo* outAllocationInfo = nullptr);

        void releaseImage(const ResourceHandle& handle);
        VkImage getImage(const ResourceHandle& handle);
        VmaAllocation getImageAllocation(const ResourceHandle& handle);
        ImageResource* getImageResource(const ResourceHandle& handle);
        const ImageResource* getImageResource(const ResourceHandle& handle) const;

    private:
        uint32_t getOrCreateBufferDescIndex(const BufferResourceDesc& desc);
        std::vector<std::unique_ptr<BufferResource>>& getBufferResourcePool(uint32_t descIndex);
        const std::vector<std::unique_ptr<BufferResource>>& getBufferResourcePool(uint32_t descIndex) const;
        uint32_t getOrCreateImageDescIndex(const ImageResourceDesc& desc);
        std::vector<std::unique_ptr<ImageResource>>& getImageResourcePool(uint32_t descIndex);
        const std::vector<std::unique_ptr<ImageResource>>& getImageResourcePool(uint32_t descIndex) const;

    private:
        mutable std::mutex resourceAllocatorMutex;
        mutable std::mutex resourceTableMutex;

        VmaAllocator m_allocator = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkInstance m_instance = VK_NULL_HANDLE;

        std::vector<BufferResourceDesc> m_bufferDescs;
        std::vector<ImageResourceDesc> m_imageDescs;
        std::vector<std::vector<std::unique_ptr<BufferResource>>> m_bufferResourcePools;
        std::vector<std::vector<std::unique_ptr<ImageResource>>> m_imageResourcePools;
};
