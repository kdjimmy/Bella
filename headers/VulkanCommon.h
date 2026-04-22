#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err != VK_SUCCESS) {                                        \
            throw std::runtime_error(                                   \
                std::string("Vulkan error: ") +                         \
                std::to_string(err) +                                   \
                " at " + __FILE__ + ":" + std::to_string(__LINE__)      \
            );                                                          \
        }                                                               \
    } while (0)

template<typename T>
inline T LoadVkInstanceFunction(VkInstance instance, const char* functionName)
{
    return reinterpret_cast<T>(vkGetInstanceProcAddr(instance, functionName));
}

struct VulkanQueueInfo
{
    int graphicFamily = -1;
    int presentFamily = -1;
    int computeFamily = -1;
    VkQueue computeQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkQueue graphicQueue = VK_NULL_HANDLE;
};

// VMA-related extension toggles.
// Extensions that additionally require explicit VkPhysicalDevice*Features remain opt-in.
#ifndef BELLA_VMA_ENABLE_DEDICATED_ALLOCATION
#define BELLA_VMA_ENABLE_DEDICATED_ALLOCATION 1
#endif

#ifndef BELLA_VMA_ENABLE_BIND_MEMORY2
#define BELLA_VMA_ENABLE_BIND_MEMORY2 1
#endif

#ifndef BELLA_VMA_ENABLE_MEMORY_BUDGET
#define BELLA_VMA_ENABLE_MEMORY_BUDGET 1
#endif

#ifndef BELLA_VMA_ENABLE_BUFFER_DEVICE_ADDRESS
#define BELLA_VMA_ENABLE_BUFFER_DEVICE_ADDRESS 1
#endif

#ifndef BELLA_VMA_ENABLE_MEMORY_PRIORITY
#define BELLA_VMA_ENABLE_MEMORY_PRIORITY 1
#endif

#ifndef BELLA_VMA_ENABLE_MAINTENANCE4
#define BELLA_VMA_ENABLE_MAINTENANCE4 1
#endif

#ifndef BELLA_VMA_ENABLE_MAINTENANCE5
#define BELLA_VMA_ENABLE_MAINTENANCE5 1
#endif

#ifndef BELLA_VMA_ENABLE_EXTERNAL_MEMORY_WIN32
#define BELLA_VMA_ENABLE_EXTERNAL_MEMORY_WIN32 1
#endif

inline void AppendVmaDeviceExtensions(std::vector<const char*>& extensions, uint32_t apiVersion)
{
#if BELLA_VMA_ENABLE_DEDICATED_ALLOCATION
#if VK_KHR_get_memory_requirements2 && VK_KHR_dedicated_allocation
    if (apiVersion < VK_API_VERSION_1_1)
    {
        extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
        extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
    }
#endif
#endif

#if BELLA_VMA_ENABLE_BIND_MEMORY2
#if VK_KHR_bind_memory2
    if (apiVersion < VK_API_VERSION_1_1)
    {
        extensions.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
    }
#endif
#endif

#if BELLA_VMA_ENABLE_MEMORY_BUDGET
#if VK_EXT_memory_budget
    extensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
#endif
#endif

#if BELLA_VMA_ENABLE_BUFFER_DEVICE_ADDRESS
#if VK_KHR_buffer_device_address
    if (apiVersion < VK_API_VERSION_1_2)
    {
        extensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    }
#endif
#endif

#if BELLA_VMA_ENABLE_MEMORY_PRIORITY
#if VK_EXT_memory_priority
    extensions.push_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
#endif
#endif

#if BELLA_VMA_ENABLE_MAINTENANCE4
#if VK_KHR_maintenance4
    if (apiVersion < VK_API_VERSION_1_3)
    {
        extensions.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
    }
#endif
#endif

#if BELLA_VMA_ENABLE_MAINTENANCE5
#if VK_KHR_maintenance5
    extensions.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
#endif
#endif

#if BELLA_VMA_ENABLE_EXTERNAL_MEMORY_WIN32
#if defined(_WIN32) && VK_KHR_external_memory_win32
    extensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#endif
#endif
}

inline VmaAllocatorCreateFlags BuildVmaAllocatorCreateFlags(uint32_t apiVersion)
{
    VmaAllocatorCreateFlags flags = 0;

#if BELLA_VMA_ENABLE_DEDICATED_ALLOCATION
#if VK_KHR_get_memory_requirements2 && VK_KHR_dedicated_allocation
    if (apiVersion < VK_API_VERSION_1_1)
    {
        flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
    }
#endif
#endif

#if BELLA_VMA_ENABLE_BIND_MEMORY2
#if VK_KHR_bind_memory2
    if (apiVersion < VK_API_VERSION_1_1)
    {
        flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
    }
#endif
#endif

#if BELLA_VMA_ENABLE_MEMORY_BUDGET
#if VK_EXT_memory_budget
    flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
#endif
#endif

#if BELLA_VMA_ENABLE_BUFFER_DEVICE_ADDRESS
#if VK_KHR_buffer_device_address || VMA_VULKAN_VERSION >= 1002000
    flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
#endif
#endif

#if BELLA_VMA_ENABLE_MEMORY_PRIORITY
#if VK_EXT_memory_priority
    flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
#endif
#endif

#if BELLA_VMA_ENABLE_MAINTENANCE4
#if VK_KHR_maintenance4
    if (apiVersion < VK_API_VERSION_1_3)
    {
        flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    }
#endif
#endif

#if BELLA_VMA_ENABLE_MAINTENANCE5
#if VK_KHR_maintenance5
    flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;
#endif
#endif

#if BELLA_VMA_ENABLE_EXTERNAL_MEMORY_WIN32
#if defined(_WIN32) && VK_KHR_external_memory_win32
    flags |= VMA_ALLOCATOR_CREATE_KHR_EXTERNAL_MEMORY_WIN32_BIT;
#endif
#endif

    return flags;
}
