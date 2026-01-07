#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace rtype::server {

class GameSession;

/**
 * @brief Task to be executed by a worker thread
 */
struct SessionTask {
    GameSession* session;
    float delta_time;
};

/**
 * @brief Thread pool for parallel GameSession updates
 *
 * This thread pool uses a fixed number of worker threads to execute
 * GameSession::update() calls in parallel. It implements a barrier
 * synchronization pattern where the main thread schedules a batch of
 * tasks, waits for all workers to complete, then proceeds.
 *
 * Thread-safe: All public methods can be called from the main thread.
 */
class SessionThreadPool {
public:
    /**
     * @brief Construct a thread pool with the specified number of workers
     * @param num_workers Number of worker threads (default: 6)
     */
    explicit SessionThreadPool(size_t num_workers = 6);

    /**
     * @brief Destructor - waits for all workers to finish and joins them
     */
    ~SessionThreadPool();

    SessionThreadPool(const SessionThreadPool&) = delete;
    SessionThreadPool& operator=(const SessionThreadPool&) = delete;
    SessionThreadPool(SessionThreadPool&&) = delete;
    SessionThreadPool& operator=(SessionThreadPool&&) = delete;

    /**
     * @brief Schedule a batch of tasks for execution
     *
     * This method queues all tasks and wakes up worker threads.
     * It does NOT wait for completion - use wait_for_completion() for that.
     *
     * @param tasks Vector of tasks to execute
     */
    void schedule_batch(const std::vector<SessionTask>& tasks);

    /**
     * @brief Wait for all scheduled tasks to complete (barrier)
     *
     * Blocks the calling thread until all tasks from the last
     * schedule_batch() call have been completed by workers.
     */
    void wait_for_completion();

    /**
     * @brief Get the number of worker threads
     * @return Number of workers in the pool
     */
    size_t get_worker_count() const { return workers_.size(); }

private:
    /**
     * @brief Worker thread main loop
     *
     * Each worker repeatedly:
     * 1. Waits for tasks in the queue
     * 2. Pops a task
     * 3. Executes GameSession::update()
     * 4. Signals completion
     */
    void worker_loop();

    std::vector<std::thread> workers_;

    std::queue<SessionTask> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable task_cv_;

    std::mutex completion_mutex_;
    std::condition_variable completion_cv_;
    std::atomic<size_t> tasks_pending_{0};
    std::atomic<size_t> tasks_completed_{0};

    std::atomic<bool> shutdown_{false};
};

}
