#pragma once
#include "Resources.h"
#include <string>
#include <vector>
#include <functional>

namespace Bella
{
class RenderGraph; // 前置声明

class RenderPass
{
public:
    static constexpr unsigned Unused = ~0u;

    struct AccessedResource
    {
        VkPipelineStageFlags2 stage = 0;
        VkAccessFlags2 access = 0;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct AccessedTextureResource : AccessedResource
    {
        RenderTextureResource* texture = nullptr;
    };

    struct AccessedBufferResource : AccessedResource
    {
        RenderBufferResource* buffer = nullptr;
    };

    RenderPass(RenderGraph& graph_, unsigned index_, RenderGraphQueueType queue_)
        : graph(graph_), index(index_), queue(queue_) {}

    // 基本属性
    inline unsigned getIndex() const { return index; }
    inline RenderGraphQueueType getQueue() const { return queue; }
    inline void setName(const std::string& n) { name = n; }
    inline const std::string& getName() const { return name; }

    inline void setPhysicalPassIndex(unsigned p) { physical_pass = p; }
    inline unsigned getPhysicalPassIndex() const { return physical_pass; }

    // 输入 / 输出声明（纹理）
    RenderTextureResource* addColorOutput(RenderTextureResource* tex, const AttachmentInfo& info)
    {
        tex->setAttachmentInfo(info);
        color_outputs.push_back(tex);
        tex->addWriteInPasses(index);
        return tex;
    }

    RenderTextureResource* addResolveOutput(RenderTextureResource* tex, const AttachmentInfo& info)
    {
        tex->setAttachmentInfo(info);
        resolve_outputs.push_back(tex);
        tex->addWriteInPasses(index);
        return tex;
    }

    RenderTextureResource* setDepthStencilOutput(RenderTextureResource* tex, const AttachmentInfo& info)
    {
        tex->setAttachmentInfo(info);
        depth_stencil_output = tex;
        tex->addWriteInPasses(index);
        return tex;
    }

    RenderTextureResource* setDepthStencilInput(RenderTextureResource* tex)
    {
        depth_stencil_input = tex;
        tex->addReadInPasses(index);
        return tex;
    }

    RenderTextureResource* addTextureInput(RenderTextureResource* tex,
                                           VkPipelineStageFlags2 stage,
                                           VkAccessFlags2 access,
                                           VkImageLayout layout)
    {
        AccessedTextureResource ar{};
        ar.texture = tex;
        ar.stage = stage;
        ar.access = access;
        ar.layout = layout;
        generic_texture_inputs.push_back(ar);
        tex->addReadInPasses(index);
        return tex;
    }

    RenderTextureResource* addStorageTextureOutput(RenderTextureResource* tex,
                                                   const AttachmentInfo& info,
                                                   VkPipelineStageFlags2 stage,
                                                   VkAccessFlags2 access,
                                                   VkImageLayout layout)
    {
        tex->setAttachmentInfo(info);
        AccessedTextureResource ar{};
        ar.texture = tex;
        ar.stage = stage;
        ar.access = access;
        ar.layout = layout;
        storage_texture_outputs.push_back(ar);
        tex->addWriteInPasses(index);
        return tex;
    }

    // 输入 / 输出声明（buffer）
    RenderBufferResource* addUniformBufferInput(RenderBufferResource* buf,
                                                VkPipelineStageFlags2 stage,
                                                VkAccessFlags2 access)
    {
        AccessedBufferResource br{};
        br.buffer = buf;
        br.stage = stage;
        br.access = access;
        generic_buffer_inputs.push_back(br);
        buf->addReadInPasses(index);
        return buf;
    }

    RenderBufferResource* addStorageBufferOutput(RenderBufferResource* buf,
                                                 const BufferResourceDescription& info,
                                                 VkPipelineStageFlags2 stage,
                                                 VkAccessFlags2 access)
    {
        buf->setBufferInfo(info);
        AccessedBufferResource br{};
        br.buffer = buf;
        br.stage = stage;
        br.access = access;
        storage_buffer_outputs.push_back(br);
        buf->addWriteInPasses(index);
        return buf;
    }

    // 清除值查询（可选）
    bool getClearColor(unsigned attachmentIndex, VkClearColorValue* value) const
    {
        if (attachmentIndex < color_outputs.size() && value)
        {
            *value = color_outputs[attachmentIndex]->getAttachmentInfo().clear_color;
            return true;
        }
        return false;
    }

    bool getClearDepthStencil(VkClearDepthStencilValue* value) const
    {
        if (depth_stencil_output && value)
        {
            *value = depth_stencil_output->getAttachmentInfo().clear_depth_stencil;
            return true;
        }
        return false;
    }

    // 构建回调（你可以在 RenderGraph 执行阶段调用）
    using BuildCallback = std::function<void(VkCommandBuffer& cmd)>;
    void setBuildCallback(BuildCallback cb) { build_cb = std::move(cb); }

    // 访问集合（给 RenderGraph 生成 barrier 用）
    const std::vector<RenderTextureResource*>& getColorOutputs() const { return color_outputs; }
    const std::vector<RenderTextureResource*>& getResolveOutputs() const { return resolve_outputs; }
    const std::vector<AccessedTextureResource>& getGenericTextureInputs() const { return generic_texture_inputs; }
    const std::vector<AccessedTextureResource>& getStorageTextureOutputs() const { return storage_texture_outputs; }
    const std::vector<AccessedBufferResource>& getGenericBufferInputs() const { return generic_buffer_inputs; }
    const std::vector<AccessedBufferResource>& getStorageBufferOutputs() const { return storage_buffer_outputs; }
    RenderTextureResource* getDepthStencilInput() const { return depth_stencil_input; }
    RenderTextureResource* getDepthStencilOutput() const { return depth_stencil_output; }

    std::function<void(VkClearColorValue&)> setClearColorValue;
    std::function<void(VkClearDepthStencilValue&)> setClearDepthStencilValue;

private:
    RenderGraph& graph;
    unsigned index = 0;
    unsigned physical_pass = Unused;
    RenderGraphQueueType queue = FRAME_GRAPH_UNDEFINED_QUEUE;
    std::string name;

    // 纹理
    std::vector<RenderTextureResource*> color_outputs;
    std::vector<RenderTextureResource*> resolve_outputs;
    std::vector<AccessedTextureResource> generic_texture_inputs;
    std::vector<AccessedTextureResource> storage_texture_outputs;
    RenderTextureResource* depth_stencil_input = nullptr;
    RenderTextureResource* depth_stencil_output = nullptr;

    // buffer
    std::vector<AccessedBufferResource> generic_buffer_inputs;
    std::vector<AccessedBufferResource> storage_buffer_outputs;

    BuildCallback build_cb;
};
} // namespace Bella
