#pragma once

#include "RenderPass.h"
#include <memory>
#include <unordered_map>

namespace Bella
{
    struct Barrier
    {
        unsigned index = 0;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkPipelineStageFlags2 stage = 0;
        VkAccessFlags2 access = 0;
        bool history = false;
    };

    struct Barriers
    {
        std::vector<Barrier> invalidate;
        std::vector<Barrier> flush;
    };

    inline std::vector<Barriers> pass_barriers;

    class RenderGraph
    {
    public:
        RenderGraph() = default;

    private:
        VkDevice* device = nullptr;
        std::vector<std::unique_ptr<RenderPass>> renderPasses;
        std::vector<std::unique_ptr<RenderResource>> renderResources;
        std::unordered_map<std::string, unsigned> passToIndex;
        std::unordered_map<std::string, unsigned> resourceToIndex;
        std::string backBufferSource;
        std::vector<unsigned> passStack;
    };
}
