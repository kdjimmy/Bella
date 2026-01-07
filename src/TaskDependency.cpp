#include "headers/TaskDependency.h"

using namespace Task;

explicit TaskDependency::TaskDependency(ThreadGroup* group):threadGroup(group)
{
    completed_Task.store(0, std::memory_order_relaxed);
    pending_task.store(0, std::memory_order_relaxed);
    dependencyCount.store(1, std::memory_order_relaxed);
}

void TaskDependency::task_Completed()
{
    auto tmp = pending_task.fetch_sub(1, std::memory_order_acq_rel);        
    if(tmp == 1)            // all task has finished, call dependee to run
    {
        notify_dependees();
    }
}

void TaskDependency::notify_dependees()
{
    if(signal)
    {
        signal->increase();
    }
    for(auto& taskDep:pending)
    {
        taskDep.get()->dependency_satisfied();
    }
    pending.clear();
    std::unique_lock<std::mutex> lk(mtx);
    finished = true;
    cv.notify_all();
}

bool TaskDependency::dependency_satisfied()
{
    auto tmp = dependencyCount.fetch_sub(1, std::memory_order_acq_rel);
    if(tmp == 1)
    {
        if(tasks.size() > 0)
        {
            threadGroup->addToReadyQueue(std::move(tasks));
            tasks.clear();
        }
        else notify_dependees();
        return true;
    }
    return false;
}