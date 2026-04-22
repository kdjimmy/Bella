#pragma once

#include "RenderPass.h"
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <unordered_set>

using namespace Bella;

namespace FrameGraph
{
    class FrameGraph
    {
    public:
        VkDevice device = VK_NULL_HANDLE;

        explicit FrameGraph(const VkDevice& _device) : device(_device)
        {
            std::printf("createing frame graph device = 0x%p", static_cast<const void*>(&device));
        }

        inline RenderPass& addPass(RenderPass& pass)
        {
            return pass;
        }

    private:
        std::vector<std::shared_ptr<RenderPass>> passes;
        std::vector<std::shared_ptr<RenderResource>> resources;
        std::unordered_map<std::string, unsigned> pass_to_index;
        std::unordered_map<std::string, unsigned> resource_to_index;
        std::vector<std::unordered_set<unsigned>> pass_dependencies;
        std::vector<std::unordered_set<unsigned>> pass_merge_dependencies;
        std::string backbuffer_source;
    };
}
