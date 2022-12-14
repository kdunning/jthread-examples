/**
 * @file    jthread-ex6-class-adv.cpp
 *
 * @brief   Example using a class to show more thorough std::jthread examples.
 *          Within this example, a class is created and used to control a 
 *          std::jthread, allowing full control over terminating the thread.
 *
 * @author  Kris Dunning (kris.dunning@itdev.co.uk)
 * @date    2022
 */

#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <functional>
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
/// @brief  Value to set the terminal text yellow
static const int COL_YLW = 33;

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

/// @brief  Short-hand name for the lamda provided for callback
using Callback = std::function<void(void)>;

/// @brief  Class that uses a std::jthread to operate stoppable actions
template<typename T>
class WorkerThread
{
public:
    /// @brief  Constructor
    /// @param  name            The name of the thread
    /// @param  colour          The colour to be used for logging
    /// @param  defaultValue    The default value
    WorkerThread(const std::string &name, const int colour, T defaultValue)
        : mName(name)
        , mColour(colour)
        , mData(defaultValue)
    {
        LOG(mColour, mName, "Constructed");
    }

    /// @brief  Destructor
    virtual ~WorkerThread()
    {
        stop();
    }

    /// @brief  Starts the thread in the correct mode
    /// NOTE:   Because we are passing an argument into the worker thread,
    ///         we need to bind the method.
    void start()
    {
        // For reference, if starting a thread with additional parameters,
        // they would be set after the std::bind_front() call, e.g.
        // std::jthread(std::bind_front(&ClassName::MethodName, this), anInt, aStr);
        // 
        // The signature for the bound method would be either:
        // void MethodName(std::stop_token, int, std::string) or 
        // void MethodName(int, std::string) or 
        mThread = std::jthread(std::bind_front(&WorkerThread::worker, this));
    }

    /// @brief  Stops the thread, blocking if requested
    /// @param  block    Whether the call is blocking (i.e. joins)
    void stop(bool block = false)
    {
        if (mThread.get_stop_token().stop_possible())
        {
            mThread.get_stop_source().request_stop();
            if (block)
            {
                mThread.join();
            }
        }
    }

    /// @brief  Adds a stop_callback as requested
    /// @param cb   The callback to be triggered when stop is requested
    /// @returns    Pointer to the new stop_callback
    std::stop_callback<Callback> *addCallback(Callback cb)
    {
        std::stop_callback<Callback> *v = new std::stop_callback<Callback>(mThread.get_stop_token(), cb);
        return v;
    }

    /// @brief  Joins the thread, if it is running
    void join()
    {
        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    /// @brief  Sets the thread data, allowing the thread loop to check
    ///         for the desired data
    /// @param  data    The data to be set
    void setData(T data)
    {
        if (mThread.joinable() && !mThread.get_stop_token().stop_requested())
        {
            std::lock_guard lock(mMutex);
            mData = data;
            mCv.notify_all();
        }
    }

protected:

    /// @brief  Worker for an interruptable thread. This takes note of the
    ///         stop_token and exits as required. A built-in delay is added
    ///         to demonstrate the delay between requesting a stop and the
    ///         thread stopping, i.e. when calling stop() as a blocking call.
    void worker(const std::stop_token &token)
    {
        LOG(mColour, mName, "Starting worker");
        // Signal the condition variable to check the current conditions
        std::stop_callback stop(token, [&]() { mCv.notify_all(); });
        bool done = false;
        while (!token.stop_requested() && !done)
        {
            DOT(mColour);
            std::unique_lock lock(mMutex);
            mCv.wait(lock, [&]() {
                return (done = taskComplete()) || token.stop_requested();
            });
            LOG(mColour, mName, "Token: " << token.stop_requested() <<
                " | Data: " << done);
        }
        std::cout << std::endl;
        LOG(mColour, mName, "Leaving worker");
    }

    /// @brief  Abstract method, used to indicate whether the thread is complete
    virtual bool taskComplete() = 0;

    /// @brief  The thread name
    const std::string &mName;
    /// @brief  The colour to use in logging
    const int mColour;
    /// @brief  The underlying thread object
    std::jthread mThread;
    /// @brief  Mutex used to protect the data value
    std::mutex mMutex;
    /// @brief  Condition variable used to signal when the thread data changes
    std::condition_variable mCv;
    /// @brief  The thread data
    T mData;
};

/// @brief  Special worker class expecting the data to be true before finishing
class BoolWorkerThread : public WorkerThread<bool>
{
public:
    /// @brief  Constructor
    /// @param  name            The name of the thread
    /// @param  colour          The colour to be used for logging
    BoolWorkerThread(const std::string &name, const int colour)
    : WorkerThread(name, colour, false)
    {}

protected:

