#include "TaskGroup.h"
#include "Task.h"

namespace Task
{
    struct Thread
    {
        std::thread thread;
        uint32_t id;
        Thread(std::thread&& _thread, uint32_t _id)
        {
            thread = std::move(_thread);
            id = _id;
        }
    };
    struct threadSet
    {
        std::mutex mtx;
        std::condition_variable cv;
        std::vector<std::unique_ptr<Thread>> threads;
        std::queue<std::shared_ptr<Task>> taskQueue;
    };

    inline std::function<void()> beginFuntion = []() -> void {
        std::cout << "begin thread tid = " << std::this_thread::get_id() << std::endl;
    };

    class ThreadGroup
    {
    private:
        std::mutex wait_mtx;    //thread group wait for all task to finish
        std::condition_variable wait_cv;
        bool active = false;
        bool finished = false;
        bool dead = false;
        std::atomic_uint total_task_count;
        std::atomic_uint completed_task_count;
        threadSet foreGround, backGround;
        void run_loop(uint32_t thread_id, TaskKind kind);
    public:
        explicit ThreadGroup();
        ~ThreadGroup();
        ThreadGroup(ThreadGroup&& other) = delete;
        ThreadGroup& operator=(ThreadGroup&& other) = delete;
        void start(uint32_t foreGroundThreads, uint32_t backGroundThreads);
        void stop();

        template<class T>
        inline void enqueueTask(T&& tasks, TaskGroup& group)
        {
            //total_task_count.fetch_add(1, std::memory_order::memory_order_acq_rel);
            group.enqueueTask(std::forward<T>(tasks));
        }

        template<class T>
        inline TaskGroup* createTaskGroup(T&& tasks)
        {
            TaskGroup* taskGroup = new TaskGroup(this, 0);
            enqueueTask(std::forward<T>(tasks), *taskGroup);
            return taskGroup;
        }
        void addToReadyQueue(std::vector<std::shared_ptr<Task>>&& tasks);
        void addDependency(TaskGroup& dependee, TaskGroup& dependency);
        //wait idle calls the main thread to wait until all tasks have finished
        void waitIdle();
        void submit(TaskGroup& group);
    };
}