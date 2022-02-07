#ifndef UPDATESCHEDULER_HPP
#define UPDATESCHEDULER_HPP

#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include "avgtracker.hpp"

struct Chunk;
class Simulation;

namespace scheduler {

namespace detail {

struct ChunkForUpdating {
    size_t ch_x, ch_y;
    Chunk *ch;
};

class Queue {
public:
    Queue();

    void push(const ChunkForUpdating &cfu);
    bool pop(ChunkForUpdating &cfu);

    void reset();
    void clear();

private:
    std::vector<ChunkForUpdating> m_data;
    size_t m_pointer;
    std::mutex m_mtx;
};


enum State {
    Working,
    Paused,
    Stopped,
};

} //detail

enum Mode {
    Prepare,
    Update,
    Render
};

using LoadTracker = AvgTracker<float, 64>;

class UpdateScheduler {
public:

    UpdateScheduler(Simulation &sim,
            size_t num_threads = std::thread::hardware_concurrency() - 1);

    void push_chunk(size_t ch_x, size_t ch_y, Chunk *ch);
    void clear();

    void run(Mode mode);

    const std::vector<LoadTracker>& load_balance() const;

    ~UpdateScheduler();

private:
    void thread_routine(size_t worker_idx);

    void process_chunks(size_t worker_idx);

    void prepare(size_t worker_idx);
    void update(size_t worker_idx);
    void render(size_t worker_idx);

    Simulation &m_sim;
    //each queue contains a group of independent chunks that can be updated in parallel
    detail::Queue m_groups[4];
    size_t m_active_group;
    Mode m_mode;

    std::vector<std::thread> m_threads;
    std::mutex m_mtx;
    std::condition_variable m_cv, m_done;
    std::atomic<detail::State> m_status;
    int m_busy;

    std::vector<uint64_t> m_stats;
    std::vector<LoadTracker> m_load_balance;
};

} //scheduler

#endif

