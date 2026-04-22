#pragma once

#include "VulkanCommon.h"
#include <GLFW/glfw3.h>
#include <vector>

class VulkanContext
{
    public:
        explicit VulkanContext(bool enableDebug = false);
        ~VulkanContext();

        void initialize();
        void cleanUp();

        void setWindow(GLFWwindow* window);
        void createInstance();
        void createDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createDevice();

        VkInstance getInstance() const;
        VkPhysicalDevice getPhysicalDevice() const;
        VkDevice getDevice() const;
        VkSurfaceKHR getSurface() const;
        const VulkanQueueInfo& getQueues() const;
        VmaAllocatorCreateFlags getVmaAllocatorCreateFlags() const;
        bool isDebugEnabled() const;

    private:
        bool checkLayersSupport(std::vector<const char*>& layerName);
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        static uint32_t calculateGpuScore(VkPhysicalDevice physicalDevice);
        void findQueueFamilyIndices(VkPhysicalDevice physicaldevice);

        template<class T>
        void pickPhysicalDevice(T func)
        {
            uint32_t deviceCount = 0;
            VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr));
            if(deviceCount == 0)
            {
                throw std::runtime_error("no graphic cards found.");
            }

            std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
            VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data()));

            uint32_t bestScore = 0;
            for(auto& physicalDevice: physicalDevices)
            {
                auto score = func(physicalDevice);
                if(score > bestScore)
                {
                    bestScore = score;
                    m_physicalDevice = physicalDevice;
                }
            }

            if(m_physicalDevice == VK_NULL_HANDLE)
            {
                throw std::runtime_error("failed to pick gpu.");
            }
        }

    private:
        bool m_enableDebug = false;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
        VulkanQueueInfo m_queues;
        VkDevice m_device = VK_NULL_HANDLE;
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        GLFWwindow* m_window = nullptr;
        std::vector<const char*> m_demandedLayer;
        std::vector<const char*> m_demandedInstanceExtension;
        VmaAllocatorCreateFlags m_vmaAllocatorFlags = 0;
        PFN_vkCreateDebugUtilsMessengerEXT m_createDebugUtilsMessengerEXT = nullptr;
};
