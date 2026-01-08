#include "FrameGraph.h"
#include "Task.h"

class A
{
    public:
        A(int tmp)
        {
            c = tmp;
        }
        int print()
        {
            std::cout << "aaaaaaa " << c << std::endl;
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

int main()
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