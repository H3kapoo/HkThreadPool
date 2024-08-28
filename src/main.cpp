#include "ThreadPool.hpp"
#include "Utility.hpp"

int main(int argc, char** argv)
{
    hk::ThreadPool tp(4);
    std::vector<std::future<int>> futures;

    auto fun = [](int i)
    {
        println("asta e threadul %d", i);
        return i;
    };

    for (int i = 0; i < 10; i++)
    {
        futures.emplace_back(tp.enqueue(fun, i));
    }

    for (auto& ft : futures)
    {
        ft.get();
    }

    return 0;
}