#pragma once
#include <iostream>
#include <cstdio>
#include <thread>
#include <mutex>
#include <future>
#include <memory>
#include <queue>
#include <vector>

namespace Task
{
    class Task;
    class ThreadGroup;
    struct TaskSignal
    {
        std::mutex mtx;
        std::condition_variable cv;
        uint32_t counter = 0;
        //a task finishes and increase the signal
        void increase()
        {
            std::unique_lock<std::mutex> lk(mtx);
            counter ++;
            cv.notify_all(); //???
        }

        //main thread wait for the finished task >= other
        void waitForValue(int other)
        {
            std::unique_lock<std::mutex> lk(mtx);
            cv.wait(lk, [&](){return counter >= other;});
        }

        uint32_t get_count()
        {
            std::unique_lock<std::mutex> lk(mtx);
            return counter;
        }
    };

    enum TaskKind
    {
        ForeGround = 0,
        BackGround = 1
    };

    class TaskDependency
    {
        public:
            explicit TaskDependency(ThreadGroup* group);
            void task_Completed();
            bool dependency_satisfied();
            void notify_dependees();
            std::condition_variable cv;
            std::mutex mtx;
            bool finished = false;
            std::atomic_uint pending_task;
            std::string desc;
            TaskKind taskClass = TaskKind::ForeGround;
            TaskSignal* signal = nullptr;
            ThreadGroup* threadGroup = nullptr;
            std::vector<std::shared_ptr<TaskDependency>> pending;           //依赖于我的其他依赖
            std::atomic_uint completed_Task;
            
            std::vector<Task*> tasks;
            
            std::atomic_uint dependencyCount;       //我依赖于其他的依赖数量

            
            
            
    }; 
}