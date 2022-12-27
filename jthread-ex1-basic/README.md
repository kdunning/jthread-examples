# Example 1 - Basic
This example covers a basic `std::jthread` using a static function with and without parameters to show the general usage.

## Building
### Visual studio
You can either load the project/solution into Visual studio, or the root directory contains the solution file to build all examples.
### Linux
A (very) simple shell script, `build_all.sh` exists in the root directory, allowing you to build all examples. You can, however build with:

```bash:../build_all.sh [4]
g++ -std=c++20 -o ex1 jthread-ex1-basic/jthread-ex1-basic.cpp
```

## Uninterruptable Function
There are two uninterruptable functions, one with and one without arguments. This is to demonstrate how to bind a `std::jthread` to both.

The function without arguments simply calls the other with default arguments, so we shall focus on that.

```cpp:jthread-ex1-basic.cpp    [20-31]
/// @brief  A function that takes in two arguments and blocks until completion.
/// @param  name    The name given to this action
/// @param  delay   The time this function will run for
static void uninterruptibleArgs(
    const std::string &name,
    const std::chrono::milliseconds &delay
)
{
    LOG(name, "Thread will terminate in " << delay.count() << " milliseconds");
    std::this_thread::sleep_for(delay);
    LOG(name, "Leaving thread.");
}
```
The function deliberately blocks using a simple sleep. Without sending an interrupt to the thread, this cannot be stopped.
The parameter `name` is to aid visibility and logging, and `delay` sets the amount of time the thread should sleep.

## Demonstration
To demonstrate this working, three threads are created.

### Thread Without Arguments
``` cpp:jthread-ex1-basic.cpp    [43-45]
    // Runs the function without arguments - this will run for a set length of
    // time - in this example, deliberately longer than thread uThreadQuick.
    std::jthread uThread(uninterruptible);
```
Here we can see a simple creation of the thread with just one argument passed in; the function name to run. This will run for the default length of time with the default name, as seen below:
``` cpp:jthread-ex1-basic.cpp    [37]
    uninterruptibleArgs("Unnamed Thread", 500ms);
```

### The "Quick" Thread
This thread is designed to only run for a short period of time.
``` cpp:jthread-ex1-basic.cpp    [51-53]
    // Runs the function with arguments. This will complete before the previous
    // thread.
    std::jthread uThreadQuick(uninterruptibleArgs, "Quick Thread", 25ms);
```
It makes use of the parameterised method, passing in a shorter time than the default.

### The "Slow" Thread
This thread is designed to run for longer than any of the others, like the "quick" thread, it passes in parameters on creation of the thread.
``` cpp:jthread-ex1-basic.cpp    [59-61]
    // Runs the function with arguments. This will complete last, after all of
    // the other threads (except main)
    std::jthread uThreadSlow(uninterruptibleArgs, "Slow Thread", 3000ms);
```

### Joining
With `std::jthread`, there is often no need to join the thread, as the destructor will join automatically if the thread is joinable. This means that the line:
``` cpp:jthread-ex1-basic.cpp    [66:69]
    uThreadSlow.join();

    // Final thread, runs without joining.
    std::jthread uFinalThread(uninterruptibleArgs, "Final Thread", 100ms);
```
is superfluous, but it can help when you wish to wait for a thread to complete before carrying out another action. In the above example, a final thread is triggered once the slow thread has completed.

## Output
Below is a captured output from running the code, with line numbers added for clarity.
```
1   Unnamed Thread: Thread will terminate in 500 milliseconds
2   Quick Thread: Thread will terminate in 25 milliseconds
3   Slow Thread: Thread will terminate in 3000 milliseconds
4   Quick Thread: Leaving thread.
5   Unnamed Thread: Leaving thread.
6   Slow Thread: Leaving thread.
7   Main: About to leave the main thread
8   Final Thread: Thread will terminate in 100 milliseconds
9   Final Thread: Leaving thread.
```
Lines 1-3 display that all three threads have started, noting that "Final Thread" does not start until line 8, after the slow thread has completed.

Lines 4-6 show that the first three threads have completed.

Line 7, a log message from the main thread shows that we have completed the main method and all that remains is waiting for the "Final Thread" to complete, without having to call `join()`.



## What have we learned
1. How to initialise a `std::jthread` with a static function with and without arguments.
2. Join can be called, but it is not essential.

