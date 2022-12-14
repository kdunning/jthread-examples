/**
 * @file    jthread-ex2-stopping.cpp
 *
 * @brief   Simple example of using std::stop_source to politely terminate a
 *          running std::jthread, with and without parameters.
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

/// @brief  A function that takes only the jthread's stop_token.
///         This function will continue until stopped, sending a 
///         coloured dot to the output at regular intervals.
static void interruptible(std::stop_token token)
{
    interruptibleArgs(token, COL_MAG, "Unnamed Magenta Thread", 100ms);
}

/// @brief  Main
int main(int argc, char** argv)
{
    static const std::string NAME = "Main";
    // Runs the function without arguments - this will print a dot to screen in
    // a default colour until stopped.
    std::jthread iThread(interruptible);

    // Slight sleep to allow the thread thread to print to screen before the
    // next thread starts.
    std::this_thread::sleep_for(500ms);

    // Runs the function with arguments. This will display frequent red dots
    // until stopped.
    std::jthread iThreadQuick(interruptibleArgs, COL_RED, "Quick Red Thread", 25ms);

    // Slight sleep to allow the thread thread to print to screen before the
    // next thread starts.
    std::this_thread::sleep_for(500ms);

    // Runs the function with arguments. This will display infrequent green 
    // dots until stopped.
    std::jthread iThreadSlow(interruptibleArgs, COL_GRN, "Slow Green", 250ms);

    // Add a delay allowing each thread to run for a few seconds
    std::this_thread::sleep_for(3s);

    // Check that the thread is stoppable, then stop it
    if (iThread.get_stop_token().stop_possible())
    {
        LOG(COL_BLU, NAME, "Stopping the parameterless thread");
        iThread.request_stop();
    }

    // Delay before stopping the next
    std::this_thread::sleep_for(1s);

    if (iThreadQuick.get_stop_token().stop_possible())
    {
        LOG(COL_BLU, NAME, "Stopping the quick red thread");
        iThreadQuick.request_stop();
    }

    // Deyal before stopping the next
    std::this_thread::sleep_for(1s);

    if (iThreadSlow.get_stop_token().stop_possible())
    {
        LOG(COL_BLU, NAME, "Stopping the slow green thread");
        iThreadSlow.request_stop();
    }

    // Note - there's no need to join any of the threads

    LOG(COL_BLU, NAME, "About to leave the main thread");
    return 0;
}
