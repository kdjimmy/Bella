#include "TaskDependency.h"
#include "ThreadGroup.h"
namespace Task
{
    TaskDependency::TaskDependency(ThreadGroup* group, uint32_t _id, TaskKind type): threadGroup(group), id(_id), taskClass(type)
    {
        completed_Task.store(0, std::memory_order_relaxed);
        pending_task.store(0, std::memory_order_relaxed);
        dependencyCount.store(0, std::memory_order_relaxed);
        
    }

    //get the remaining task, it reaches 1, notify the dependee to run
    void TaskDependency::task_Completed()
    {
        auto tmp = pending_task.fetch_sub(1, std::memory_order_acq_rel);
        if (tmp == 1)            // all task has finished, call dependee to run
        {
            notify_dependees();
        }
    }

    //signal to increase, notify dependees to run, remark finished
    void TaskDependency::notify_dependees()
    {
        if (signal)
        {
            signal->increase();
        }
        ////std::cout << "notify dependees " << pending.size() << "dep id = " << id << std::endl;
        for (auto& taskDep : pending)
        {
            ////std::cout << "pre id = " << id << " notify dependee id = " << taskDep.get()->id << std::endl;
            taskDep.get()->dependency_satisfied();
        }
        //std::cout << "notify dependees done dep id = " << id << std::endl;
        //pending.clear();
        {
            std::unique_lock<std::mutex> lk(mtx);
            //std::cout << "notify dependees done dep2 id = " << id << std::endl;
            finished = true;
        }
        //std::cout << "notify dependees done dep id3 = " << id << std::endl;
        cv.notify_all();
    }

    // if has remaining task, add to ready queue, or notify the later dependee to run
    bool TaskDependency::dependency_satisfied()
    {
        auto tmp = dependencyCount.fetch_sub(1, std::memory_order_acq_rel);
        ////std::cout << "dependency count = " << tmp - 1 << "dep id" << id <<std::endl;
        if (tmp <= 1)
        {
            if (tasks.size() > 0)
            {
                ////std::cout << "add-task = " << id << "size = " << tasks.size() << std::endl;
                threadGroup->addToReadyQueue(std::move(tasks));
                ////std::cout << "add task = " << id << "size = " << tasks.size() <<std::endl;
                tasks.clear();
            }
            else notify_dependees();
            return true;
        }
        return false;
    }
}