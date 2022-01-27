#ifndef PERF_TRACKER_HPP
#define PERF_TRACKER_HPP

const int MAX_FRAME_TIMES = 64;

class FpsTracker {
public:
    FpsTracker();

    void push(float frame_time);

    float avg_frame_time() const;
    float avg_fps() const;
private:
    float m_frame_times[MAX_FRAME_TIMES];
    float m_sum;
    size_t m_idx;
};

#endif