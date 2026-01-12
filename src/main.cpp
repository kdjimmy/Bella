#include "FrameGraph.h"
//#include "Task.h"
#include "ThreadGroup.h"


int main()
{
    std::unique_ptr<Task::ThreadGroup> threadGroup = std::make_unique<Task::ThreadGroup>();
    threadGroup->start(20, 20);
    Task::TaskGroup group1(threadGroup.get(), 0, Task::TaskKind::ForeGround);
    Task::TaskGroup group2(threadGroup.get(), 1, Task::TaskKind::BackGround);
    Task::TaskGroup group3(threadGroup.get(), 2, Task::TaskKind::BackGround);
    Task::TaskGroup group4(threadGroup.get(), 3, Task::TaskKind::ForeGround);
    Task::TaskGroup group5(threadGroup.get(), 4, Task::TaskKind::ForeGround);
    threadGroup->addDependency(group2, group1);
    threadGroup->addDependency(group3, group1);
    threadGroup->addDependency(group4, group2);
    threadGroup->addDependency(group4, group3);
    threadGroup->addDependency(group5, group1);
    auto taskFunc = [](int a)->void
    {
        std::cout << "task func a = " << a << std::endl;
    };
    for(int i = 0; i < 5; i ++)
    {
        auto realFunc = std::bind(taskFunc, i);
        if(i < 1)
        {
            threadGroup->enqueueTask(std::move(realFunc), group1);
        }
        else if(i < 2)
        {
            threadGroup->enqueueTask(std::move(realFunc), group2);
        }
        else if(i < 3)
        {
            threadGroup->enqueueTask(std::move(realFunc), group3);
        }
        else if(i < 4)
        {
            threadGroup->enqueueTask(std::move(realFunc), group4);
        }
        else
        {
            threadGroup->enqueueTask(std::move(realFunc), group5);
        }
    }
    threadGroup->submit(group1);
    //threadGroup->submit(group2);
    //threadGroup->submit(group3);
    //threadGroup->submit(group4);
    threadGroup->waitIdle();
    threadGroup->stop();
    return 0;
}
class A
{
    public:
        A(int tmp)
        {
            c = tmp;
        }
        int print()
        {
            //std::cout << "aaaaaaa " << c << std::endl;
            return c;
        }
    int c;
};

int fa(int a,int b)
{
    int c = a + b;
    std::cout << "fa = " << c << std::endl;
    return c;
}


int main1()
{
    auto func = [](int a,int b)->int{
        int c = a + b;
        std::cout << "res = " << c << std::endl;
        return c;
    };
    Task::Callable<int(int, int), 64, 8> call1(std::move(func));
    //call1.addInvokeObject(std::move(func));

    Task::Callable<int(int, int), 64, 8> call2(fa);
    //call2.addInvokeObject(fa);

    A a(5);
    Task::Callable<int(void), 64, 8> call3(&a, &A::print);
    //call3.addInvokeObject<A>(&a, &A::print);

    call1.call(1,2);
    call2.call(3,4);
    call3.call();
    return 0;
}