#ifndef UTIL_HPP
#define UTIL_HPP

struct Rect {
    int left, top, right, bottom;

    Rect();
    Rect(int left, int top, int right, int bottom);
    Rect(int max_width, int max_height);

    void include(int x, int y, int r = 0);
    void include(const Rect &other);
    void extend_by(int r);

    void fit(int max_width, int max_height);
    void reset(int max_width, int max_height);
    void clear(int max_width, int max_height);

    bool is_empty() const;
    bool contains(int x, int y) const;
    bool intersects(const Rect &other) const;
    Rect intersection(const Rect &other) const;
};

#endif

