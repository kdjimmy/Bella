#include "VulkanContext.h"

#include <cstring>
#include <iostream>
#include <set>
#include <type_traits>

template<bool EnableDebug>
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                    VkDebugUtilsMessageTypeFlagsEXT,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void*)
{
    if constexpr (EnableDebug)
    {
        std::cout << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

template<class T, class V>
static std::enable_if_t<std::is_same_v<T, V>, bool>
CheckDemand(const std::vector<T>& vec1, const std::vector<V>& vec2)
{
    constexpr bool IsCString =
        std::is_same_v<std::decay_t<T>, char*> ||
        std::is_same_v<std::decay_t<T>, const char*>;

    for (const auto& a : vec1)
    {
        bool found = false;

        if constexpr (IsCString)
        {
            for (const auto& b : vec2)
            {
                if (a && b && std::strcmp(a, b) == 0)
                {
                    found = true;
                    break;
                }
            }
        }
        else
        {
            for (const auto& b : vec2)
            {
                if (a == b)
                {
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
            return false;
        }
    }

    return true;
}

VulkanContext::VulkanContext(bool enableDebug)
    : m_enableDebug(enableDebug)
{
    if(m_enableDebug)
    {
        m_demandedLayer.emplace_back("VK_LAYER_KHRONOS_validation");
        m_demandedInstanceExtension.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
}

VulkanContext::~VulkanContext()
{
    cleanUp();
}

void VulkanContext::initialize()
{
    createInstance();
    createDebugMessenger();

    if(m_window != nullptr)
    {
        createSurface();
    }

    pickPhysicalDevice();
    createDevice();
}

void VulkanContext::cleanUp()
{
    if (m_device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_device);
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE && m_instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_debugMessenger != VK_NULL_HANDLE && m_createDebugUtilsMessengerEXT != nullptr)
    {
        auto destroyDebugMessenger =
            LoadVkInstanceFunction<PFN_vkDestroyDebugUtilsMessengerEXT>(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebugMessenger != nullptr)
        {
            destroyDebugMessenger(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    m_physicalDevice = VK_NULL_HANDLE;
    m_queues = {};
}

void VulkanContext::setWindow(GLFWwindow* window)
{
    m_window = window;
}

void VulkanContext::createInstance()
{
    if (m_instance != VK_NULL_HANDLE)
    {
        return;
    }

    if(m_enableDebug && !checkLayersSupport(m_demandedLayer))
    {
        throw std::runtime_error("not support demanded layer");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Bella";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
    appInfo.pEngineName = "BellaEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> extensions = m_demandedInstanceExtension;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions != nullptr)
    {
        extensions.insert(extensions.end(), glfwExtensions, glfwExtensions + glfwExtensionCount);
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_demandedLayer.size());
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledLayerNames = m_demandedLayer.data();
    createInfo.ppEnabledExtensionNames = extensions.data();

    if(m_enableDebug)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));
}

void VulkanContext::createDebugMessenger()
{
    if (!m_enableDebug || m_instance == VK_NULL_HANDLE || m_debugMessenger != VK_NULL_HANDLE)
    {
        return;
    }

    m_createDebugUtilsMessengerEXT =
        LoadVkInstanceFunction<PFN_vkCreateDebugUtilsMessengerEXT>(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (m_createDebugUtilsMessengerEXT == nullptr)
    {
        throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT.");
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    VK_CHECK(m_createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger));
}

void VulkanContext::createSurface()
{
    if (m_surface != VK_NULL_HANDLE)
    {
        return;
    }

    if (m_window == nullptr)
    {
        throw std::runtime_error("failed to create window surface: GLFW window is null.");
    }

    if (m_instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to create window surface: Vulkan instance is null.");
    }

    VK_CHECK(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface));
}

void VulkanContext::pickPhysicalDevice()
{
    if (m_physicalDevice != VK_NULL_HANDLE)
    {
        return;
    }

    pickPhysicalDevice(calculateGpuScore);
}

void VulkanContext::createDevice()
{
    if (m_device != VK_NULL_HANDLE)
    {
        return;
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        pickPhysicalDevice();
    }

    findQueueFamilyIndices(m_physicalDevice);

    std::set<int> indices = {m_queues.computeFamily, m_queues.graphicFamily, m_queues.presentFamily};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    for(int idx: indices)
    {
        if(idx < 0)
        {
            continue;
        }

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(idx);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    VkPhysicalDeviceFeatures deviceFeatures{};
    createInfo.pEnabledFeatures = &deviceFeatures;

    std::vector<const char*> deviceExtensions;
    if(m_surface != VK_NULL_HANDLE)
    {
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    AppendVmaDeviceExtensions(deviceExtensions, VK_API_VERSION_1_4);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VK_CHECK(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));
    m_vmaAllocatorFlags = BuildVmaAllocatorCreateFlags(VK_API_VERSION_1_4);

    vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_queues.graphicFamily), 0, &m_queues.graphicQueue);
    vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_queues.computeFamily), 0, &m_queues.computeQueue);
    vkGetDeviceQueue(m_device, static_cast<uint32_t>(m_queues.presentFamily), 0, &m_queues.presentQueue);
}

VkInstance VulkanContext::getInstance() const
{
    return m_instance;
}

VkPhysicalDevice VulkanContext::getPhysicalDevice() const
{
    return m_physicalDevice;
}

VkDevice VulkanContext::getDevice() const
{
    return m_device;
}

VkSurfaceKHR VulkanContext::getSurface() const
{
    return m_surface;
}

const VulkanQueueInfo& VulkanContext::getQueues() const
{
    return m_queues;
}

VmaAllocatorCreateFlags VulkanContext::getVmaAllocatorCreateFlags() const
{
    return m_vmaAllocatorFlags;
}

bool VulkanContext::isDebugEnabled() const
{
    return m_enableDebug;
}

bool VulkanContext::checkLayersSupport(std::vector<const char*>& demandedLayers)
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::vector<const char*> layerNames(layerCount);
    for(uint32_t i = 0; i < layerCount; i++)
    {
        layerNames[i] = availableLayers[i].layerName;
    }

    return CheckDemand(demandedLayers, layerNames);
}

void VulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = m_enableDebug ? DebugCallback<true> : DebugCallback<false>;
}

uint32_t VulkanContext::calculateGpuScore(VkPhysicalDevice physicalDevice)
{
    uint32_t score = 0;
    VkPhysicalDeviceProperties props{};
    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);

    if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 10000;
    }

    score += props.limits.maxImageDimension2D;

    if(features.geometryShader)
    {
        score += 10000;
    }

    if(props.limits.maxComputeWorkGroupInvocations > 0)
    {
        score += 10000;
    }

    return score;
}

