#ifndef RECT_HPP
#define RECT_HPP

#include <algorithm>
#include <limits>
#include <cassert>

template<typename T>
struct Rect {
    T left, top, right, bottom;

    Rect() = default;
    Rect(T left, T top, T right, T bottom)
        : left(left), top(top), right(right), bottom(bottom) 
    {}

    template<typename U>
    explicit Rect(const Rect<U> &other) {
        left = static_cast<T>(other.left);
        top = static_cast<T>(other.top);
        right = static_cast<T>(other.right);
        bottom = static_cast<T>(other.bottom);
    }

    template<bool with_neighbours>
    void include(int x, int y) {
        if (with_neighbours) {
            include(Rect(x - 1, y - 1, x + 1, y + 1));
            return;
        }

        left = std::min(left, x);
        top = std::min(top, y);
        right = std::max(right, x);
        bottom = std::max(bottom, y);
    }

    void include(const Rect &r) {
        left = std::min(left, r.left);
        top = std::min(top, r.top);
        right = std::max(right, r.right);
        bottom = std::max(bottom, r.bottom);
    }

    bool contains(T x, T y) const {
        return x >= left && x <= right
            && y >= top && y <= bottom;
    }

    bool intersects(const Rect &r) const {
        return r.right >= left && r.bottom >= top
            && r.left <= right && r.top <= bottom;
    }

    Rect intersection(const Rect &other) const {
        assert(intersects(other));
        return Rect(
            std::max(left, other.left),
            std::max(top, other.top),
            std::min(right, other.right),
            std::min(bottom, other.bottom)
        );
    }

    void reset() {
        const T MIN_VAL = std::numeric_limits<T>::min();
        const T MAX_VAL = std::numeric_limits<T>::max();
        left = MAX_VAL;
        top = MAX_VAL;
        right = MIN_VAL;
        bottom = MIN_VAL;
    }

    bool is_empty() const {
        return left > right || top > bottom;
    }

    bool operator==(const Rect &other) const {
        return left == other.left && top == other.top
            && right == other.right && bottom == other.bottom;
    }

    //doens't perform any checks
    T width() const { return right - left + 1; }
    //doens't perform any checks
    T height() const { return bottom - top + 1; }

    //doens't perform any checks
    T area() const {
        return width() * height();
    }

    T shared_area(const Rect &other) const {
        if (!intersects(other))
            return 0;
        return intersection(other).area();
    }
};

#endif

