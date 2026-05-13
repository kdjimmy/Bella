#include "VulkanRender.h"

void VulkanRender::createCommandPool(uint32_t familyIndex, VkCommandPoolCreateFlags flag)
{
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = familyIndex;
    createInfo.flags = flag;
    VK_CHECK(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_pool));
}

std::vector<VkCommandBuffer> VulkanRender::createCommandBuffer(VkCommandBufferLevel level, uint32_t commandBufferCount)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_pool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = commandBufferCount;
    allocInfo.pNext = nullptr;
    std::vector<VkCommandBuffer> commandBuffers(commandBufferCount);
    VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, commandBuffers.data()));
    return commandBuffers;
}

void VulkanRender::recordCommandBuffers(VkCommandBuffer commandBuffer, VkRenderPass renderPass, 
    VkExtent2D swapchainExtent, VkFramebuffer framebuffer)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    //APPEND THE CONTENT OF RENDER PASS
    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void VulkanRender::submitQueue(const std::vector<VkCommandBuffer>& commandBuffers, VkQueue& queue)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = commandBuffers.size();
    submitInfo.pCommandBuffers = commandBuffers.data();
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}