void VulkanContext::findQueueFamilyIndices(VkPhysicalDevice physicaldevice)
{
    m_queues = {};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicaldevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicaldevice, &queueFamilyCount, queueFamilies.data());

    int idx = 0;
    for(const auto& family: queueFamilies)
    {
        if((family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            m_queues.graphicFamily = idx;
        }

        if(m_surface != VK_NULL_HANDLE)
        {
            VkBool32 presentSupport = VK_FALSE;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicaldevice, static_cast<uint32_t>(idx), m_surface, &presentSupport));
            if(presentSupport)
            {
                m_queues.presentFamily = idx;
            }
        }

        if((family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
        {
            m_queues.computeFamily = idx;
        }

        if(m_surface == VK_NULL_HANDLE &&
           m_queues.graphicFamily != -1 &&
           m_queues.computeFamily != -1)
        {
            break;
        }

        if(m_surface != VK_NULL_HANDLE &&
           m_queues.computeFamily != -1 &&
           m_queues.graphicFamily != -1 &&
           m_queues.presentFamily != -1)
        {
            break;
        }

        idx++;
    }

    if(m_queues.graphicFamily == -1)
    {
        throw std::runtime_error("failed to find graphics queue family.");
    }

    if(m_queues.computeFamily == -1)
    {
        m_queues.computeFamily = m_queues.graphicFamily;
    }

    if(m_surface == VK_NULL_HANDLE)
    {
        m_queues.presentFamily = m_queues.graphicFamily;
    }
    else if(m_queues.presentFamily == -1)
    {
        throw std::runtime_error("failed to find present queue family.");
    }
}
