#include "SessionThreadPool.hpp"
#include "GameSession.hpp"
#include <iostream>

namespace rtype::server {

SessionThreadPool::SessionThreadPool(size_t num_workers)
{
    workers_.reserve(num_workers);
    for (size_t i = 0; i < num_workers; ++i)
        workers_.emplace_back(&SessionThreadPool::worker_loop, this);
    std::cout << "[SessionThreadPool] Created thread pool with " << num_workers << " workers\n";
}

SessionThreadPool::~SessionThreadPool()
{
    shutdown_.store(true, std::memory_order_release);
    task_cv_.notify_all();

    for (auto& worker : workers_)
        if (worker.joinable())
            worker.join();
    std::cout << "[SessionThreadPool] Destroyed thread pool\n";
}

void SessionThreadPool::schedule_batch(const std::vector<SessionTask>& tasks)
{
    if (tasks.empty())
        return;
    tasks_completed_.store(0, std::memory_order_release);
    tasks_pending_.store(tasks.size(), std::memory_order_release);
    {
        std::lock_guard lock(queue_mutex_);
        for (const auto& task : tasks)
            task_queue_.push(task);
    }
    task_cv_.notify_all();
}

void SessionThreadPool::wait_for_completion()
{
    std::unique_lock lock(completion_mutex_);
    completion_cv_.wait(lock, [this] {
        return tasks_completed_.load(std::memory_order_acquire) ==
               tasks_pending_.load(std::memory_order_acquire);
    });
}

void SessionThreadPool::worker_loop()
{
    while (!shutdown_.load(std::memory_order_acquire)) {
        SessionTask task;
        {
            std::unique_lock lock(queue_mutex_);
            task_cv_.wait(lock, [this] {
                return !task_queue_.empty() || shutdown_.load(std::memory_order_acquire);
            });
            if (shutdown_.load(std::memory_order_acquire))
                break;
            if (!task_queue_.empty()) {
                task = task_queue_.front();
                task_queue_.pop();
            } else {
                continue;
            }
        }
        if (task.session)
            task.session->update(task.delta_time);
        size_t completed = tasks_completed_.fetch_add(1, std::memory_order_acq_rel) + 1;
        if (completed == tasks_pending_.load(std::memory_order_acquire)) {
            std::lock_guard lock(completion_mutex_);
            completion_cv_.notify_one();
        }
    }
}

}
