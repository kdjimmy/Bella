#pragma once
#include "Task.h"
#include "TaskDependency.h"

namespace Task
{
    class ThreadGroup;
    class TaskGroup
    {
        private:
            ThreadGroup* group;
            
            unsigned int id = 0;
            bool flushed = false;
        public:
            std::shared_ptr<TaskDependency> depend;
            explicit TaskGroup(ThreadGroup* _group, int _id, TaskKind type = TaskKind::ForeGround)
            {
                depend = std::make_shared<TaskDependency>(_group, _id, type);
                group = _group;
                id = _id;
                //depend.get()->pending_task.store(0, std::memory_order::memory_order_acquire);
            }

            //push task into this task group
            template<class T>
            void enqueueTask(T&& tasks)
            {
                depend.get()->tasks.push_back(std::make_shared<Task>(depend, std::forward<T>(tasks)));
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

            //main thread wait for this task group to finish
            inline void wait()
            {
                if(!flushed) flush();
                std::unique_lock<std::mutex> lk(depend.get()->mtx);
                depend.get()->cv.wait(lk, [this](){return depend.get()->finished;});
            }

            //query whether all tasks in this group have finished
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

