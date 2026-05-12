#pragma once

#include <cstring>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#include <GLFW/glfw3.h>
class BaseImpl
{
    public:
        BaseImpl() = default;
        virtual ~BaseImpl() = default;

        virtual void initialize(GLFWwindow* window) = 0;
        virtual void cleanUp() = 0;
        virtual void run() = 0;
        virtual void render() = 0;
};
