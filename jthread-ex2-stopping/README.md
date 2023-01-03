# Example 2 - Stopping
This example covers the use of a `std::stop_source` to terminate a running `std::jthread`. Unlike the previous example, the functions used in this example are interruptable.

## Building
### Visual studio
You can either load the project/solution into Visual studio, or the root directory contains the solution file to build all examples.
### Linux
A (very) simple shell script, `build_all.sh` exists in the root directory, allowing you to build all examples. You can, however build with:

```bash:../build_all.sh -s 7
g++ -std=c++20 -o ex2 jthread-ex2-stopping/jthread-ex2-stopping.cpp
```

## Stop sources and tokens
A `std::stop_source` (see [documentation](TODO)) is a new class - TODO add description and link to `std::stop_token`.

## Describing the function
As with the previous example, there are two functions; one with arguments, one without. The latter calls the former with some default parameters.

```cpp:jthread-ex2-stopping.cpp -s52 -e74
/// @brief  A function that takes in a stop_token from the jthread, and three 
///         user arguments. This function observes the stop_token to exit 
///         politely.
///         Until it receives a stop signal, it will continue running.
/// @param  token       The stop token for this thread
/// @param  foreground  Foreground colour for disambiguation
/// @param  name        The name given to this action
/// @param  delay       The time between stop checks and dot prints
static void interruptibleArgs(std::stop_token token,
    const int foreground,
    const std::string name,
    const std::chrono::milliseconds delay
)
{
    LOG(foreground, name, "Starting thread until stopped.");
    while (!token.stop_requested())
    {
        DOT(foreground);
        std::this_thread::sleep_for(delay);
    }
    
    LOG(foreground, name, "Leaving thread.");
}
```

The function accepts four arguments:
- `token` - A `std::stop_token` object indicating whether the thread is to stop
- `foreground` - An integer representing the colour for this thread to use when printing messages to ther terminal
- `name` - The name of the thread to help disambiguate messages printed to the terminal
- `delay` - The amount of time for the thread to sleep between checks of the stop token, `token`.

On entry, the thread prints a message indicating that it has started. Once into the `while` loop, a coloured dot is printed to the terminal, and then the thread sleeps. If the value returned from `token.stop_requested()` indicates that no stop is requested, the loop continues.

Upon exit, a message is printed indicating such.



