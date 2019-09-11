#ifndef SENTRY_WORKER_HPP_INCLUDED
#define SENTRY_WORKER_HPP_INCLUDED

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include "internal.hpp"

namespace sentry {

class BackgroundWorker {
   public:
    BackgroundWorker();
    void start();
    void kill();
    void shutdown();
    void submit_task(std::function<void()> task);

   private:
    std::condition_variable m_wake;
    std::mutex m_wake_lock;
    std::mutex m_task_lock;
    std::deque<std::function<void()> *> m_tasks;
    std::thread m_thread;
    bool m_running;
};

}  // namespace sentry

#endif
