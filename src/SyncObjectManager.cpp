#include "SyncObjectManager.h"
#include <set>
void SyncObjectManager::registerFence(uint32_t passId, bool if_wait, VkFenceCreateFlags flag)
{
    VkFence fence;
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.flags = flag;
    VK_CHECK(vkCreateFence(m_device, &createInfo, nullptr, &fence));
    fences.push_back({fence, false});
    if(if_wait)
    {
        m_waitFenceMap[passId] = fences.size() - 1;
    }
    else
    {
        m_signalFenceMap[passId] = fences.size() - 1;
    }
}

void SyncObjectManager::markSemaphoreUsed(uint32_t semaphoreId)
{
    if(semaphoreId < semaphores.size())
    {
        semaphores[semaphoreId].used = true;
    }
}

void SyncObjectManager::markFenceUsed(uint32_t fenceId)
{
    if(fenceId < fences.size())
    {
        fences[fenceId].used = true;
    }
}

SyncObjectManager::SyncObjectManager(VkDevice device)
{
    m_device = device;
    semaphores.clear();
    fences.clear();
    m_waitSemMap.clear();
    m_signalSemMap.clear();
    m_waitFenceMap.clear();
    m_signalFenceMap.clear();
}

SyncObjectManager::~SyncObjectManager()
{
    std::set<VkFence> waitFences;
    std::vector<SemaphoreValue> waitTimelineSemaphore;
    for(auto& iter:m_signalSemMap)
    {
        passId pass = iter.first;
        SemaphoreValue semaphoreValue = iter.second;
        uint32_t semaphoreId = semaphoreValue.semaphoreId;
        auto sem = semaphores[semaphoreId];
        //probably no need to consider binary semaphore
        if(sem.kind == SemaphoreKind::TimelineSemaphore)
        {
            waitTimelineSemaphore.emplace_back(semaphoreValue);
        }
        else
        {
            if(m_waitFenceMap.find(pass.renderPassId) != m_waitFenceMap.end())
            {
                waitFences.insert(fences[m_waitFenceMap[pass.renderPassId]].fence);
            }
        }
    }
    for(auto& iter:m_waitSemMap)
    {
        passId pass = iter.first;
        SemaphoreValue semaphoreValue = iter.second;
        uint32_t semaphoreId = semaphoreValue.semaphoreId;
        auto sem = semaphores[semaphoreId];
        if(sem.kind == SemaphoreKind::TimelineSemaphore)
        {
            waitTimelineSemaphore.emplace_back(semaphoreValue);
        }
        else
        {
            if(m_waitFenceMap.find(pass.renderPassId) != m_waitFenceMap.end())
            {
                waitFences.insert(fences[m_waitFenceMap[pass.renderPassId]].fence);
            }
        }
    }
    for(auto &waitTimelineSem:waitTimelineSemaphore)
    {
        if(waitTimelineSem.semaphoreId >= semaphores.size() || !semaphores[waitTimelineSem.semaphoreId].used)
        {
            continue;
        }
        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &semaphores[waitTimelineSem.semaphoreId].semaphore;
        waitInfo.pValues = &waitTimelineSem.semaphoreValue;
        VK_CHECK(vkWaitSemaphores(m_device, &waitInfo, UINT64_MAX));
        //vkDestroySemaphore(m_device, semaphores[waitTimelineSem.semaphoreId].semaphore, nullptr);
    }
    for(auto& fenceInfo: fences)
    {
        if(fenceInfo.used)
        {
            VK_CHECK(vkWaitForFences(m_device, 1, &fenceInfo.fence, VK_TRUE, UINT64_MAX));
        }
        vkDestroyFence(m_device, fenceInfo.fence, nullptr);
    }
    for(auto& sem: semaphores)
    {
        vkDestroySemaphore(m_device, sem.semaphore, nullptr);
    }

    semaphores.clear();
    fences.clear();
    m_waitSemMap.clear();
    m_signalSemMap.clear();
    m_waitFenceMap.clear();
    m_signalFenceMap.clear();
}
