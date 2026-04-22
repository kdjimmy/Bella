#include "VulkanResourceManager.h"

#include <algorithm>
#include <stdexcept>

VulkanResourceManager::VulkanResourceManager(VkPhysicalDevice physicalDevice,
                                             VkDevice device,
                                             VkInstance instance)
    : m_physicalDevice(physicalDevice),
      m_device(device),
      m_instance(instance)
{
}

VulkanResourceManager::VulkanResourceManager(VkPhysicalDevice* physicalDevice,
                                             VkDevice* device,
                                             VkInstance* instance)
{
    if (physicalDevice != nullptr)
    {
        m_physicalDevice = *physicalDevice;
    }
    if (device != nullptr)
    {
        m_device = *device;
    }
    if (instance != nullptr)
    {
        m_instance = *instance;
    }
}

VulkanResourceManager::~VulkanResourceManager()
{
    cleanUp();
}

bool VulkanResourceManager::init(VmaAllocatorCreateFlags flags)
{
    std::lock_guard<std::mutex> allocatorLock(resourceAllocatorMutex);

    if (m_allocator != VK_NULL_HANDLE)
    {
        return true;
    }
    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to initialize VulkanResourceManager: physical device is null.");
    }
    if (m_device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to initialize VulkanResourceManager: device is null.");
    }
    if (m_instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to initialize VulkanResourceManager: instance is null.");
    }

    VmaAllocatorCreateInfo createInfo{};
    createInfo.physicalDevice = m_physicalDevice;
    createInfo.device = m_device;
    createInfo.instance = m_instance;
    createInfo.flags = flags;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VK_CHECK(vmaCreateAllocator(&createInfo, &m_allocator));
    return true;
}

void VulkanResourceManager::cleanUp()
{
    std::lock_guard<std::mutex> tableLock(resourceTableMutex);
    std::lock_guard<std::mutex> allocatorLock(resourceAllocatorMutex);

    if (m_allocator != VK_NULL_HANDLE)
    {
        for (auto& resources : m_bufferResourcePools)
        {
            for (auto& resource : resources)
            {
                if (resource && resource->buffer != VK_NULL_HANDLE && resource->allocation != VK_NULL_HANDLE)
                {
                    vmaDestroyBuffer(m_allocator, resource->buffer, resource->allocation);
                    resource->buffer = VK_NULL_HANDLE;
                    resource->allocation = VK_NULL_HANDLE;
                }
            }
        }

        for (auto& resources : m_imageResourcePools)
        {
            for (auto& resource : resources)
            {
                if (resource && resource->image != VK_NULL_HANDLE && resource->allocation != VK_NULL_HANDLE)
                {
                    vmaDestroyImage(m_allocator, resource->image, resource->allocation);
                    resource->image = VK_NULL_HANDLE;
                    resource->allocation = VK_NULL_HANDLE;
                }
            }
        }

        m_imageResourcePools.clear();
        m_bufferResourcePools.clear();
        m_imageDescs.clear();
        m_bufferDescs.clear();

        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }
}

VmaAllocationObject VulkanResourceManager::allocateMemory(const VkMemoryRequirements& memReq,
                                                          VmaAllocationCreateFlags flags,
                                                          VmaMemoryUsage usage,
                                                          VmaPool* pool,
                                                          float priority)
{
    std::lock_guard<std::mutex> allocatorLock(resourceAllocatorMutex);

    if (m_allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to allocate memory: allocator is null.");
    }

    VmaAllocationCreateInfo createInfo{};
    createInfo.flags = flags;
    createInfo.usage = usage;
    createInfo.priority = priority;
    if (pool != nullptr)
    {
        createInfo.pool = *pool;
    }

    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo{};
    VK_CHECK(vmaAllocateMemory(m_allocator, &memReq, &createInfo, &allocation, &allocationInfo));
    return {allocation, allocationInfo};
}

uint32_t VulkanResourceManager::getOrCreateBufferDescIndex(const BufferResourceDesc& desc)
{
    for (uint32_t i = 0; i < m_bufferDescs.size(); ++i)
    {
        if (m_bufferDescs[i] == desc)
        {
            return i;
        }
    }

    const uint32_t index = static_cast<uint32_t>(m_bufferDescs.size());
    m_bufferDescs.push_back(desc);
    m_bufferResourcePools.emplace_back();
    return index;
}

