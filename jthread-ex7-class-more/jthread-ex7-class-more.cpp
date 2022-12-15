/**
 * @file    jthread-ex7-class-more.cpp
 *
 * @brief   Further class based examples, showing a thread pool making use of
 *          the new wait() method associated with std::condition_variable_any,
 *          allowing a stop token to be passed in, reducing the number of lines
 *          required to terminate a thread.
 *          This also shows the limitation of this feature, in that threads will
 *          continue even after being told to stop if their condition variable's
 *          predicate conditions are met.
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
#include <queue>
#include <vector>

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
/// @brief  Value to set the terminal text cyan
static const int COL_CYN = 36;

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

/// @brief  Short-hand name for the lamda provided for callback
using Callback = std::function<void(void)>;

/// @brief  Class that uses a std::jthread to operate stoppable actions
class WorkerThread
{
public:
    /// @brief  Constructor
    /// @param  name            The name of the thread
    /// @param  colour          The colour to be used for logging
    /// @param  finishEarly     Indicates whether this thread is permitted to
    ///                         finish early if a stop is requested.
    WorkerThread(const std::string name, const int colour, bool finishEarly)
        : mName(name)
        , mColour(colour)
        , mFinishEarly(finishEarly)
    {
        LOG(mColour, mName, "Constructed");
    }

    /// @brief  Destructor
    virtual ~WorkerThread()
    {
        stop();
    }

    /// @brief  Starts the thread
    /// @param  mutex   The mutex protecting the queue
    /// @param  cv      The condition variable to assess the queue
    /// @param  queue   The queue of work items to process
    void start(
        std::mutex &mutex,
        std::condition_variable_any &cv,
        std::queue<int> &queue
    )
    {
        // Firstly, we need to bind the worker method along with this instance.
        // Secondly, each reference value must be passed in using std::ref().
        mThread = std::jthread(
            std::bind_front(&WorkerThread::worker, this),
            std::ref(mutex), std::ref(cv), std::ref(queue)
        );
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

protected:

    /// @brief  Worker for an interruptable thread. This takes note of the
    ///         stop_token and exits as required. A built-in delay is added
    ///         to demonstrate the delay between requesting a stop and the
    ///         thread stopping, i.e. when calling stop() as a blocking call.
    /// @param  token   The stop token associated with this thread
    /// @param  mutex   The mutex protecting the queue
    /// @param  cv      The condition variable used to assess the queue
    /// @param  queue   The queue containing work items
    void worker(
        const std::stop_token &token,
        std::mutex &mutex,
        std::condition_variable_any &cv,
        std::queue<int> &queue
    )
    {
        LOG(mColour, mName, "Starting worker");
        while (true)
        {
            int value = 0;
            {
                std::unique_lock lock(mutex);
                // The new method wait() accepts a std::stop_token, and returns
                // the result from the predicate, and so, if the result is
                // false, we know that stop was requested. Otherwise, there are
                // items on the queue, and therefore, work to be done.
                //
                // If work remains but a stop request has been made, we cannot
                // tell without checking the token manually, and so the
                // mFinishEarly value lets this thread know whether it should
                // continue working or exit immediately if a stop was requested.
                if (!cv.wait(lock, token, [&]() { return queue.size() > 0; }) ||
                    (mFinishEarly && token.stop_requested()))
                {
                    break;
                }
                value = queue.front();
                LOG(mColour, mName, "Doing action with ID: " << value);
                queue.pop();
            }
            // This must be done outside of the lock, so that other threads
            // may have chance to work on other items in the queue
            std::this_thread::sleep_for(value * 100ms);
        }
        
        LOG(mColour, mName, "Leaving worker");
    }

    /// @brief  The thread name
    const std::string mName;
    /// @brief  The colour to use in logging
    const int mColour;
    ///  @brief Allows the thread to finish if stop is requested, even if work remains
    const bool mFinishEarly;
    /// @brief  The underlying thread object
    std::jthread mThread;
    
};

/// @brief  Main
int main(int argc, char** argv)
{
    // The default number of threads in the pool
    int threadCount = 4;
    // Allow the user to provide a number of threads for test as a single
    // argument on the command line.
    if (argc > 1)
    {
        try
        {
            threadCount = std::stoi(argv[1]);
        }
        catch (std::exception)
        {
            LOG(COL_RED, "ERROR", "Invalid thread count: " << argv[1]);
        }        
    }

    // Constants
    static const std::string NAME = "Main";
    static const int COL = COL_BLU;
    static const int SPECIAL_THREAD = threadCount / 2;
    static const int COL_COUNT = 4;
    static const int COLS[COL_COUNT] = { COL_GRN, COL_YLW, COL_RED, COL_CYN };
    static const std::string NAME_PREFIX = "Worker_";

    LOG(COL, NAME, "Running with a pool of " << threadCount << " threads");

    // The task queue for the workers to act upon
    std::queue<int> taskQueue;
    std::mutex mutex;
    std::condition_variable_any cv;

    // Additional thread to be started on the death of another thread.
    // Note that this thread is not allowed to exit early, it must do all jobs
    // remaining in the queue before it is permitted to stop.
    WorkerThread extraThread("Extra", COL_MAG, false);

    // Initialise a group of worker threads and start them immediately, adding
    // one special stop_callback to trigger the extra (clean-up) thread.
    // Note that these threads are allowed to exit early, even if work is
    // remaining on the queue.
    std::vector<std::unique_ptr<WorkerThread>> threads;
    for (int i = 0; i < threadCount; ++i)
    {
        threads.push_back(std::make_unique<WorkerThread>(
            NAME_PREFIX + std::to_string(i + 1), COLS[i % COL_COUNT], true));
        threads.back()->start(mutex, cv, taskQueue);
        
        // Slight pause to help prevent overlapping prints to the terminal
        std::this_thread::sleep_for(2ms);
        
        // If it's the middle worker, we want its death to trigger the 
        // start of the extra thread
        if (i == SPECIAL_THREAD)
        {
            threads.back()->addCallback([&]() {
                extraThread.start(mutex, cv, taskQueue);
            });
        }
    }

    // Allow all threads to run briefly
    std::this_thread::sleep_for(1s);
    
    // Use the known time from the worker threads to calculate a delay
    int delayMultiplier = 0;
    // Add a bundle of tasks to the queue
    for (int i = threadCount * 10; i > 0; --i)
    {
        delayMultiplier += i;
        std::unique_lock lock(mutex);
        taskQueue.push(i);
        // Notify one thread at a time
        cv.notify_one();
    }

    // Sleep for enough time for the extra thread to have to work for about 10
    // seconds
    delayMultiplier /= threadCount;
    std::this_thread::sleep_for(
        (delayMultiplier * 100ms) - (10000ms / threadCount)
    );

    // Now, kill off the thread collection, which will trigger the start of the
    // extra thread to clean up any remaining jobs
    LOG(COL, NAME, "Killing thread pool");
    for (auto &thread : threads)
    {
        thread->stop();
    }

    LOG(COL, NAME, "Waiting for the extra thread to finish the jobs...");
    // Stop the thread and wait until it has finished
    extraThread.stop(true);
    LOG(COL, NAME, "All jobs complete.");

    return 0;
}
