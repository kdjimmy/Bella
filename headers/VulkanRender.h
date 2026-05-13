#pragma once
#include "vulkan/vulkan.h"
#include "VulkanCommon.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanResourceManager.h"
#include "VulkanRenderPass.h"
#include <vector>
#include <cstring>
#include <unordered_map>


struct RenderPassInfo
{
    uint32_t frameIndex = 0;
    uint32_t imageIndex = 0;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkExtent2D extent{};
    VkRect2D renderArea{};
    std::vector<VkClearValue> clearValues;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<BufferResource> vertexBuffers;
    std::vector<VkDeviceSize> vertexBufferOffsets;
    BufferResource indexBuffer;
    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 1;

    std::vector<VkSemaphore> semaphores;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
};

class VulkanRender
{
    public:
        void createCommandPool(uint32_t familyIndex, VkCommandPoolCreateFlags flag = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        std::vector<VkCommandBuffer> createCommandBuffer(VkCommandBufferLevel level, uint32_t commandBufferCount);
        void recordCommandBuffers(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkExtent2D swapchainExtent, VkFramebuffer framebuffer);
        void submitQueue(const std::vector<VkCommandBuffer>& commandBuffers, VkQueue& queue);


        void registerPass(const std::string& passName, VulkanRenderPass& renderPass);           //bind the renderpass first
        void bindPassBuffer(const std::string& passName, const std::vector<BufferResource> &vertexBufferResources, 
            const BufferResource &indexBufferResource, uint32_t indexCount, uint32_t vertexCount = 0, uint32_t instanceCount = 1);
        void registerCommandBuffer(const std::string& passName, VkCommandBuffer commandBuffer);
        void registerPipeline(const std::string& passName, VkPipeline pipeline);
        void registerSyncObject(const std::string& passName);
    private:
        VkDevice* m_device = nullptr;
        VulkanContext* m_context = nullptr;
        VulkanSwapchain* m_swapChain = nullptr;
        VulkanResourceManager* m_resourceManager = nullptr;
        VkCommandPool m_commandPool;
        std::unordered_map<std::string, RenderPassInfo> m_renderPassInfos;
        //std::vector<RenderPassInfo> m_renderPassInfos;
};
