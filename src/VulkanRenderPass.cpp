#include "VulkanRenderPass.h"

void VulkanRenderPass::createRenderPass()
{
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = m_attachments.size();
    renderPassInfo.pAttachments = m_attachments.data();
    renderPassInfo.subpassCount = m_subpasses.size();
    renderPassInfo.pSubpasses = m_subpasses.data();
    renderPassInfo.dependencyCount = m_subpassDeps.size();
    renderPassInfo.pDependencies = m_subpassDeps.data();
    VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass));
}