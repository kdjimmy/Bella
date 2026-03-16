#include "RenderGraph.h"

namespace Bella
{
    template<class T1, class T2>
    bool checkSize(const std::vector<T1*>& a, const std::vector<T2*>& b) {
        return a.size() == b.size();
    }

    #define ValidateSize(a,b,c) if(!checkSize(a,b)) { throw std::runtime_error(c); }

    template<class T>
    void validateResource(const std::vector<T*>& resources1, const std::vector<T*>& resources2) {
        if(std::is_same_v<T, RenderTextureResource>)
        {
            for(unsigned int i = 0; i < resources1.size(); i++)
            {
                auto res1 = resources1[i];
                auto res2 = resources2[i];
                if(res1->desc != res2->desc)
                {
                    throw std::runtime_error("RenderGraph Resource dimension mismatch.");
                }
            }
        }
        if(std::is_same_v<T, RenderBufferResource>)
        {
            for(unsigned int i = 0; i < resources1.size(); i++)
            {
                auto res1 = resources1[i];
                if(res1 == nullptr) continue;
                auto res2 = resources2[i];
                if(res1->desc != res2->desc)
                {
                    throw std::runtime_error("RenderGraph Resource dimension mismatch.");
                }
            }
        }
    }

    
#if 0
    void RenderGraph::validatePass()
    {
        for(auto& passPtr: renderPasses)
        {
            auto pass = passPtr.get();

            ValidateSize(pass->getColorOutputs(), pass->getResolveOutputs(),
                "RenderPass " + pass->getName() + " color output and resolve output size mismatch.");

            ValidateSize(pass->getColorInputs(), pass->getColorOutputs(),
                "RenderPass " + pass->getName() + " color input and output size mismatch.");
            
            ValidateSize(pass->getStorageInputs(), pass->getStorageTextureOutputs(),
                "RenderPass " + pass->getName() + " storage texture input and output size mismatch.");
            
            ValidateSize(pass->getBlitInputs(), pass->getBlitOutputs(),
                "RenderPass " + pass->getName() + " blit input and output size mismatch.");
            
            ValidateSize(pass->getStorageTextureInputs(), pass->getStorageTextureOutputs(),
                "RenderPass " + pass->getName() + " storage texture input and output size mismatch.");
            
            validateResource(pass->getColorInputs(), pass->getColorOutputs());
            validateResource(pass->getStorageInputs(), pass->getStorageOutputs());
            validateResource(pass->getStorageTextureInputs(), pass->getStorageTextureOutputs());
            validateResource(pass->getBlitInputs(), pass->getBlitOutputs());

            if(pass->getDepthStencilInput() && pass->getDepthStencilOutput())
            {
                if(!(pass->getDepthStencilInput()->desc == pass->getDepthStencilOutput()->desc))
                {
                    throw std::runtime_error("RenderPass " + pass->getName() + " depth-stencil input and output dimension mismatch.");
                }
            }
        }
    }
#endif

}