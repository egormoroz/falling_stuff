#include "world.hpp"
#include <memory>
#include <cassert>
#include <type_traits>

World::World() {
    const size_t INITIAL_WIDTH = 4;
    const size_t INITIAL_HEIGHT = 3;
    static_assert(NUM_BLOCKS == INITIAL_WIDTH * INITIAL_HEIGHT);

    m_left = 0;
    m_top = 0;
    for (auto &i: m_slotmap)
        for (auto &j: i)
            j = NUM_BLOCKS;

    memset(m_blocks, 0, sizeof(m_blocks));
    for (size_t j = 0; j < INITIAL_HEIGHT; ++j) {
        for (size_t i = 0; i < INITIAL_WIDTH; ++i) {
            size_t slot = j * INITIAL_WIDTH + i;
            m_slotmap[j][i] = slot;

            Block &blk = m_blocks[slot];
            blk.left = i;
            blk.top = j;
            for (auto &i: blk.chunks) {
                for (auto &j: i) {
                    j.cur_dirty_rect.reset();
                    j.next_dirty_rect.reset();
                }
            }
        }
    }

    /* m_block.left = 0; */
    /* m_block.top = 0; */

    /* memset(&m_block, 0, sizeof(m_block)); */
    /* for (auto &i: m_block.chunks) */
    /*     for (auto &j: i) { */
    /*         j.cur_dirty_rect.reset(); */
    /*         j.next_dirty_rect.reset(); */
    /*     } */
}

bool World::is_chunk_loaded(size_t ch_x, size_t ch_y) const {
    return is_block_loaded(ch_x / Block::N, ch_y / Block::N);
    /* return ch_x < Block::N && ch_y < Block::N; */
}

Block::Chunk& World::get_chunk(size_t ch_x, size_t ch_y) {
    Block &blk = get_block(ch_x / Block::N, ch_y / Block::N);
    return blk.chunks[ch_y % Block::N][ch_x % Block::N];
    /* return m_block.chunks[ch_y][ch_x]; */
}

const Block::Chunk& World::get_chunk(size_t ch_x, size_t ch_y) const {
    const Block &blk = get_block(ch_x / Block::N, ch_y / Block::N);
    return blk.chunks[ch_y % Block::N][ch_x % Block::N];
    /* return m_block.chunks[ch_y][ch_x]; */
}

Particle& World::get(size_t x, size_t y) {
    Block &blk = get_block(x / Block::BLOCK_SIZE, y / Block::BLOCK_SIZE);
    return blk.get(x % Block::BLOCK_SIZE, y % Block::BLOCK_SIZE);
    /* return m_block.get(x, y); */
}

const Particle& World::get(size_t x, size_t y) const {
    const Block &blk = get_block(x / Block::BLOCK_SIZE, y / Block::BLOCK_SIZE);
    return blk.get(x % Block::BLOCK_SIZE, y % Block::BLOCK_SIZE);
    /* return m_block.get(x, y); */
}