    /// @brief  Indicates whether the task is complete
    /// NOTE: This is thread safe, as its usage is protected by the lock
    ///       within the thread method's condition variable usage.
    virtual bool taskComplete() override
    {
        return mData;
    }
};

/// @brief  Special worker class expecting the value to match a target
class IntWorkerThread : public WorkerThread<int>
{
public:
    /// @brief  Constructor
    /// @param  name            The name of the thread
    /// @param  colour          The colour to be used for logging
    /// @param  target          The target value to complete the thread
    IntWorkerThread(const std::string &name, const int colour, const int target)
    : WorkerThread(name, colour, 0)
    , mTarget(target)
    {
    }

protected:
    
    /// @brief  Indicates whether the task is complete
    /// NOTE: This is thread safe, as its usage is protected by the lock
    ///       within the thread method's condition variable usage.
    virtual bool taskComplete() override
    {
        return mTarget == mData;
    }

    /// @brief  The target value to complete the thread
    const int mTarget;
};


/// @brief  Main
int main(int argc, char** argv)
{
    static const std::string NAME = "Main";
    static const int COL = COL_BLU;

    static const int INT_1_TARGET = 10;
    static const int INT_2_TARGET = 3;

    static const std::string BOOL_1_THREAD_NAME = "Bool Thread 1";
    static const std::string BOOL_2_THREAD_NAME = "Bool Thread 2";
    static const std::string INT_1_THREAD_NAME =
        "Target Thread (" + std::to_string(INT_1_TARGET) + ")";
    static const std::string INT_2_THREAD_NAME =
        "Target Thread (" + std::to_string(INT_2_TARGET) + ")";

    BoolWorkerThread boolThread1(BOOL_1_THREAD_NAME, COL_GRN);
    BoolWorkerThread boolThread2(BOOL_2_THREAD_NAME, COL_MAG);
    IntWorkerThread intThread1(INT_1_THREAD_NAME, COL_RED, INT_1_TARGET);
    IntWorkerThread intThread2(INT_2_THREAD_NAME, COL_YLW, INT_2_TARGET);
    
    // Start all but the second integer thread with delay between them to
    // prevent messages interrupting
    boolThread1.start();
    std::this_thread::sleep_for(10ms);
    boolThread2.start();
    std::this_thread::sleep_for(10ms);
    intThread1.start();

    // Allow all threads to run for a few seconds
    std::this_thread::sleep_for(4s);

    // Stop the first bool thread by setting the data value to true
    boolThread1.setData(true);

    // Stop the second bool thread by politely asking it to stop, then
    // waiting for it before continuing.
    boolThread2.stop(true);

    // Set the stop callback for the first integer thread so that it 
    // triggers the start of the second
    auto lambda = [&intThread2]() {
        LOG(COL, NAME, "Starting thread");
        intThread2.start();
    };
    std::unique_ptr<std::stop_callback<Callback>> startSecond(
        intThread1.addCallback(lambda)
    );
    // Let the first run for a brief while
    std::this_thread::sleep_for(1s);
    intThread1.stop(true);

    // Stop the second thread by setting the value around the target
    const int start = INT_2_TARGET - 2;
    const int end = INT_2_TARGET + 3;
    for (int i = start; i < end; ++i)
    {
        LOG(COL, NAME, "Setting target to " << i);
        intThread2.setData(i);
        std::this_thread::sleep_for(1s);
    }

    // The second thread should have completed due to conditions above 

    return 0;
}
