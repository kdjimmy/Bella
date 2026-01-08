#include "RenderPass.h"
#include <memory>
namespace FrameGraph
{
    class FrameGraph
    {
    public:
        VkDevice device = VK_NULL_HANDLE;
        FrameGraph(const VkDevice& _device) :device(_device)
        {
            printf("createing frame graph device = 0x%p", &device);
        }

        inline RenderPass& addPass(const RenderPass& pass)
        {

        }
    private:
        std::vector<std::shared_ptr<RenderPass>> passes;
        std::vector<std::shared_ptr<Resource::RenderResource>> resources;
        std::unordered_map<std::string, unsigned> pass_to_index;
        std::unordered_map<std::string, unsigned> resource_to_index;
        std::vector<std::unordered_set<unsigned>> pass_dependencies;
        std::vector<std::unordered_set<unsigned>> pass_merge_dependencies;
        std::string backbuffer_source;
    };
};
