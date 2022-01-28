#include "perf_tracker.hpp"
#include <memory>
#include <algorithm>

FpsTracker::FpsTracker() {
    m_sum = 0.f;
    m_idx = 0;
    memset(m_frame_times, 0, sizeof(m_frame_times));
}

float FpsTracker::recorded_period() const {
    return m_sum;
}

float FpsTracker::avg_frame_time() const {
    return m_sum / MAX_FRAME_TIMES;
}

float FpsTracker::avg_fps() const {
    return 1000.f / avg_frame_time();
}

float FpsTracker::longest() const {
    return *std::max_element(std::begin(m_frame_times), 
        std::end(m_frame_times));
}

void FpsTracker::push(float frame_time) {
    m_sum += frame_time - m_frame_times[m_idx];
    m_frame_times[m_idx] = frame_time;
    if (++m_idx >= MAX_FRAME_TIMES)
        m_idx = 0;
}

