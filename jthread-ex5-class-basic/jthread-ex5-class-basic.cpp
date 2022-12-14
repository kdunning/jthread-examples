/**
 * @file    jthread-ex5-class-basic.cpp
 *
 * @brief   Example using a class to show basic std::jthread examples.
 *          Within this example, there is both an interruptable and
 *          uninterruptable version, and uses of std::stop_callback to trigger
 *          actions.
 *
 * @author  Kris Dunning (kris.dunning@itdev.co.uk)
 * @date    2022
 */

#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <functional>

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

/// @brief  Simple class that uses a std::jthread to operate simple actions
class SimpleWorkerThrad
{
public:
    /// @brief  Constructor
    /// @param  name            The name of the thread
    /// @param  colour          The colour to be used for logging
    /// @param  interruptable   Whether  interruptable or uninterruptable
    SimpleWorkerThrad(const std::string &name, const int colour,
        const bool interruptable)
    : mName(name)
    , mColour(colour)
    , mInterruptable(interruptable)
    {
        LOG(mColour, mName, "Constructed");
    }

    /// @brief  Destructor
    virtual ~SimpleWorkerThrad()
    {
        stop();
    }

    /// @brief  Starts the thread in the correct mode
    void start()
    {
        if (mInterruptable)
        {
            mThread = std::jthread(&SimpleWorkerThrad::interruptableWorker, this);
        }
        else
        {
            mThread = std::jthread(&SimpleWorkerThrad::uninterruptableWorker, this);
        }
    }

    /// @brief  Stops the thread, blocking if requested
    /// @param  block    Whether the call is blocking (i.e. joins)
    void stop(bool block = false)
    {
        if (mThread.get_stop_token().stop_possible())
        {
            mThread.request_stop();
            if (block)
            {
                mThread.join();
            }
        }
    }

    /// @brief  Joins the thread, if it is running
    void join()
    {
        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    /// @brief  Short-hand name for the lamda provided for callback
    using Callback = std::function<void(void)>;

    /// @brief  Adds a stop_callback as requested
    /// @param cb   The callback to be triggered when stop is requested
    /// @returns    Pointer to the new stop_callback
    std::stop_callback<Callback> *addCallback(Callback cb)
    {
        std::stop_callback<Callback> *v = new std::stop_callback<Callback>(mThread.get_stop_token(), cb);
        return v;
    }

private:

    /// @brief  Worker for an interruptable thread. This takes note of the
    ///         stop_token and exits as required. A built-in delay is added
    ///         to demonstrate the delay between requesting a stop and the
    ///         thread stopping, i.e. when calling stop() as a blocking call.
    void interruptableWorker()
    {
        LOG(mColour, mName, "Starting interruptable worker");
        const std::stop_token &token = mThread.get_stop_token();
        while (!token.stop_requested())
        {
            DOT(mColour);
            std::this_thread::sleep_for(100ms);
        }
        std::cout << std::endl;
        // Slight delay to show that stopping with block set
        LOG(mColour, mName, "Adding delierate pause...");
        std::this_thread::sleep_for(1s);
        LOG(mColour, mName, "Leaving interruptable worker");
    }

    /// @brief  Worker for an uninterruptable thread. This simply runs until
    ///         completion. This is a very rare option, and should ideally be
    ///         replaced by some means of terminating the thread early.
    void uninterruptableWorker()
    {
        LOG(mColour, mName, "Starting uninterruptable worker");
        for (int i = 0; i < 20; ++i) 
        { 
            DOT(mColour);
            std::this_thread::sleep_for(250ms);
        }
        std::cout << std::endl;
        LOG(mColour, mName, "Leaving uninterruptable worker: " << mThread.get_stop_token().stop_possible());
    }

    /// @brief  The thread name
    const std::string &mName;
    /// @brief  The colour to use in logging
    const int mColour;
    /// @brief  Whether interruptable
    const bool mInterruptable;
    /// @brief  The underlying thread object
    std::jthread mThread;
};

/// @brief  Main
int main(int argc, char** argv)
{
    static const std::string NAME = "Main";
    static const int COL = COL_BLU;

    static const std::string UNINT_THREAD_NAME = "Uninterruptable";
    static const std::string INT_1_THREAD_NAME = "Interruptable 1";
    static const std::string INT_2_THREAD_NAME = "Interruptable 2";

    // Simple thread to show uninterruptable operation, it will run until it's
    // completed its task.
    
    SimpleWorkerThrad uninterruptableThread(UNINT_THREAD_NAME, COL_GRN, false);
    
    // First interruptable thread.     
    SimpleWorkerThrad interruptableThread_1(INT_1_THREAD_NAME, COL_RED, true);

    // Second interruptable thread. 
    SimpleWorkerThrad interruptableThread_2(INT_2_THREAD_NAME, COL_MAG, true);

    // Delay before starting
    std::this_thread::sleep_for(1s);
    uninterruptableThread.start();

    // Wait for the uninterruptable thread to complete, then start the first
    // of the interruptable threads
    uninterruptableThread.join();

    interruptableThread_1.start();
    // Trigger the start of the second thread based on the first
    auto lambda = [&interruptableThread_2]() {
        LOG(COL, NAME, "Callback triggered to start second thread");
        interruptableThread_2.start();
    };
    std::unique_ptr<std::stop_callback<SimpleWorkerThrad::Callback>> cb(
        interruptableThread_1.addCallback(lambda)
    );
    

    // Allow the first thread to run for a few seconds
    std::this_thread::sleep_for(3s);
    // Stop, but wait for it to complete before continuing
    LOG(COL, NAME, "Stopping and blocking on " << INT_1_THREAD_NAME);
    interruptableThread_1.stop(true);
    LOG(COL, NAME, INT_1_THREAD_NAME << " has completed.");

    // Another brief pause for the second thread before ending it
    std::this_thread::sleep_for(2s);
    interruptableThread_2.stop();

    return 0;
}
