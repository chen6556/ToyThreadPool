#include <iostream>
#include "ToyThreadPool.hpp"

int func(int &i)
{
    return ++i;
}

int main()
{
    ToyThreadPool threads(4);
    int a = 0, b = 1;

    threads.add_task(func, std::ref(a));
    threads.add_task(func, b);

    threads.join();

    std::cout << "a : " << a << " b : " << b << std::endl;
    return 0;
}