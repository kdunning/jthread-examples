/**
 * @file    jthread-ex1-basic.cpp
 *
 * @brief   Basic example of a std::jthread accepting a simple static function
 *          with and without parameters.
 *
 * @author  Kris Dunning (kris.dunning@itdev.co.uk)
 * @date    2022
 */

#include <thread>
#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

/// @brief  Provides a standardised log message
#define LOG(name, msg) std::cout << name << ": " << msg << std::endl;

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

/// @brief  A function that takes no arguments, but blocks until it has
///         completed for a known amount of time.
static void uninterruptible()
{
    uninterruptibleArgs("Unnamed Thread", 500ms);
}

/// @brief  Main
int main(int argc, char** argv)
{
    // Runs the function without arguments - this will run for a set length of
    // time - in this example, deliberately longer than thread uThreadQuick.
    std::jthread uThread(uninterruptible);

    // Slight sleep to allow the thread thread to print to screen before the
    // next thread starts.
    std::this_thread::sleep_for(10ms);

    // Runs the function with arguments. This will complete before the previous
    // thread.
    std::jthread uThreadQuick(uninterruptibleArgs, "Quick Thread", 25ms);

    // Slight sleep to allow the thread thread to print to screen before the
    // next thread starts.
    std::this_thread::sleep_for(10ms);

    // Runs the function with arguments. This will complete last, after all of
    // the other threads (except main)
    std::jthread uThreadSlow(uninterruptibleArgs, "Slow Thread", 3000ms);

    // There is no need to join any std::jthread, however, this can be used to
    // print the final line of this main function, such that it appears after
    // the quick thread, but before the slow thread.
    uThread.join();
    LOG("Main", "About to leave the main thread");
    return 0;
}
