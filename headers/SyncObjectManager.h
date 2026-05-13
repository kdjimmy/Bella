#pragma once
#include "VulkanCommon.h"
#include <vector>
#include <unordered_map>

struct passId
{
    uint32_t renderPassId;
    uint32_t subPassId;
    bool operator == (const passId& other) const
    {
        return renderPassId == other.renderPassId && subPassId == other.subPassId;
    }
};

struct passIdHash
{
    std::size_t operator()(const passId& passid) const
    {
        return ((static_cast<std::size_t>(passid.renderPassId)) << 32) ^ (static_cast<std::size_t>(passid.subPassId));
    }
};

enum SemaphoreKind
{
    Semaphore,
    TimelineSemaphore
};

struct VkSemaphoreInfo
{
    SemaphoreKind kind;
    VkSemaphore semaphore;
    bool used = false;
};

struct VkFenceInfo
{
    VkFence fence;
    bool used = false;
};

struct SemaphoreValue
{
    uint32_t semaphoreId;
    uint64_t semaphoreValue;
};

class SyncObjectManager
{
    public:
        SyncObjectManager(VkDevice device);
        ~SyncObjectManager();
        std::vector<VkSemaphoreInfo> semaphores;
        std::vector<VkFenceInfo> fences;
        //std::vector<VkSemaphore> timelinesemaphores;
        std::unordered_map<passId, SemaphoreValue, passIdHash> m_waitSemMap;
        std::unordered_map<passId, SemaphoreValue, passIdHash> m_signalSemMap;
        std::unordered_map<uint32_t, uint32_t> m_waitFenceMap;
        std::unordered_map<uint32_t, uint32_t> m_signalFenceMap;
        template<SemaphoreKind T>
        void registerSemaphore(passId &passid, uint32_t value, bool if_wait)
        {
            if constexpr (T == SemaphoreKind::Semaphore)
            {
                VkSemaphoreCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                VkSemaphore semaphore;
                VK_CHECK(vkCreateSemaphore(m_device, &createInfo, nullptr, &semaphore));
                semaphores.emplace_back({SemaphoreKind::Semaphore, semaphore, false});
                if(if_wait)
                {
                    m_waitSemMap[passid] = {semaphores.size() - 1, 0};
                }
                else
                {
                    m_signalSemMap[passid] = {semaphores.size() - 1, 0};
                }
            }

            else if constexpr (T == SemaphoreKind::TimelineSemaphore)
            {
                VkSemaphoreTypeCreateInfo timelineCreateInfo{};
                timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
                timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
                timelineCreateInfo.initialValue = 0;
                VkSemaphoreCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                createInfo.pNext = &timelineCreateInfo;
                VkSemaphore timelineSemaphore;
                VK_CHECK(vkCreateSemaphore(m_device, &createInfo, nullptr, &timelineSemaphore));
                semaphores.emplace_back({SemaphoreKind::TimelineSemaphore, timelineSemaphore, false});
                if(if_wait)
                {
                    m_waitSemMap[passid] = {semaphores.size() - 1, value};
                }
                else
                {
                    m_signalSemMap[passid] = {semaphores.size() - 1, value};
                }
            }
        };

        void registerFence(uint32_t passId, bool if_wait, VkFenceCreateFlags flag);
        void markSemaphoreUsed(uint32_t semaphoreId);
        void markFenceUsed(uint32_t fenceId);

    private:
        VkDevice m_device;

};
