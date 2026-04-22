#pragma once

#include "BaseImpl.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include <GLFW/glfw3.h>

class VulkanImpl: public BaseImpl
{
    public:
        VulkanImpl();
        ~VulkanImpl() override;

        void initialize() override;
        void cleanUp() override;
        void run() override;
        void render() override;

        void setWindow(GLFWwindow* window);

        void createInstance();
        void createDebugMessenger();
        void createSurface();
        void createDevice();
        void createSwapChain(uint32_t preferredWidth, uint32_t preferredHeight, int arrayLayers = 1);
        void recreateSwapChain(uint32_t preferredWidth, uint32_t preferredHeight, int arrayLayers = 1);

        VulkanContext& getContext();
        const VulkanContext& getContext() const;
        VulkanSwapchain& getSwapchain();
        const VulkanSwapchain& getSwapchain() const;

    private:
        VulkanContext m_context;
        VulkanSwapchain m_swapchain;
};
