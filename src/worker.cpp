#include "worker.hpp"
#include <chrono>

using namespace sentry;

BackgroundWorker::BackgroundWorker() : m_running(false) {
}

void BackgroundWorker::start() {
    if (m_running) {
        return;
    }

    SENTRY_LOG("starting background worker");
    m_running = true;
    m_thread = std::thread([this]() {
        while (m_running) {
            std::function<void()> *task = nullptr;
            bool got_task = false;
            {
                std::lock_guard<std::mutex> _lock(m_task_lock);
                if (!m_tasks.empty()) {
                    task = m_tasks.front();
                    m_tasks.pop_front();
                    got_task = true;
                }
            }

            if (!got_task) {
                std::unique_lock<std::mutex> lock(m_wake_lock);
                m_wake.wait_for(lock, std::chrono::seconds(5));
            } else if (task) {
                (*task)();
            } else {
                m_running = false;
                m_wake.notify_one();
            }
        }
        SENTRY_LOG("background worker shut down");
    });
    m_thread.detach();
}

void BackgroundWorker::kill() {
    SENTRY_LOG("killing background worker");
    {
        std::lock_guard<std::mutex> _lock(m_task_lock);
        m_tasks.push_back(nullptr);
    }
    m_wake.notify_all();
}

void BackgroundWorker::shutdown() {
    SENTRY_LOG("shutting down background worker");
    {
        std::lock_guard<std::mutex> _lock(m_task_lock);
        m_tasks.push_back(nullptr);
    }
    m_wake.notify_all();

    std::chrono::system_clock::time_point started =
        std::chrono::system_clock::now();
    while (true) {
        {
            std::lock_guard<std::mutex> _lock(m_task_lock);
            if (m_tasks.empty()) {
                break;
            }
        }
        std::unique_lock<std::mutex> lock(m_wake_lock);
        m_wake.wait_for(lock, std::chrono::seconds(1));

        std::chrono::duration<double> diff =
            std::chrono::system_clock::now() - started;
        if (diff.count() >= 5.0) {
            break;
        }
    }
}

void BackgroundWorker::submitTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> _lock(m_task_lock);
        m_tasks.push_back(new std::function<void()>(task));
    }
    m_wake.notify_one();
}