//TODO: Rewrite this for multiple blocks
void World::fit_dirty_rects() {
    Rect<int> r;
    //Now that's a lot of branching!
    for (size_t ch_y = 0; ch_y < Block::N; ++ch_y) {
        for (size_t ch_x = 0; ch_x < Block::N; ++ch_x) {
            auto &ch = get_chunk(ch_x, ch_y);
            if (ch.next_dirty_rect.is_empty())
                continue;

            auto &bounds = ch.next_dirty_rect;
            if (ch_x) {
                //Left
                r = chunk_bounds(ch_x - 1, ch_y);
                if (bounds.intersects(r))
                    get_chunk(ch_x - 1, ch_y).next_dirty_rect.include(bounds.intersection(r));

                //Top left
                if (ch_y) {
                    r = chunk_bounds(ch_x - 1, ch_y - 1);
                    if (bounds.intersects(r))
                        get_chunk(ch_x - 1, ch_y - 1).next_dirty_rect.include(bounds.intersection(r));
                }

                //Bottom left
                if (is_chunk_loaded(ch_x - 1, ch_y + 1)) {
                    r = chunk_bounds(ch_x - 1, ch_y + 1);
                    if (bounds.intersects(r))
                        get_chunk(ch_x - 1, ch_y + 1).next_dirty_rect.include(bounds.intersection(r));
                }
            }

            if (is_chunk_loaded(ch_x + 1, ch_y)) {
                //Right
                r = chunk_bounds(ch_x + 1, ch_y);
                if (bounds.intersects(r))
                    get_chunk(ch_x + 1, ch_y).next_dirty_rect.include(bounds.intersection(r));

                //Top right
                if (ch_y) {
                    r = chunk_bounds(ch_x + 1, ch_y - 1);
                    if (bounds.intersects(r))
                        get_chunk(ch_x + 1, ch_y - 1).next_dirty_rect.include(bounds.intersection(r));
                }

                //Bottom right
                if (is_chunk_loaded(ch_x + 1, ch_y + 1)) {
                    r = chunk_bounds(ch_x + 1, ch_y + 1);
                    if (bounds.intersects(r))
                        get_chunk(ch_x + 1, ch_y + 1).next_dirty_rect.include(bounds.intersection(r));
                }
            }

            //Top
            if (ch_y) {
                r = chunk_bounds(ch_x, ch_y - 1);
                if (bounds.intersects(r))
                    get_chunk(ch_x, ch_y - 1).next_dirty_rect.include(bounds.intersection(r));
            }

            //Bottom
            if (is_chunk_loaded(ch_x, ch_y + 1)) {
                r = chunk_bounds(ch_x, ch_y + 1);
                if (bounds.intersects(r))
                    get_chunk(ch_x, ch_y + 1).next_dirty_rect.include(bounds.intersection(r));
            }

            ch.next_dirty_rect = ch.next_dirty_rect.intersection(chunk_bounds(ch_x, ch_y));
        }
    }

    for (size_t j = 0; j < Block::N; ++j) {
        for (size_t i = 0; i < Block::N; ++i) {
            auto &ch = m_block.chunks[j][i];
            ch.cur_dirty_rect = ch.next_dirty_rect;
            ch.next_dirty_rect.reset();

            if (!ch.cur_dirty_rect.is_empty())
                ch.needs_redrawing.include(ch.cur_dirty_rect);
        }
    }
}

bool World::is_block_loaded(size_t blk_x, size_t blk_y) const {
    return contains_block(blk_x, blk_y) 
        && m_slotmap[blk_y - m_top][blk_x - m_left] != NUM_BLOCKS;
}

Block& World::get_block(size_t blk_x, size_t blk_y) {
    assert(is_block_loaded(blk_x, blk_y));
    return m_blocks[m_slotmap[blk_y - m_top][blk_x - m_left]];
}

const Block& World::get_block(size_t blk_x, size_t blk_y) const {
    assert(is_block_loaded(blk_x, blk_y));
    return m_blocks[m_slotmap[blk_y - m_top][blk_x - m_left]];
}

void World::load_block(size_t blk_x, size_t blk_y) {
    size_t slot = include_block(blk_x, blk_y);
    Block &blk = m_blocks[slot];
    //just reset it
    memset(&blk, 0, sizeof(blk));
    blk.left = blk_x;
    blk.top = blk_y;
    for (auto &i: blk.chunks) {
        for (auto &j: i) {
            j.cur_dirty_rect.reset();
            j.next_dirty_rect.reset();
        }
    }
}


void World::unload_block(size_t blk_x, size_t blk_y) {
    //maybe write it to the disk??
}

bool World::contains_block(size_t blk_x, size_t blk_y) const {
    return blk_x < m_left || blk_x >= m_left + NUM_BLOCKS
            || blk_y < m_top || blk_y >= m_top + NUM_BLOCKS;
}

size_t World::get_block_slot(size_t blk_x, size_t blk_y) const {
    assert(contains_block(blk_x, blk_y));
    return m_slotmap[blk_y - m_top][blk_x - m_left];
}

