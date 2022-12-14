/**
 * @file    jthread-ex4-adv-stopping.cpp
 *
 * @brief   Example using condition variables to terminate a blocking call
 *          within a std::jthread. This example is roughly based on the previous
 *          with some modifications.
 *
 * @author  Kris Dunning (kris.dunning@itdev.co.uk)
 * @date    2022
 */

#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>

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

/// @brief  Function that waits for a value to be updated before continuing.
///         Until it receives a stop signal, or the data has been signalled
///         as done, it will continue waiting.
/// 
/// @param  token       The stop token for this thread
/// @param  foreground  Foreground colour for disambiguation
/// @param  name        The name given to this action
/// @param  mutex       The data mutex (protecting "blockingRes" value)
/// @param  cv          Condition variable to check for changes
/// @param  blockingRes Reference to the blocking data being processed
static void blockingWorker(std::stop_token token,
    const int foreground,
    const std::string name,
    std::mutex &mutex,
    std::condition_variable &cv,
    bool &blockingRes
)
{
    LOG(foreground, name, "Starting thread until data ready or stopped.");

    // Here we have a callback within the thread function, which will
    // break free from the condition_variable's wait.
    std::stop_callback cb(token, [foreground, name, &cv]() {
            LOG(foreground, name + "_CB", "Thread terminating...");
            cv.notify_all();
        }
    );
    bool done = false;
    do
    {
        DOT(foreground);
        std::unique_lock lock(mutex);
        cv.wait(lock, [&]() {
            return (done = blockingRes) || token.stop_requested();
        });
        LOG(foreground, name, "Token: " << token.stop_requested() << " | Data: " << done);
    } while (!token.stop_requested() && !done);

    LOG(foreground, name, "Leaving thread.");
}

/// @brief  Stops the thread, if it is stoppable
/// @param  jt          The std::jthread to be stopped
/// @param  col         The colour to print log messages in
/// @param  name        The name of the calling thread
/// @param  thrName     The name of the thread to be closed
static void stopThread(std::jthread& jt, int col, 
    const std::string &name, const std::string &thrName
)
{
    if (jt.get_stop_token().stop_possible())
    {
        LOG(col, name, "Stopping thread: " << thrName);
        jt.get_stop_source().request_stop();
    }
}

/// @brief  Main
int main(int argc, char** argv)
{
    static const std::string NAME = "Main";
    static const int COL = COL_BLU;

    /*
     * Thread to be stopped via "unblocking" data
     */
    const std::string DATA_RELEASE_THREAD_NAME = "Data Release Red";
    std::mutex dataRelThrMutex;
    std::condition_variable dataRelThrCv;
    bool dataRelData = false;
    // Start the thread
    std::jthread dataRelThread(blockingWorker, COL_RED, DATA_RELEASE_THREAD_NAME,
        std::ref(dataRelThrMutex), std::ref(dataRelThrCv), std::ref(dataRelData));

    // Slight sleep to allow the thread thread to print to screen before the
    // next thread starts.
    std::this_thread::sleep_for(500ms);

    /*
     * Thread to be stopped by stopping the thread manually
     */
    const std::string MAN_STOP_THREAD_NAME = "Manually Stopped Green";
    std::mutex manRelThrMutex;
    std::condition_variable manRelThrCv;
    bool manRelData = false;
    // Start the thread
    std::jthread manRelThread(blockingWorker, COL_GRN, MAN_STOP_THREAD_NAME,
        std::ref(manRelThrMutex), std::ref(manRelThrCv), std::ref(manRelData));

    // Add a delay allowing each thread to run for a few seconds
    std::this_thread::sleep_for(3s);

    // Stop the data release thread by setting the data to true and signalling
    // the condition variable
    LOG(COL, NAME, "Unblocking " << DATA_RELEASE_THREAD_NAME << " data");
    {
        std::lock_guard lock(dataRelThrMutex);
        dataRelData = true;
        dataRelThrCv.notify_all();
    }

    // Delay before stopping the next
    std::this_thread::sleep_for(2s);

    // Stopping the manual release thread
    LOG(COL, NAME, "Stopping " << MAN_STOP_THREAD_NAME);
    stopThread(manRelThread, COL, NAME, MAN_STOP_THREAD_NAME);

    // Note - there's no need to join any of the threads

    LOG(COL, NAME, "About to leave the main thread");
    return 0;
}
