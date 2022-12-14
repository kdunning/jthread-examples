/**
 * @file    jthread-ex3-more-stopping.cpp
 *
 * @brief   More complicated example of stopping std::jthreads.
 *          This is based on the previous example, extending some
 *          of the functionality to show some basic
 *          std::stop_callback uses.
 *
 * @author  Kris Dunning (kris.dunning@itdev.co.uk)
 * @date    2022
 */

#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>

using namespace std::chrono_literals;

/// @brief  The colour code prefix
static const std::string COL_PRE = "\033[1;";
/// @brief  The colour code suffix
static const std::string COL_SUF = "m";
/// @brief  Colour code to return to normal
static const std::string COL_NORM = "\033[0m";

/// @brief  Value to set the terminal text red
static const int COL_RED = 31;
/// @brief  Value to set the terminal text green
static const int COL_GRN = 32;
/// @brief  Value to set the terminal text blue
static const int COL_BLU = 34;
/// @brief  Value to set the terminal text magenta
static const int COL_MAG = 35;

/// @brief  Helper function to get the colour string so that it does not get
///         flushed to the terminal before it has been combined.
std::string getColour(int col)
{
    std::stringstream ss;
    ss << COL_PRE << std::to_string(col) << COL_SUF;
    return ss.str();
}

/// @brief  Provides a standardised log message with colour
#define LOG(col, name, msg)     std::cout << getColour(col) << name << ": " << \
                                msg << COL_NORM << std::endl;

/// @brief  Prints a coloured dot to screen, indicating a thread is still
///         running.
#define DOT(col)        std::cout << getColour(col) << "." << COL_NORM << \
                        std::flush;

/// @brief  A function that takes in a stop_token from the jthread, and three
///         user arguments. This function observes the stop_token to exit
///         politely.
///         Until it receives a stop signal, it will continue running.
///
///         When stopped, this method will use a std::stop_callback to
///         calculate the approximate run time of the thread.
///
/// @param  token       The stop token for this thread
/// @param  foreground  Foreground colour for disambiguation
/// @param  name        The name given to this action
/// @param  delay       The time between stop checks and dot prints
static void worker(std::stop_token token,
    const int foreground,
    const std::string name,
    const std::chrono::milliseconds delay
)
{
    auto start = std::chrono::system_clock::now();
    LOG(foreground, name, "Starting thread until stopped.");

    // Here we have a callback within the thread function, which will
    // take in local parameters and print some information to the
    // terminal.
    std::stop_callback cb(token, [foreground, name, start]() {
        const std::chrono::milliseconds duration =
            std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::system_clock::now() - start);
        LOG(foreground, name + "_CB", "Thread terminated after: " <<
            duration.count() << " ms");
    });
    while (!token.stop_requested())
    {
        DOT(foreground);
        std::this_thread::sleep_for(delay);
    }

    LOG(foreground, name, "Leaving thread.");
}

/// @brief  Main
int main(int argc, char** argv)
{
    static const std::string NAME = "Main";
    static const int COL = COL_BLU;

    // Runs the function with arguments. This will display frequent red dots
    // until stopped.
    std::jthread iThreadQuick(worker, COL_RED, "Quick Red Thread", 25ms);
    // Callback added here, acted upon when the thread above is stopped
    std::stop_callback quickCallback(iThreadQuick.get_stop_token(), []() {
            LOG(COL, NAME + "_CB", ">> Stop callback triggered from the quick thread <<");
        }
    );

    // Slight sleep to allow the thread thread to print to screen before the
    // next thread starts.
    std::this_thread::sleep_for(500ms);

    // Runs the function with arguments. This will display infrequent green
    // dots until stopped.
    std::jthread iThreadSlow(worker, COL_GRN, "Slow Green", 250ms);

    // Add a delay allowing each thread to run for a few seconds
    std::this_thread::sleep_for(3s);

    // Stop the quick thread, which should trigger the callback in main and
    // the callback within the function.
    if (iThreadQuick.get_stop_token().stop_possible())
    {
        LOG(COL, NAME, "Stopping the quick red thread");
        iThreadQuick.request_stop();
    }

    // Deyal before stopping the next
    std::this_thread::sleep_for(2s);

    // Stopping the slower thread will only show a callback from within
    // the thread itself.
    if (iThreadSlow.get_stop_token().stop_possible())
    {
        LOG(COL, NAME, "Stopping the slow green thread");
        iThreadSlow.request_stop();
    }

    // Note - there's no need to join any of the threads

    LOG(COL, NAME, "About to leave the main thread");
    return 0;
}