size_t World::recalc_slotmap(size_t new_left, size_t new_top) {
    size_t unoccupied = NUM_BLOCKS;
    uint8_t new_offsets[NUM_BLOCKS][NUM_BLOCKS];
    for (size_t j = 0; j < NUM_BLOCKS; ++j) {
        for (size_t i = 0; i < NUM_BLOCKS; ++i) {
            size_t blk_x = new_left + i, blk_y = new_top + j;
            if (contains_block(blk_x, blk_y)) {
                //keep and remap the block
                new_offsets[j][i] = get_block_slot(blk_x, blk_y);
            } else {
                if (m_slotmap[j][i] != NUM_BLOCKS) {
                    unload_block(blk_x, blk_y);
                    unoccupied = m_slotmap[j][i];
                }
                new_offsets[j][i] = NUM_BLOCKS;
            }
        }
    }
    m_left = new_left;
    m_top = new_top;
    memcpy(m_slotmap, new_offsets, sizeof(new_offsets));
    return unoccupied;
}


//saturated substraction
template<typename T>
T sat_sub(T x, T y) {
    static_assert(std::is_unsigned<T>::value, "must be unsigned");
    T res = x - y;
    res &= -(res <= x);
    return res;
}

size_t World::find_unoccupied() const {
    bool b[NUM_BLOCKS + 1]{};
    for (size_t j = 0; j < NUM_BLOCKS; ++j)
        for (size_t i = 0; i < NUM_BLOCKS; ++i)
            b[m_slotmap[j][i]] = true;
    return std::find(std::begin(b), std::end(b) - 1, false) - std::begin(b);
}

bool World::furthest_occupied(size_t blk_x, size_t blk_y,
            size_t &furthest_x, size_t &furthest_y) const 
{
    assert(contains_block(blk_x, blk_y));
    int x = blk_x - m_left, y = blk_y - m_top;
    int max_d = 0;
    for (int j = 0; j < NUM_BLOCKS; ++j) {
        for (int i = 0; i < NUM_BLOCKS; ++i) {
            int d = abs(x + y - i - j);
            if (m_slotmap[j][i] != NUM_BLOCKS && d > max_d) {
                max_d = d;
                furthest_x = m_left + static_cast<size_t>(x);
                furthest_y = m_top + static_cast<size_t>(y);
            }
        }
    }

    return max_d != 0;
}

Rect<size_t> make_bounds(size_t left, size_t top) {
    return { left, top, left + World::NUM_BLOCKS - 1, 
        top + World::NUM_BLOCKS - 1};
}

size_t World::include_block(size_t blk_x, size_t blk_y) {
    if (contains_block(blk_x, blk_y)) {
        size_t available = find_unoccupied();
        if (available != NUM_BLOCKS) {
            m_slotmap[blk_y - m_top][blk_x - m_left] = available;
            return available;
        }

        size_t furthest_x = 0, furthest_y = 0;
        assert(furthest_occupied(blk_x, blk_y, furthest_x, furthest_y));

        uint8_t &furthest = m_slotmap[furthest_y - m_top][furthest_x - m_left];
        m_slotmap[blk_y - m_top][blk_x - m_left] = furthest;
        furthest = NUM_BLOCKS;
        return furthest;
    }

    Rect<size_t> new_bounds[4] = {
        //top left
        make_bounds(sat_sub(blk_x, NUM_BLOCKS - 1), sat_sub(blk_y, NUM_BLOCKS - 1)),
        //top right
        make_bounds(blk_x, sat_sub(blk_y, NUM_BLOCKS - 1)),
        //bottom left
        make_bounds(sat_sub(blk_x, NUM_BLOCKS - 1), blk_y),
        //bottom right
        make_bounds(blk_x, blk_y)
    };
    Rect<size_t> old_bounds = make_bounds(m_left, m_top);
    auto pred = [&old_bounds](auto &lhs, auto &rhs) { 
        return old_bounds.intersection(lhs).area()
            < old_bounds.intersection(rhs).area();
    };
    Rect<size_t> &best = *std::max_element(
        std::begin(new_bounds), std::end(new_bounds), pred);

    size_t unoccupied = recalc_slotmap(best.left, best.top);
    if (unoccupied != NUM_BLOCKS) {
        //at least one block got removed lmao get hecking recked
        m_slotmap[blk_y - m_top][blk_x - m_left] = unoccupied;
        return unoccupied;
    } else {
        //no blocks were removed...
        //this is only possible if there had been no blocks in the first place
        size_t _tmp_x, _tmp_y;
        assert(!furthest_occupied(blk_x, blk_y, _tmp_x, _tmp_y));
        m_slotmap[blk_y - m_top][blk_x - m_left] = 0;
        return 0;
    }
}

