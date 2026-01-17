#include "RenderPass.h"
#include <memory>
namespace Bella
{
    struct Barrier
    {
        unsigned index;
        VkImageLayout imageLayout;
        VkPipelineStageFlags2 stage;
        VkAccessFlags2 access;
        bool history;
    };

    struct Barriers
    {
        std::vector<Barrier> invalidate;
		std::vector<Barrier> flush;
    };

    std::vector<Barriers> pass_barriers;            //total added barrier

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
    }
}