#include "ThreadGroup.h"

namespace Task
{
    ThreadGroup::ThreadGroup()
    {
        //active.store(false, std::memory_order_relaxed);
        //finished.store(false, std::memory_order_relaxed);
        total_task_count.store(0, std::memory_order_relaxed);
        completed_task_count.store(0, std::memory_order_relaxed);
    }

    void ThreadGroup::stop()
    {
        if(!active)
        {
            std::cout << "thread group is inactive" << std::endl;
            return;
        }
        waitIdle();
        // call all threads to stop
        {
            std::unique_lock<std::mutex> lk(foreGround.mtx);
            foreGround.cv.notify_all();
        }
        {
            std::unique_lock<std::mutex> lk(backGround.mtx);
            backGround.cv.notify_all();
        }

        for(uint32_t i = 0; i < foreGround.threads.size(); i++)
        {
            if(foreGround.threads[i]->thread.joinable())
            {
                foreGround.threads[i]->thread.join();
                foreGround.threads[i].reset();
            }
        }
        for(uint32_t i = 0; i < backGround.threads.size(); i++)
        {
            if(backGround.threads[i]->thread.joinable())
            {
                backGround.threads[i]->thread.join();
                backGround.threads[i].reset();
            }
        }
        finished = true;
        active = false;
    }

    void ThreadGroup::waitIdle()
    {
        std::unique_lock<std::mutex> lk(wait_mtx);
        wait_cv.wait(lk, [this](){return total_task_count.load(std::memory_order_acquire) == completed_task_count.load(std::memory_order_acquire);});
        std::cout << "all tasks have finished" << std::endl;
    }

    void ThreadGroup::start(uint32_t foreGroundThreads, uint32_t backGroundThreads)
    {
        dead = false;
        active = true;
        foreGround.threads.resize(foreGroundThreads);
        backGround.threads.resize(backGroundThreads);
        auto Func = [this](uint32_t idx, TaskKind taskKind) -> void{
            beginFuntion();
            run_loop(idx, taskKind);
        };
        for(uint32_t i = 0; i < foreGroundThreads; i++)
        {
            auto& t = foreGround.threads[(int)i];
            t = std::make_unique<Thread>(std::thread(Func, i + 1, TaskKind::ForeGround), i + 1);
        }

        for(uint32_t i = 0; i < backGroundThreads; i++)
        {
            auto& t = backGround.threads[(int)i];
            t = std::make_unique<Thread>(std::thread(Func, i + 1 + foreGroundThreads, TaskKind::BackGround), i + 1 + foreGroundThreads);
        }
    }

    ThreadGroup::~ThreadGroup()
    {
        if(active && !finished)
        {
            stop();
        }
    }

    void ThreadGroup::submit(TaskGroup &group)
    {
        group.flush();
    }

    void ThreadGroup::addDependency(TaskGroup& dependee, TaskGroup& dependency)
    {
        dependency.depend.get()->pending.push_back(std::shared_ptr<TaskDependency>(dependee.depend.get()));
        dependee.depend.get()->dependencyCount.fetch_add(1, std::memory_order_acq_rel);
    }

    template<class T>
    void ThreadGroup::enqueueTask(T&& tasks, TaskGroup& group)
    {
        total_task_count.fetch_add(1, std::memory_order::memory_order_acq_rel);
        group.enqueueTask(std::forward<T>(tasks));
    }

    template<class T>
    TaskGroup* ThreadGroup::createTaskGroup(T&& tasks)
    {
        TaskGroup* taskGroup = new TaskGroup(this, 0);
        enqueueTask(std::forward<T>(tasks), *taskGroup);
        return taskGroup;
    }

    void ThreadGroup::addToReadyQueue(std::vector<std::shared_ptr<Task>>&& tasks)
    {
        unsigned int fgTaskCount = 0;
        unsigned int bgTaskCount = 0;
        for (auto& it : tasks)
        {
            if (it.get()->taskDependency.get()->taskClass == TaskKind::ForeGround)
            {
                foreGround.taskQueue.emplace(std::move(it));
                fgTaskCount++;
            }
            if (it.get()->taskDependency.get()->taskClass == TaskKind::BackGround)
            {
                backGround.taskQueue.emplace(std::move(it));
                bgTaskCount++;
            }
        }

        total_task_count.fetch_add(fgTaskCount + bgTaskCount, std::memory_order::memory_order_acq_rel);

        if (fgTaskCount)
        {
            std::unique_lock<std::mutex> lk(foreGround.mtx);
            if (fgTaskCount >= foreGround.threads.size())
            {
                foreGround.cv.notify_all();
            }
            else
            {
                for (int i = 0; i < fgTaskCount; i++)
                {
                    foreGround.cv.notify_one();
                }
            }
        }
        if (bgTaskCount)
        {
            std::unique_lock<std::mutex> lk(backGround.mtx);
            if (bgTaskCount >= backGround.threads.size())
            {
                backGround.cv.notify_all();
            }
            else
            {
                for (int i = 0; i < bgTaskCount; i++)
                {
                    backGround.cv.notify_one();
                }
            }
        }
    }

    void ThreadGroup::run_loop(uint32_t thread_id, TaskKind kind)
    {
            threadSet* threadSet = (kind == TaskKind::ForeGround) ? &foreGround : &backGround;
            while (1)
            {
                std::unique_lock<std::mutex> lk(threadSet->mtx);
                threadSet->cv.wait(lk, [&, this]()
                    {
                        return dead && !threadSet->taskQueue.empty();
                    });

                if (dead && threadSet->taskQueue.empty())
                {
                    break;
                }
                std::shared_ptr<Task> task(threadSet->taskQueue.front());
                threadSet->taskQueue.pop();
                if (task.get()->callable.isActive())
                {
                    std::cout << "thread id " << std::this_thread::get_id() << " is running a task" << std::endl;
                    task.get()->callable.call();
                }
                task.get()->taskDependency.get()->task_Completed();
                auto tmp = completed_task_count.fetch_add(1, std::memory_order_acq_rel);
                if (tmp + 1 == total_task_count.load(std::memory_order_acquire))
                {
                    std::unique_lock<std::mutex> wait_lk(wait_mtx);
                    wait_cv.notify_all();
                }
            }
    }
}