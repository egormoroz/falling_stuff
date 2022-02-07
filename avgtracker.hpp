#ifndef AVGTRACKER_HPP
#define AVGTRACKER_HPP

#include <array>

template<typename T, size_t N>
class AvgTracker {
public:
    using const_iterator = typename std::array<T, N>::const_iterator;

    AvgTracker() : m_sum(), m_idx(0) {
        m_data.fill(T());
    }

    const_iterator begin() const { return m_data.cbegin(); }
    const_iterator end() const { return m_data.cend(); }

    void push(const T &x) {
        m_sum += x - m_data[m_idx];
        m_data[m_idx++] = x;
        if (m_idx >= N)
            m_idx = 0;
    }

    template<typename U = T>
    T average() const { 
        return static_cast<U>(m_sum) / static_cast<int64_t>(N); 
    }

    T sum() const { return m_sum; }

    T last() const { 
        return m_data[m_idx ? m_idx - 1 : N - 1]; 
    }

private:
    std::array<T, N> m_data;
    T m_sum;
    size_t m_idx;
};

#endif

