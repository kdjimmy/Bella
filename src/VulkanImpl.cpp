#include "VulkanImpl.h"

#include <iostream>
#include <optional>

VulkanImpl::VulkanImpl()
{
    std::cout << "creating vulkanimpl" << std::endl;
}

VulkanImpl::~VulkanImpl()
{
    cleanUp();
    std::cout << "deleting vulkanimpl" << std::endl;
}

void VulkanImpl::initialize()
{
    m_context.initialize();

    if(m_context.getSurface() != VK_NULL_HANDLE)
    {
        createSwapChain(1920, 1680, 1);
    }
}

void VulkanImpl::cleanUp()
{
    m_swapchain.cleanUp();
    m_context.cleanUp();
}

void VulkanImpl::run()
{
}

void VulkanImpl::render()
{
}

void VulkanImpl::setWindow(GLFWwindow* window)
{
    m_context.setWindow(window);
}

void VulkanImpl::createInstance()
{
    m_context.createInstance();
}

void VulkanImpl::createDebugMessenger()
{
    m_context.createDebugMessenger();
}

void VulkanImpl::createSurface()
{
    m_context.createSurface();
}

void VulkanImpl::createDevice()
{
    m_context.createDevice();
}

void VulkanImpl::createSwapChain(uint32_t preferredWidth, uint32_t preferredHeight, int arrayLayers)
{
    m_swapchain.create(
        m_context.getDevice(),
        m_context.getPhysicalDevice(),
        m_context.getSurface(),
        m_context.getQueues(),
        preferredWidth,
        preferredHeight,
        static_cast<uint32_t>(arrayLayers),
        std::nullopt,
        std::nullopt,
        std::nullopt);
}

void VulkanImpl::recreateSwapChain(uint32_t preferredWidth, uint32_t preferredHeight, int arrayLayers)
{
    m_swapchain.cleanUp();
    createSwapChain(preferredWidth, preferredHeight, arrayLayers);
}

VulkanContext& VulkanImpl::getContext()
{
    return m_context;
}

const VulkanContext& VulkanImpl::getContext() const
{
    return m_context;
}

VulkanSwapchain& VulkanImpl::getSwapchain()
{
    return m_swapchain;
}

const VulkanSwapchain& VulkanImpl::getSwapchain() const
{
    return m_swapchain;
}
