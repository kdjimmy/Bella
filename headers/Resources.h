#include <vulkan/vulkan.h>
#include <vector>
#include <mutex>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <set>
#include <unordered_set>>

namespace Resource
{
    enum RenderResourceType {
        Texture,            // 普通纹理
        ColorAttachment,    // 颜色附件
        DepthStencil,       // 深度/模板附件
        StorageTexture,     // 可读写纹理
        BlitTexture,        // 用于 Blit 的纹理
        GenericTexture,     // 普通采样纹理
        Buffer,             // 通用 Buffer
        StorageBuffer,      // 可读写 Buffer
        HistoryTexture,     // 历史帧（TAA/SSR）
        Proxy,              // 跨队列同步的代理资源
    };

    enum RenderGraphQueueType {
        FRAME_GRAPH_UNDEFINED_QUEUE = 0,
        FRAME_GRAPH_GRAPHIC_QUEUE = 1,
        FRAME_GRAPH_COMPUTE_QUEUE = 1 << 1,
        FRAME_GRAPH_ASYNC_GRAPHIC_QUEUE = 1 << 2,
        FRAME_GRAPH_ASYNC_COMPUTE_QUEUE = 1 << 3
    };

    enum AttachmentLifeCircle {
        ATTACHMENT_PERSISTENT_BIT = 1 << 0,
        ATTACHMENT_TRANSIENT_BIT = 1 << 1
    };

    enum AttachmentType {
        Color,
        Depth,
        Resolve,
        Storage
    };

    class RenderResource
    {
        public:
            RenderResource(RenderResourceType _type, unsigned _index, const char* _name)
            {
                type = _type;
                index = _index;
                name = _name;
            }
            RenderResource& operator=(const RenderResource& other) = delete;
            RenderResource(const RenderResource& other) = delete;
            inline RenderResourceType getType() const
            {
                return type;
            }
            inline unsigned int getIndex() const
            {
                return index;
            }
            inline std::string getName() const
            {
                return name;
            }
            inline void addQueue(RenderGraphQueueType other)
            {
                queue_Types = static_cast<RenderGraphQueueType>(queue_Types | other);
            }
            inline RenderGraphQueueType getQueue() const
            {
                return queue_Types;
            }
            inline void setPhysicalIndex(unsigned int index)
            {
                physical_index = index;
            }
            inline unsigned int getPhysicalIndex() const
            {
                return physical_index;
            }
            std::unordered_set<unsigned int> getWriteInPasses() const
            {
                return write_in_passes;
            }
            std::unordered_set<unsigned int> getReadInPasses() const
            {
                return read_in_passes;
            }
            inline void addWriteInPasses(unsigned int pass)
            {
                write_in_passes.emplace(pass);
            }
            inline void addReadInPasses(unsigned int pass)
            {
                read_in_passes.emplace(pass);
            }
        private:
            RenderResourceType type;
            unsigned int index, physical_index;
            std::string name;
            RenderGraphQueueType queue_Types = FRAME_GRAPH_UNDEFINED_QUEUE;
            std::unordered_set<unsigned int> write_in_passes;
            std::unordered_set<unsigned int> read_in_passes;
    };

    struct AttachmentInfo
    {
        VkFormat format = VK_FORMAT_UNDEFINED;          // 图像格式（RGBA16F、D32F 等）
        VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout final_layout   = VK_IMAGE_LAYOUT_UNDEFINED;
        VkAttachmentLoadOp  load_op  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE;
        VkClearColorValue clear_color = {};             // Color clear
        VkClearDepthStencilValue clear_depth_stencil = {1.0f, 0}; // Depth clear
        uint32_t samples = 1;                           // MSAA
        uint32_t levels  = 1;                           // mip levels
        uint32_t layers  = 1;                           // array layers
        AttachmentLifeCircle flags = ATTACHMENT_PERSISTENT_BIT;
        // flags: TRANSIENT / PERSISTENT / MIPGEN / HISTORY 等
        // 可选：用于区分 Color / Depth / Resolve / StorageTexture
        AttachmentType type = AttachmentType::Color;
    };

    class ResourceDescription
    {
        public:
            RenderGraphQueueType queueType;
            AttachmentLifeCircle flags;
            ResourceDescription()
            {
                flags = ATTACHMENT_PERSISTENT_BIT;
                queueType = FRAME_GRAPH_UNDEFINED_QUEUE;
            }
    };

    class TextureResourceDescription: public ResourceDescription
    {
        public:
            VkFormat format = VK_FORMAT_UNDEFINED;
            unsigned int width = 0;
            unsigned int height = 0;
            unsigned int depth = 1;
            unsigned int layers = 1;
            unsigned int levels = 1;
            unsigned int samples = 1;
            VkImageUsageFlags imageUsage = 0;
            VkImageLayout imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

            bool operator=(const TextureResourceDescription& other)const
            {
                return format == other.format &&
                width == other.width &&
                height == other.height &&
                depth == other.depth &&
                layers == other.layers &&
                levels == other.levels &&
                flags == other.flags &&
                imageLayout == other.imageLayout &&
                transform == other.transform;
            }
    };

    class BufferResourceDescription: public ResourceDescription
    {
        public:
            unsigned int size = 0;
            unsigned int stride = 0;
            VkBufferUsageFlags usage = 0;
            bool persistent = false; // 是否跨帧保留（history buffer） 
            bool transient = false; // 是否允许 transient（一般 false）
            VkPipelineStageFlags2 stages = 0;
            VkAccessFlags2 access = 0; 
            bool operator=(const BufferResourceDescription& other)const
            {
                return (size == other.size) && (stride == other.stride) && (usage == other.usage)
                    && (queueType == other.queueType) && (flags == other.flags) && (stages == other.stages) && (access == other.access);
            }
    };

    class RenderTextureResource: public RenderResource
    {
        private:
            AttachmentInfo attachmentInfo;
        public:
            TextureResourceDescription desc;
            inline void setAttachmentInfo(const AttachmentInfo& info)
            {
                attachmentInfo = info;
                desc.format = info.format;
                desc.samples = info.samples;
                desc.layers = info.layers;
                desc.flags = static_cast<AttachmentLifeCircle>(desc.flags | info.flags);
                switch(info.type)
                {
                    case AttachmentType::Color:
                        desc.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        desc.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                        break;
                    case AttachmentType::Depth:
                        desc.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        desc.imageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                        break;
                    case AttachmentType::Resolve:
                        desc.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                        desc.imageUsage |= (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
                        break;
                    case AttachmentType::Storage:
                        desc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                        desc.imageUsage |- VK_IMAGE_USAGE_STORAGE_BIT;
                        break;
                }
            }
    };

    class RenderBufferResource: public RenderResource
    {
        public:
            BufferResourceDescription bufferInfo;
            inline void addBufferInfo(const BufferResourceDescription& other)
            {
                bufferInfo = other;
                ///  should use or??????
            }
    };
}
