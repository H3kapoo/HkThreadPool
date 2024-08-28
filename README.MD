# Thread Pool

### Requirements
 - CMake installed.
 - C++17 or higher (change inside CMakeLists.txt). 
 - Ninja make generator (but you can change and use any other generator).

### Compile
```bash
./build.sh
```

### Usage
Please check ```src/main.cpp```:

```Cpp
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
```

### Notes
 - Windows/MacOS should be default supported as this used only the standard library.