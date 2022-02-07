#ifndef WORLD_HPP
#define WORLD_HPP

#include "particle.hpp"
#include "rect.hpp"
#include <cassert>

struct Chunk {
    static const size_t SIZE = 64;

    Particle& get(size_t x, size_t y);
    const Particle& get(size_t x, size_t y) const;

    bool is_dirty() const { return !cur_dirty_rect.is_empty(); }

    Particle data[SIZE][SIZE];

    //dirty rect gets updated during chunk processing,
    //and might be larger than the size of chunk;
    //these are absolute coordinates
    Rect<int> cur_dirty_rect, next_dirty_rect;
    Rect<int> needs_redrawing;
};

struct Block {
    static const size_t SIZE = 512;
    static const size_t N = SIZE / Chunk::SIZE;

    Particle& get(size_t x, size_t y);
    const Particle& get(size_t x, size_t y) const;

    Chunk chunks[N][N];
};

inline Rect<int> chunk_bounds(int ch_x, int ch_y) {
    return Rect<int>(
         ch_x * Chunk::SIZE, 
         ch_y * Chunk::SIZE, 
        (ch_x + 1) * Chunk::SIZE - 1,
        (ch_y + 1) * Chunk::SIZE - 1
    );
}

//a wrapper around array of blocks, 
//which will be generated or loaded / unloaded dynamically
class World {
public:
    static const size_t NUM_BLOCKS = 2;

    World();

    bool is_particle_loaded(size_t x, size_t y) const;
    bool is_chunk_loaded(size_t ch_x, size_t ch_y) const;
    bool is_block_loaded(size_t blk_x, size_t blk_y) const;

    Chunk& get_chunk(size_t ch_x, size_t ch_y);
    const Chunk& get_chunk(size_t ch_x, size_t ch_y) const;

    Particle& get(size_t x, size_t y);
    const Particle& get(size_t x, size_t y) const;

    void fit_dirty_rects(bool keep_old);


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
    size_t furthest_occupied(size_t blk_x, size_t blk_y,
            size_t &furthest_x, size_t &furthest_y) const;

    //returns a slot that got freed after applying new bounds (or NUM_BLOCKS if none was freed)
    size_t recalc_slotmap(size_t new_left, size_t new_top);

    //reserves a place for the block and removes the furthest block(s) if needed
    size_t include_block(size_t blk_x, size_t blk_y);

    void fit_block(size_t blk_x, size_t blk_y, Block &blk, bool keep_old);
};

#endif

