#ifndef WORLD_HPP
#define WORLD_HPP

#include "particle.hpp"
#include <algorithm>
#include <cassert>
#include <limits>

template<typename T>
struct Rect {
    T left, top, right, bottom;

    Rect() = default;
    Rect(T left, T top, T right, T bottom)
        : left(left), top(top), right(right), bottom(bottom) 
    {}

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
};

struct Block {
    static const size_t BLOCK_SIZE = 512;
    static const size_t CHUNK_SIZE = 64;
    static const size_t N = BLOCK_SIZE / CHUNK_SIZE;

    //in relative coords: (0, 0) is the top left point of the block
    Particle& get(size_t rel_x, size_t rel_y) {
        Chunk &ch = chunks[rel_y / CHUNK_SIZE][rel_x / CHUNK_SIZE];
        return ch.data[rel_y % CHUNK_SIZE][rel_x % CHUNK_SIZE];
    }

    //in relative coords: (0, 0) is the top left point of the block
    const Particle& get(size_t rel_x, size_t rel_y) const {
        const Chunk &ch = chunks[rel_y / CHUNK_SIZE][rel_x / CHUNK_SIZE];
        return ch.data[rel_y % CHUNK_SIZE][rel_x % CHUNK_SIZE];
    }

    struct Chunk {
        bool is_dirty() const { return !cur_dirty_rect.is_empty(); }

        Particle data[CHUNK_SIZE][CHUNK_SIZE];
        //dirty rect gets updated during chunk processing,
        //and might be larger than the size of chunk;
        //these are absolute coordinates
        Rect<int> cur_dirty_rect, next_dirty_rect;
        Rect<int> needs_redrawing;
    } chunks[N][N];

    //measured in blocks
    size_t left, top;
};

inline Rect<int> chunk_bounds(int ch_x, int ch_y) {
    return Rect<int>(
         ch_x * Block::CHUNK_SIZE, 
         ch_y * Block::CHUNK_SIZE, 
        (ch_x + 1) * Block::CHUNK_SIZE - 1,
        (ch_y + 1) * Block::CHUNK_SIZE - 1
    );
}

//a wrapper around array of blocks, 
//which will be generated or loaded / unloaded dynamically
class World {
public:
    static const size_t NUM_BLOCKS = 12;

    World();

    bool is_chunk_loaded(size_t ch_x, size_t ch_y) const;

    Block::Chunk& get_chunk(size_t ch_x, size_t ch_y);
    const Block::Chunk& get_chunk(size_t ch_x, size_t ch_y) const;

    Particle& get(size_t x, size_t y);
    const Particle& get(size_t x, size_t y) const;

    //and generate a bitmap of dirty chunks as well?
    //better be done in parallel...
    //fits dirty rects and swaps next and current
    //and also updates needs_redrawing
    //TODO: Rewrite this for multiple blocks
    void fit_dirty_rects();

    //enumerates dirty chunks in checkers fashion
    //MODE: 0 - update mode, 1 - redraw mode
    //TODO: Get rid of this and use enumerate_blocks(...)
    template<int PASS, int MODE = 0, typename F>
    void enumerate(F &f) {
        static_assert(PASS >= 0 && PASS <= 4, "only 4 passes are possible");
        static_assert(MODE == 0 || MODE == 1, "only 2 mods available");

        switch (PASS) {
        case 0:
            for (size_t ch_y = 0; ch_y < Block::N; ++ch_y) {
                for (size_t ch_x = 0; ch_x < Block::N; ++ch_x) {
                    auto &ch = get_chunk(ch_x, ch_y);
                    if (ch.is_dirty() && MODE == 0 || 
                            !ch.needs_redrawing.is_empty() && MODE == 1)
                        f(ch_x, ch_y, ch);
                }
            }
            break;
        case 1:
            for (size_t ch_y = 0; ch_y < Block::N; ch_y += 2) {
                for (size_t ch_x = 0; ch_x < Block::N; ch_x += 2) {
                    auto &ch = get_chunk(ch_x, ch_y);
                    if (ch.is_dirty() && MODE == 0 || 
                            !ch.needs_redrawing.is_empty() && MODE == 1)
                        f(ch_x, ch_y, ch);
                }
            }
            break;
        case 2:
            for (size_t ch_y = 0; ch_y < Block::N; ch_y += 2) {
                for (size_t ch_x = 1; ch_x < Block::N; ch_x += 2) {
                    auto &ch = get_chunk(ch_x, ch_y);
                    if (ch.is_dirty() && MODE == 0 || 
                            !ch.needs_redrawing.is_empty() && MODE == 1)
                        f(ch_x, ch_y, ch);
                }
            }
            break;
        case 3:
            for (size_t ch_y = 1; ch_y < Block::N; ch_y += 2) {
                for (size_t ch_x = 0; ch_x < Block::N; ch_x += 2) {
                    auto &ch = get_chunk(ch_x, ch_y);
                    if (ch.is_dirty() && MODE == 0 || 
                            !ch.needs_redrawing.is_empty() && MODE == 1)
                        f(ch_x, ch_y, ch);
                }
            }
            break;
        case 4:
            for (size_t ch_y = 1; ch_y < Block::N; ch_y += 2) {
                for (size_t ch_x = 1; ch_x < Block::N; ch_x += 2) {
                    auto &ch = get_chunk(ch_x, ch_y);
                    if (ch.is_dirty() && MODE == 0 || 
                            !ch.needs_redrawing.is_empty() && MODE == 1)
                        f(ch_x, ch_y, ch);
                }
            }
            break;
        default:
            //unreachable
            break;
        };
    }

    bool is_block_loaded(size_t blk_x, size_t blk_y) const;

    Block& get_block(size_t blk_x, size_t blk_y);
    const Block& get_block(size_t blk_x, size_t blk_y) const;

    //enumerates all loaded blocks row by row, each from left to right
    template<typename F>
    void enumerate_blocks(F &f) {
        for (size_t j = 0; j < NUM_BLOCKS; ++j) {
            for (size_t i = 0; i < NUM_BLOCKS; ++i) {
                size_t slot = m_slotmap[j][i];
                if (slot == NUM_BLOCKS)
                    continue;
                size_t blk_x = m_left + i, blk_y = m_top + j;
                f(blk_x, blk_y, m_blocks[slot]);
            }
        }
    }

    void load_block(size_t blk_x, size_t blk_y);

private:
    Block m_block;
    size_t m_left, m_top;
    uint8_t m_slotmap[NUM_BLOCKS][NUM_BLOCKS];
    //slots for storing blocks
    //these are supposed to be lying contiguosly in the world
    //(in the worst case - in a straight line with lenghth of NUM_BLOCKS blocks)
    Block m_blocks[NUM_BLOCKS];

    //This method gets called during the update of the slotmap
    //so it'd better not touch anything!
    void unload_block(size_t blk_x, size_t blk_y);

    //--------------Helpers--------------

    bool contains_block(size_t blk_x, size_t blk_y) const;
    size_t get_block_slot(size_t blk_x, size_t blk_y) const;

    size_t find_unoccupied() const;
    bool furthest_occupied(size_t blk_x, size_t blk_y,
            size_t &furthest_x, size_t &furthest_y) const;

    //returns a slot that got freed after applying new bounds (or NUM_BLOCKS if none was freed)
    size_t recalc_slotmap(size_t new_left, size_t new_top);

    //reserves a place for the block and removes the furthest block(s) if needed
    size_t include_block(size_t blk_x, size_t blk_y);
};

#endif

