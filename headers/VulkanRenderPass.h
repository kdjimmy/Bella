#pragma once

#include "VulkanCommon.h"
#include <vector>

class VulkanRenderPass
{
    public:
        void createRenderPass();
    private:
        std::vector<VkAttachmentDescription> m_attachments;
        std::vector<VkAttachmentReference> m_attachmentRefs;
        std::vector<VkSubpassDescription> m_subpasses;
        std::vector<VkSubpassDependency> m_subpassDeps;
        VkDevice m_device;
        VkRenderPass m_renderPass;
}