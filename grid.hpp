#ifndef GRID_HPP
#define GRID_HPP

#include <vector>
#include <cassert>

template<typename T>
class Grid {
public:
    Grid(int width, int height, const T &def_val = T())
        : m_width(width), m_height(height),
          m_data(width * height, def_val) {}

    bool contains(int x, int y) const {
        return x >= 0 && y >= 0 && x < m_width && y < m_height;
    }

    auto begin() { return m_data.begin(); }
    auto begin() const { return m_data.begin(); }
    auto end() { return m_data.end(); }
    auto end() const { return m_data.end(); }

    int width() const { return m_width; }
    int height() const { return m_height; }

    T& get(int x, int y) { return m_data[xy_idx(x, y)]; }
    const T& get(int x, int y) const { return m_data[xy_idx(x, y)]; }

    void swap(int x, int y, int xx, int yy) {
        std::swap(m_data[xy_idx(x, y)], m_data[xy_idx(xx, yy)]);
    }

private:
    std::vector<T> m_data;
    int m_width, m_height;

    size_t xy_idx(int x, int y) const {
        assert(x >= 0 && x < m_width && y >= 0 && y < m_height);
        return y * m_width + x;
    }
};

#endif
