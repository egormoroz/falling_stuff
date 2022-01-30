#include "util.hpp"
#include <algorithm>

Rect::Rect()
    : Rect(0, 0, 0, 0) {}

Rect::Rect(int left, int top, int right, int bottom)
    : left(left), top(top), right(right), bottom(bottom) {}

Rect::Rect(int max_width, int max_height) {
    reset(max_width, max_height);
}

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

void Rect::reset(int max_width, int max_height) {
    left = 0;
    top = 0;
    right = max_width - 1;
    bottom = max_height - 1;
}

void Rect::clear(int max_width, int max_height) {
    left = max_width - 1;
    top = max_height - 1;
    right = -1;
    bottom = -1;
}

bool Rect::is_empty() const {
    return right < left || bottom < top;
}


bool Rect::contains(int x, int y) const {
    return x >= left && y >= top && x <= right && y <= bottom;
}

bool Rect::intersects(const Rect &other) const {
    return other.right >= left && other.bottom >= top
        && other.left <= right && other.top <= bottom;
}

Rect Rect::intersection(const Rect &other) const {
    return Rect(
        std::max(left, other.left),
        std::max(top, other.top),
        std::min(right, other.right),
        std::min(bottom, other.bottom)
    );
}

