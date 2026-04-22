#pragma once

#include <cstring>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

class BaseImpl
{
    public:
        BaseImpl() = default;
        virtual ~BaseImpl() = default;

        virtual void initialize() = 0;
        virtual void cleanUp() = 0;
        virtual void run() = 0;
        virtual void render() = 0;
};
