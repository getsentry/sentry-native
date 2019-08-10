#include "worker.hpp"
#include <chrono>

using namespace sentry;

BackgroundWorker::BackgroundWorker() : m_running(true) {
}

void BackgroundWorker::start() {
    if (m_running) {
        return;
    }

    m_thread = std::thread([this]() {
        while (m_running) {
            std::function<void()> *task = nullptr;
            {
                std::lock_guard<std::mutex> _lock(m_task_lock);
                if (!m_tasks.empty()) {
                    task = m_tasks.front();
                    m_tasks.pop_front();
                } else {
                    std::unique_lock<std::mutex> lock(m_wake_lock,
                                                      std::defer_lock);
                    m_wake.wait_for(lock, std::chrono::seconds(5));
                    continue;
                }
            }
            if (task) {
                (*task)();
            } else {
                m_running = false;
                m_wake.notify_one();
            }
        }
    });
    m_thread.detach();

    m_running = true;
}

void BackgroundWorker::kill() {
    std::lock_guard<std::mutex> _lock(m_task_lock);
    m_tasks.push_back(nullptr);
    m_wake.notify_all();
}

void BackgroundWorker::shutdown() {
    {
        std::lock_guard<std::mutex> _lock(m_task_lock);
        m_tasks.push_back(nullptr);
        m_wake.notify_all();
    }

    std::chrono::system_clock::time_point started =
        std::chrono::system_clock::now();
    while (true) {
        std::lock_guard<std::mutex> _lock(m_task_lock);
        if (m_tasks.empty()) {
            break;
        }
        std::unique_lock<std::mutex> lock(m_wake_lock, std::defer_lock);
        m_wake.wait_for(lock, std::chrono::seconds(1));

        std::chrono::duration<double> diff =
            std::chrono::system_clock::now() - started;
        if (diff.count() >= 5.0) {
            break;
        }
    }
}

void BackgroundWorker::submitTask(std::function<void()> task) {
    std::lock_guard<std::mutex> _lock(m_task_lock);
    m_tasks.push_back(new std::function<void()>(task));
    m_wake.notify_one();
}
