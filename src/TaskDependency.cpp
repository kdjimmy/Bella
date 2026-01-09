#include "TaskDependency.h"
#include "ThreadGroup.h"
namespace Task
{
    TaskDependency::TaskDependency(ThreadGroup* group, TaskKind type) :threadGroup(group), taskClass(type)
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
        for (auto& taskDep : pending)
        {
            taskDep.get()->dependency_satisfied();
        }
        //pending.clear();
        std::unique_lock<std::mutex> lk(mtx);
        finished = true;
        cv.notify_all();
    }

    // if has remaining task, add to ready queue, or notify the later dependee to run
    bool TaskDependency::dependency_satisfied()
    {
        auto tmp = dependencyCount.fetch_sub(1, std::memory_order_acq_rel);
        if (tmp <= 1)
        {
            if (tasks.size() > 0)
            {
                threadGroup->addToReadyQueue(std::move(tasks));
                tasks.clear();
            }
            else notify_dependees();
            return true;
        }
        return false;
    }
}