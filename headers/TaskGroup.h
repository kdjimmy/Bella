#pragma once
#include "headers/Task.h"
#include "headers/TaskDependency.h"

namespace Task
{
    class ThreadGroup;
    class TaskGroup
    {
        private:
            ThreadGroup* group;
            std::unique_ptr<TaskDependency> depend;
            unsigned int id = 0;
            bool flushed = false;
        public:
            explicit TaskGroup(ThreadGroup* _group, int _id)
            {
                group = _group;
                id = _id;
            }

            template<class T>
            void enqueueTask(T&& tasks)
            {
                depend.get()->tasks.push_back(std::forward<T>(task));
                depend.get()->pending_task.fetch_add(1, std::memory_order_acq_rel);
            }

            //push tasks into ready queue
            inline void flush()
            {
                if(flushed)
                {
                    std::cout << "repeat flush" << std::endl;
                    return;
                }

                if(depend.get()->dependency_satisfied())
                {
                    flushed = true;
                }
            }

            inline void wait()
            {
                if(!flushed) flush();
                std::unique_lock<std::mutex> lk(depend.get()->mtx);
                depend.get()->cv.wait(lk, [this](){return depend.get()->finished;});
            }

            inline bool poll()
            {
                if(!flushed) flush();
                auto tmp = depend.get()->pending_task.load(std::memory_order_acquire);
                return tmp == 0;
            }

            ThreadGroup* getThreadGroup(){return group;}
            void setDesc(std::string& desc)
            {
                depend.get()->desc = desc;
            }

            void set_Task_Class(TaskKind type)
            {
                depend.get()->taskClass = type;
            }

            void set_Task_Signal(TaskSignal* _signal)
            {
                depend.get()->signal = _signal;
            }
    };
}

