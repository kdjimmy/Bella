#pragma once
#include <vulkan/vulkan.h>
#include <cstring>
#include <future>
#include <mutex>
#include <memory>
#include "BaseImpl.h"
#include "VulkanImpl.h"
struct EngineDesc
{
    int renderApi;
};

class BaseEngine
{
    public:
        static BaseEngine& Get()
        {
            return *instance;
        }
        static void Create(const EngineDesc& desc)
        {
            if(desc.renderApi == 0)
            {
                Impl.reset(new VulkanImpl());
            }
        }
    private:
        static std::unique_ptr<BaseEngine> instance;
        static std::shared_ptr<BaseImpl> Impl;
};