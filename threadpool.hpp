#include <vector>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>


template<typename T>
class Queue {
public:
    Queue() = default;

    void push(const T &t) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_data.push(t);
    }

    void push(T && t) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_data.push(std::move(t));
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mtx);
        while (!m_data.empty())
            m_data.pop();
    }

    bool try_pop(T & t) {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_data.empty())
            return false;
        t = std::move(m_data.front());
        m_data.pop();
        return true;
    }

private:
    std::queue<T> m_data;
    std::mutex m_mtx;
};

class ThreadPool {
public:
    using Task = std::function<void(size_t)>;

    ThreadPool(size_t n_threads = std::thread::hardware_concurrency()) 
        : m_working(true), m_num_tasks(0) 
    {
        for (size_t i = 0; i < n_threads; ++i) {
            m_workers.emplace_back(std::bind(
                &ThreadPool::worker_routine, this, i));
        }
    }

    void launch_and_wait() {
        m_cv.notify_all();
        while (m_num_tasks)
            std::this_thread::yield();
    }

    void push_task(Task && task) {
        m_tasks.push(std::move(task));
        ++m_num_tasks;
    }

    ~ThreadPool() {
        stop();
    }

private:
    std::vector<std::thread> m_workers;
    Queue<Task> m_tasks;

    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::atomic<bool> m_working;
    std::atomic<size_t> m_num_tasks;

    void stop() {
        m_working = false;
        m_num_tasks = 0;
        m_tasks.clear();
        m_cv.notify_all();
        for (auto &w: m_workers)
            if (w.joinable())
                w.join();
    }

    void worker_routine(size_t worker_idx) {
        Task task;
        bool have_task = false;
        while (true) {
            while (have_task) {
                task(worker_idx);
                --m_num_tasks;
                if (!m_working)
                    return;

                have_task = m_tasks.try_pop(task);
            }

            std::unique_lock<std::mutex> lock(m_mtx);
            m_cv.wait(lock, [this, &have_task, &task] {
                have_task = m_tasks.try_pop(task);
                return have_task || !m_working;
            });

            if (!have_task)
                return;
        }
    }
};

