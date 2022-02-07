#include "updatescheduler.hpp"
#include <functional>
#include <numeric>
#include "simulation.hpp"

namespace scheduler {
namespace detail {

Queue::Queue() 
    : m_pointer(0) {}

void Queue::push(const ChunkForUpdating &cfu) {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_data.push_back(cfu);
}

bool Queue::pop(ChunkForUpdating &cfu) {
    std::lock_guard<std::mutex> lock(m_mtx);
    if (m_pointer >= m_data.size())
        return false;
    cfu = m_data[m_pointer++];
    return true;
}

void Queue::reset() {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_pointer = 0;
}

void Queue::clear() {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_pointer = 0;
    m_data.clear();
}

} //detail

inline size_t group_idx(size_t ch_x, size_t ch_y) {
    return 2 * (ch_y % 2) + ch_x % 2;
}

UpdateScheduler::UpdateScheduler(Simulation &sim, size_t num_threads) : m_sim(sim) {
    m_active_group = 0;
    m_status = detail::Paused;
    m_busy = num_threads;

    m_threads.reserve(num_threads);
    m_load_balance.resize(num_threads + 1);
    m_stats.resize(num_threads + 1, 0);
    for (size_t i = 0; i < num_threads; ++i) {
        m_threads.emplace_back(std::bind(
                    &UpdateScheduler::thread_routine, this, i));
    }
}


void UpdateScheduler::push_chunk(size_t ch_x, size_t ch_y, Chunk *ch) {
    detail::ChunkForUpdating cfu{ch_x, ch_y, ch};
    size_t idx = group_idx(ch_x, ch_y);
    m_groups[idx].push(cfu);
    if (idx == m_active_group)
        m_cv.notify_one();
}

void UpdateScheduler::clear() {
    for (auto &g: m_groups)
        g.clear();
}

void UpdateScheduler::run(Mode mode) {
    std::fill(m_stats.begin(), m_stats.end(), 0);

    for (size_t i = 0; i < 4; ++i) {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_groups[i].reset();
        m_active_group = i;
        m_mode = mode;
        lock.unlock();

        m_status = detail::Working;
        m_cv.notify_all();

        process_chunks(m_threads.size());
        m_status = detail::Paused;

        lock.lock();
        m_done.wait(lock, [this]() { return m_busy == 0; });
    }

    if (mode == Update) {
        uint64_t n = std::max(1, std::accumulate(m_stats.begin(), m_stats.end(), 0));
        for (size_t i = 0; i < m_threads.size() + 1; ++i)
            m_load_balance[i].push(static_cast<float>(m_stats[i]) / n);
    }
}

const std::vector<LoadTracker>& UpdateScheduler::load_balance() const {
    return m_load_balance;
}

UpdateScheduler::~UpdateScheduler() {
    m_status = detail::Stopped;
    m_cv.notify_all();
    for (auto &t: m_threads)
        t.join();
}

void UpdateScheduler::thread_routine(size_t worker_idx) {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            --m_busy;
            m_done.notify_one();
            m_cv.wait(lock, [this]() { return m_status != detail::Paused; });
            ++m_busy;
        }

        if (m_status == detail::Stopped)
            break;

        process_chunks(worker_idx);
    }
}

void UpdateScheduler::process_chunks(size_t worker_idx) {
    switch (m_mode) {
    case Prepare:
        prepare(worker_idx);
        break;
    case Update:
        update(worker_idx);
        break;
    case Render:
        render(worker_idx);
        break;
    default:
        //unreachable
        break;
    };
}

void UpdateScheduler::prepare(size_t worker_idx) {
    detail::ChunkForUpdating cfu;
    while (m_groups[m_active_group].pop(cfu)) {
        m_sim.prepare_chunk(cfu.ch_x, cfu.ch_y, *cfu.ch, worker_idx);
    }
}

void UpdateScheduler::update(size_t worker_idx) {
    detail::ChunkForUpdating cfu;
    while (m_groups[m_active_group].pop(cfu)) {
        m_sim.update_chunk(cfu.ch_x, cfu.ch_y, *cfu.ch, worker_idx);
        ++m_stats[worker_idx];
    }
}

void UpdateScheduler::render(size_t worker_idx) {
    detail::ChunkForUpdating cfu;
    while (m_groups[m_active_group].pop(cfu))
        m_sim.render_chunk(cfu.ch_x, cfu.ch_y, *cfu.ch, worker_idx);
}


} //scheduler

