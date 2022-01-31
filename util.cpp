#include "util.hpp"
#include <algorithm>
#include <numeric>
#include <cassert>

const int MIN_INT = std::numeric_limits<int>::min();
const int MAX_INT = std::numeric_limits<int>::max();

Rect::Rect()
    : Rect(0, 0, 0, 0) {}

Rect::Rect(int left, int top, int right, int bottom)
    : left(left), top(top), right(right), bottom(bottom) {}

void Rect::include(int x, int y, int r) {
    left = std::min(left, x - r);
    top = std::min(top, y - r);
    right = std::max(right, x + r);
    bottom = std::max(bottom, y + r);
}

void Rect::include(const Rect &other) {
    left = std::min(left, other.left);
    top = std::min(top, other.top);
    right = std::max(right, other.right);
    bottom = std::max(bottom, other.bottom);
}

void Rect::extend_by(int r) {
    left -= r;
    top -= r;
    right += r;
    bottom += r;
}

void Rect::fit(int max_width, int max_height) {
    left = std::max(left, 0);
    top = std::max(top, 0);
    right = std::min(right, max_width - 1);
    bottom = std::min(bottom, max_height - 1);
}

void Rect::clear() {
    left = MAX_INT;
    top = MAX_INT;
    right = MIN_INT;
    bottom = MIN_INT;
}

bool Rect::is_empty() const {
    return right < left || bottom < top;
}

bool Rect::contains(int x, int y) const {
    return x >= left && y >= top && x <= right && y <= bottom;
}

bool Rect::contains(const Rect &other) const {
    return other.left >= left && other.top >= top 
        && other.right <= right && other.bottom <= bottom;
}

bool Rect::intersects(const Rect &other) const {
    return other.right >= left && other.bottom >= top
        && other.left <= right && other.top <= bottom;
}

Rect Rect::intersection(const Rect &other) const {
    assert(intersects(other));
    return Rect(
        std::max(left, other.left),
        std::max(top, other.top),
        std::min(right, other.right),
        std::min(bottom, other.bottom)
    );
}

int Rect::width() const { return right - left + 1; }
int Rect::height() const { return bottom - top + 1; }