std::vector<std::unique_ptr<BufferResource>>& VulkanResourceManager::getBufferResourcePool(uint32_t descIndex)
{
    if (descIndex >= m_bufferResourcePools.size())
    {
        throw std::runtime_error("failed to get buffer resource pool: desc index out of range.");
    }

    return m_bufferResourcePools[descIndex];
}

const std::vector<std::unique_ptr<BufferResource>>& VulkanResourceManager::getBufferResourcePool(uint32_t descIndex) const
{
    if (descIndex >= m_bufferResourcePools.size())
    {
        throw std::runtime_error("failed to get buffer resource pool: desc index out of range.");
    }

    return m_bufferResourcePools[descIndex];
}

ResourceHandle VulkanResourceManager::createBuffer(const VkBufferCreateInfo* bufferCreateInfo,
                                                   const VmaAllocationCreateInfo* allocationCreateInfo,
                                                   VkBuffer* outBuffer,
                                                   VmaAllocation* outAllocation,
                                                   VmaAllocationInfo* outAllocationInfo)
{
    if (bufferCreateInfo == nullptr)
    {
        throw std::runtime_error("failed to create buffer: buffer create info is null.");
    }
    if (allocationCreateInfo == nullptr)
    {
        throw std::runtime_error("failed to create buffer: allocation create info is null.");
    }
    if (m_allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to create buffer: allocator is null.");
    }

    BufferResourceDesc bufferDesc{};
    bufferDesc.size = bufferCreateInfo->size;
    bufferDesc.usage = bufferCreateInfo->usage;
    bufferDesc.memoryUsage = allocationCreateInfo->usage;
    bufferDesc.requiredFlags = allocationCreateInfo->requiredFlags;
    bufferDesc.preferredFlags = allocationCreateInfo->preferredFlags;
    bufferDesc.needMapped =
        (allocationCreateInfo->flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;

    std::lock_guard<std::mutex> tableLock(resourceTableMutex);
    const uint32_t descIndex = getOrCreateBufferDescIndex(bufferDesc);
    auto& resourcePool = getBufferResourcePool(descIndex);

    for (uint32_t i = 0; i < resourcePool.size(); ++i)
    {
        BufferResource* resource = resourcePool[i].get();
        if (resource == nullptr)
        {
            continue;
        }
        if (resource->timelineSemaphore != VK_NULL_HANDLE)
        {
            uint64_t value = 0;
            VkResult result = vkGetSemaphoreCounterValue(m_device, resource->timelineSemaphore, &value);
            if (result != VK_SUCCESS || value < resource->timelineSemaphoreValue)
            {
                continue;
            }
        }
        if (resource->fence != VK_NULL_HANDLE)
        {
            VkResult result = vkGetFenceStatus(m_device, resource->fence);
            if (result != VK_SUCCESS)
            {
                continue;
            }
        }
        if (!resource->inUse)
        {
            resource->inUse = true;
            if (outBuffer != nullptr)
            {
                *outBuffer = resource->buffer;
            }
            if (outAllocation != nullptr)
            {
                *outAllocation = resource->allocation;
            }
            if (outAllocationInfo != nullptr)
            {
                *outAllocationInfo = resource->allocationInfo;
            }

            return {0u, descIndex, i};
        }
    }

    auto resource = std::make_unique<BufferResource>();
    resource->bufferDesc = bufferDesc;
    resource->inUse = true;

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo{};

    {
        std::lock_guard<std::mutex> allocatorLock(resourceAllocatorMutex);
        VK_CHECK(vmaCreateBuffer(
            m_allocator,
            bufferCreateInfo,
            allocationCreateInfo,
            &buffer,
            &allocation,
            &allocationInfo));
    }

    resource->buffer = buffer;
    resource->allocation = allocation;
    resource->allocationInfo = allocationInfo;

    if (outBuffer != nullptr)
    {
        *outBuffer = buffer;
    }
    if (outAllocation != nullptr)
    {
        *outAllocation = allocation;
    }
    if (outAllocationInfo != nullptr)
    {
        *outAllocationInfo = allocationInfo;
    }

    resourcePool.push_back(std::move(resource));
    return {0u, descIndex, static_cast<uint32_t>(resourcePool.size() - 1)};
}

void VulkanResourceManager::releaseBuffer(const ResourceHandle& handle)
{
    if (handle.kind != 0u)
    {
        throw std::runtime_error("failed to release buffer: handle kind is not buffer.");
    }

    std::lock_guard<std::mutex> tableLock(resourceTableMutex);
    BufferResource* resource = getBufferResource(handle);
    if (resource == nullptr)
    {
        throw std::runtime_error("failed to release buffer: invalid buffer handle.");
    }

    resource->inUse = false;
}

VkBuffer VulkanResourceManager::getBuffer(const ResourceHandle& handle)
{
    std::lock_guard<std::mutex> tableLock(resourceTableMutex);
    BufferResource* resource = getBufferResource(handle);
    if (resource == nullptr)
    {
        throw std::runtime_error("failed to get buffer: invalid buffer handle.");
    }

    return resource->buffer;
}

VmaAllocation VulkanResourceManager::getBufferAllocation(const ResourceHandle& handle)
{
    std::lock_guard<std::mutex> tableLock(resourceTableMutex);
    BufferResource* resource = getBufferResource(handle);
    if (resource == nullptr)
    {
        throw std::runtime_error("failed to get buffer allocation: invalid buffer handle.");
    }

    return resource->allocation;
}

BufferResource* VulkanResourceManager::getBufferResource(const ResourceHandle& handle)
{
    if (handle.kind != 0u)
    {
        return nullptr;
    }
    if (handle.descIndex >= m_bufferDescs.size())
    {
        return nullptr;
    }

    const auto& resourcePool = getBufferResourcePool(handle.descIndex);
    if (handle.resourceIndex >= resourcePool.size())
    {
        return nullptr;
    }

    return resourcePool[handle.resourceIndex].get();
}

const BufferResource* VulkanResourceManager::getBufferResource(const ResourceHandle& handle) const
{
    return const_cast<VulkanResourceManager*>(this)->getBufferResource(handle);
}

uint32_t VulkanResourceManager::getOrCreateImageDescIndex(const ImageResourceDesc& desc)
{
    for (uint32_t i = 0; i < m_imageDescs.size(); ++i)
    {
        if (m_imageDescs[i] == desc)
        {
            return i;
        }
    }

    const uint32_t index = static_cast<uint32_t>(m_imageDescs.size());
    m_imageDescs.push_back(desc);
    m_imageResourcePools.emplace_back();
    return index;
}

std::vector<std::unique_ptr<ImageResource>>& VulkanResourceManager::getImageResourcePool(uint32_t descIndex)
{
    if (descIndex >= m_imageResourcePools.size())
    {
        throw std::runtime_error("failed to get image resource pool: desc index out of range.");
    }

    return m_imageResourcePools[descIndex];
}

const std::vector<std::unique_ptr<ImageResource>>& VulkanResourceManager::getImageResourcePool(uint32_t descIndex) const
{
    if (descIndex >= m_imageResourcePools.size())
    {
        throw std::runtime_error("failed to get image resource pool: desc index out of range.");
    }

    return m_imageResourcePools[descIndex];
}

ResourceHandle VulkanResourceManager::createImage(const VkImageCreateInfo* imageCreateInfo,
                                                  const VmaAllocationCreateInfo* allocationCreateInfo,
                                                  VkImage* outImage,
                                                  VmaAllocation* outAllocation,
                                                  VmaAllocationInfo* outAllocationInfo)
{
    if (imageCreateInfo == nullptr)
    {
        throw std::runtime_error("failed to create image: image create info is null.");
    }
    if (allocationCreateInfo == nullptr)
    {
        throw std::runtime_error("failed to create image: allocation create info is null.");
    }
    if (m_allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to create image: allocator is null.");
    }

    const ImageResourceDesc imageDesc =
        ImageResourceDesc::fromCreateInfo(*imageCreateInfo, *allocationCreateInfo);

    std::lock_guard<std::mutex> tableLock(resourceTableMutex);
    const uint32_t descIndex = getOrCreateImageDescIndex(imageDesc);
    auto& resourcePool = getImageResourcePool(descIndex);

    for (uint32_t i = 0; i < resourcePool.size(); ++i)
    {
        ImageResource* resource = resourcePool[i].get();
        if (resource == nullptr)
        {
            continue;
        }
        if(resource->timelineSemaphore != VK_NULL_HANDLE)
        {
            uint64_t value = 0;
            VkResult result = vkGetSemaphoreCounterValue(m_device, resource->timelineSemaphore, &value);
            // Reuse is only safe after the tracked timeline value has completed.
            if(result != VK_SUCCESS || value < resource->timelineSemaphoreValue)
            {
               continue;
            }
        }
        if(resource->fence != VK_NULL_HANDLE)
        {
            VkResult result = vkGetFenceStatus(m_device, resource->fence);
            // Fences are valid only after the corresponding submit has signaled.
            if(result != VK_SUCCESS)
            {
               continue;
            }
        }
        if (!resource->inUse)
        {
            resource->inUse = true;
            if (outImage != nullptr)
            {
                *outImage = resource->image;
            }
            if (outAllocation != nullptr)
            {
                *outAllocation = resource->allocation;
            }
            if (outAllocationInfo != nullptr)
            {
                *outAllocationInfo = resource->allocationInfo;
            }

            return {1u, descIndex, i};
        }
    }

    auto resource = std::make_unique<ImageResource>();
    resource->imageDesc = imageDesc;
    resource->inUse = true;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo{};

    {
        std::lock_guard<std::mutex> allocatorLock(resourceAllocatorMutex);
        VK_CHECK(vmaCreateImage(
            m_allocator,
            imageCreateInfo,
            allocationCreateInfo,
            &image,
            &allocation,
            &allocationInfo));
    }

    resource->image = image;
    resource->allocation = allocation;
    resource->allocationInfo = allocationInfo;

    if (outImage != nullptr)
    {
        *outImage = image;
    }
    if (outAllocation != nullptr)
    {
        *outAllocation = allocation;
    }
    if (outAllocationInfo != nullptr)
    {
        *outAllocationInfo = allocationInfo;
    }

    resourcePool.push_back(std::move(resource));
    return {1u, descIndex, static_cast<uint32_t>(resourcePool.size() - 1)};
}

void VulkanResourceManager::releaseImage(const ResourceHandle& handle)
{
    if (handle.kind != 1u)
    {
        throw std::runtime_error("failed to release image: handle kind is not image.");
    }

    std::lock_guard<std::mutex> tableLock(resourceTableMutex);
    ImageResource* resource = getImageResource(handle);
    if (resource == nullptr)
    {
        throw std::runtime_error("failed to release image: invalid image handle.");
    }

    resource->inUse = false;
}

ImageResource* VulkanResourceManager::getImageResource(const ResourceHandle& handle)
{
    if (handle.kind != 1u)
    {
        return nullptr;
    }
    if (handle.descIndex >= m_imageDescs.size())
    {
        return nullptr;
    }

    const auto& resourcePool = getImageResourcePool(handle.descIndex);
    if (handle.resourceIndex >= resourcePool.size())
    {
        return nullptr;
    }

    return resourcePool[handle.resourceIndex].get();
}

const ImageResource* VulkanResourceManager::getImageResource(const ResourceHandle& handle) const
{
    return const_cast<VulkanResourceManager*>(this)->getImageResource(handle);
}

VkImage VulkanResourceManager::getImage(const ResourceHandle& handle)
{
   std::lock_guard<std::mutex> tableLock(resourceTableMutex);
   ImageResource* resource = getImageResource(handle);
   if (resource == nullptr)
   {
      throw std::runtime_error("failed to get image: invalid image handle.");
   }

   // Reuse the validated handle lookup path instead of duplicating index checks.
   return resource->image;
}

VmaAllocation VulkanResourceManager::getImageAllocation(const ResourceHandle& handle)
{
   std::lock_guard<std::mutex> tableLock(resourceTableMutex);
   ImageResource* resource = getImageResource(handle);
   if (resource == nullptr)
   {
      throw std::runtime_error("failed to get image allocation: invalid image handle.");
   }

   // Allocation lifetime is owned by the ImageResource entry, so fetch it through the same entry.
   return resource->allocation;
}
