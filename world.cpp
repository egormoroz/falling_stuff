#include "world.hpp"
#include <memory>
#include <cassert>
#include <type_traits>


Particle& Chunk::get(size_t x, size_t y) {
    return data[y % SIZE][x % SIZE];
}

const Particle& Chunk::get(size_t x, size_t y) const {
    return data[y % SIZE][x % SIZE];
}

//make slotmap bounds
Rect<size_t> mksl_bounds(size_t left, size_t top) {
    return { left, top, left + World::NUM_BLOCKS - 1, 
        top + World::NUM_BLOCKS - 1};
}


//saturated substraction
template<typename T>
T sat_sub(T x, T y) {
    static_assert(std::is_unsigned<T>::value, "must be unsigned");
    T res = x - y;
    res &= -(res <= x);
    return res;
}

constexpr size_t my_sqrt(size_t n) {
    if (!n)
        return 0;
    size_t x = 1, xx = 1;
    do {
        x = xx;
        xx = (x * x + n) / (2 * x);
    } while (x != xx);
    return x;
}

const size_t INITIAL_HEIGHT = my_sqrt(World::NUM_BLOCKS);
const size_t INITIAL_WIDTH = World::NUM_BLOCKS / INITIAL_HEIGHT;
const Rect<int> WORLD_BOUNDS(0, 0, INITIAL_WIDTH * Block::SIZE - 1, 
        INITIAL_HEIGHT * Block::SIZE - 1);

World::World() {
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
            m_slotmap[j][i] = static_cast<uint8_t>(slot);

            Block &blk = m_blocks[slot];
            /* blk.left = i; */
            /* blk.top = j; */
            for (auto &i: blk.chunks) {
                for (auto &j: i) {
                    j.cur_dirty_rect.reset();
                    j.next_dirty_rect.reset();
                    j.needs_redrawing.reset();
                }
            }
        }
    }
}

bool World::is_particle_loaded(size_t x, size_t y) const {
    return is_block_loaded(x / Block::SIZE, y / Block::SIZE);
}

bool World::is_chunk_loaded(size_t ch_x, size_t ch_y) const {
    return is_block_loaded(ch_x / Block::N, ch_y / Block::N);
}

Chunk& World::get_chunk(size_t ch_x, size_t ch_y) {
    Block &blk = get_block(ch_x / Block::N, ch_y / Block::N);
    return blk.chunks[ch_y % Block::N][ch_x % Block::N];
}

const Chunk& World::get_chunk(size_t ch_x, size_t ch_y) const {
    const Block &blk = get_block(ch_x / Block::N, ch_y / Block::N);
    return blk.chunks[ch_y % Block::N][ch_x % Block::N];
}

Particle& World::get(size_t x, size_t y) {
    return get_chunk(x / Chunk::SIZE, y / Chunk::SIZE).get(x, y);
}

const Particle& World::get(size_t x, size_t y) const {
    return get_chunk(x / Chunk::SIZE, y / Chunk::SIZE).get(x, y);
}

void World::fit_dirty_rects(bool keep_old) {
    auto f = [this, keep_old](size_t i, size_t j, Block &blk) { fit_block(i, j, blk, keep_old); };
    enumerate_blocks(f);
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
    /* blk.left = blk_x; */
    /* blk.top = blk_y; */
    for (auto &i: blk.chunks) {
        for (auto &j: i) {
            j.cur_dirty_rect.reset();
            j.next_dirty_rect.reset();
            j.needs_redrawing.reset();
        }
    }
}


void World::unload_block(size_t blk_x, size_t blk_y) {
    //maybe write it to the disk??
}

bool World::contains_block(size_t blk_x, size_t blk_y) const {
    return blk_x >= m_left && blk_x < m_left + NUM_BLOCKS
            && blk_y >= m_top && blk_y < m_top + NUM_BLOCKS;
}

size_t World::get_block_slot(size_t blk_x, size_t blk_y) const {
    assert(contains_block(blk_x, blk_y));
    return m_slotmap[blk_y - m_top][blk_x - m_left];
}

size_t World::find_unoccupied() const {
    bool b[NUM_BLOCKS + 1]{};
    for (size_t j = 0; j < NUM_BLOCKS; ++j)
        for (size_t i = 0; i < NUM_BLOCKS; ++i)
            b[m_slotmap[j][i]] = true;
    return std::find(std::begin(b), std::end(b) - 1, false) - std::begin(b);
}

size_t World::recalc_slotmap(size_t new_left, size_t new_top) {
    size_t unoccupied = NUM_BLOCKS;
    Rect<size_t> new_bounds = mksl_bounds(new_left, new_top);
    uint8_t new_slotmap[NUM_BLOCKS][NUM_BLOCKS];
    memset(new_slotmap, NUM_BLOCKS, sizeof(new_slotmap));

    for (size_t j = 0; j < NUM_BLOCKS; ++j) {
        for (size_t i = 0; i < NUM_BLOCKS; ++i) {
            size_t blk_x = m_left + i, blk_y = m_top + j;
            if (new_bounds.contains(blk_x, blk_y)) {
                //keep and remap the block
                new_slotmap[blk_y - new_top][blk_x - new_left] = m_slotmap[j][i];
            } else if (m_slotmap[j][i] != NUM_BLOCKS) {
                unoccupied = m_slotmap[j][i];
                unload_block(blk_x, blk_y);
            }
        }
    }

    m_left = new_left;
    m_top = new_top;
    memcpy(m_slotmap, new_slotmap, sizeof(new_slotmap));

    return unoccupied;
}


size_t World::furthest_occupied(size_t blk_x, size_t blk_y,
            size_t &furthest_x, size_t &furthest_y) const 
{
    assert(contains_block(blk_x, blk_y));
    int x = static_cast<int>(blk_x - m_left), y = static_cast<int>(blk_y - m_top),
        max_d = 0;
    size_t slot = NUM_BLOCKS;
    for (int j = 0; j < NUM_BLOCKS; ++j) {
        for (int i = 0; i < NUM_BLOCKS; ++i) {
            int d = abs(x - i) + abs(y - j);
            if (m_slotmap[j][i] != NUM_BLOCKS && d > max_d) {
                max_d = d;
                furthest_x = m_left + static_cast<size_t>(i);
                furthest_y = m_top + static_cast<size_t>(j);
                slot = m_slotmap[j][i];
            }
        }
    }

    return slot;
}


size_t World::include_block(size_t blk_x, size_t blk_y) {
    if (contains_block(blk_x, blk_y)) {
        size_t available = find_unoccupied();
        if (available != NUM_BLOCKS) {
            m_slotmap[blk_y - m_top][blk_x - m_left] = static_cast<uint8_t>(available);
            return available;
        }

        size_t furthest_x = 0, furthest_y = 0;
        assert(furthest_occupied(blk_x, blk_y, furthest_x, furthest_y) != NUM_BLOCKS);

        uint8_t furthest = m_slotmap[furthest_y - m_top][furthest_x - m_left];
        m_slotmap[blk_y - m_top][blk_x - m_left] = furthest;
        m_slotmap[furthest_y - m_top][furthest_x - m_left] = NUM_BLOCKS;
        return furthest;
    }

    Rect<size_t> new_bounds[4] = {
        //top left
        mksl_bounds(sat_sub(blk_x, NUM_BLOCKS - 1), sat_sub(blk_y, NUM_BLOCKS - 1)),
        //top right
        mksl_bounds(blk_x, sat_sub(blk_y, NUM_BLOCKS - 1)),
        //bottom left
        mksl_bounds(sat_sub(blk_x, NUM_BLOCKS - 1), blk_y),
        //bottom right
        mksl_bounds(blk_x, blk_y)
    };
    Rect<size_t> old_bounds = mksl_bounds(m_left, m_top),
        new_combined(new_bounds[0].left, new_bounds[0].top,
                new_bounds[3].right, new_bounds[3].bottom);
    auto pred = [&old_bounds](auto &lhs, auto &rhs) { 
        return old_bounds.shared_area(lhs) < old_bounds.shared_area(rhs);
    };

    Rect<size_t> &best = *std::max_element(
            std::begin(new_bounds), std::end(new_bounds), pred);

    size_t unoccupied = recalc_slotmap(best.left, best.top);
    if (unoccupied != NUM_BLOCKS) {
        //at least one block got removed lmao get hecking recked
        m_slotmap[blk_y - m_top][blk_x - m_left] = static_cast<uint8_t>(unoccupied);
        return unoccupied;
    } 

    unoccupied = find_unoccupied();
    if (unoccupied != NUM_BLOCKS) {
        m_slotmap[blk_y - m_top][blk_x - m_left] = unoccupied;
        return unoccupied;
    }

    size_t freed_x, freed_y;
    unoccupied = furthest_occupied(blk_x, blk_y, freed_x, freed_y);
    assert(unoccupied != NUM_BLOCKS);
    unload_block(freed_x, freed_y);
    m_slotmap[freed_y - m_top][freed_x - m_left] = NUM_BLOCKS;
    m_slotmap[blk_y - m_top][blk_x - m_left] = unoccupied;
    return unoccupied;
}

void World::fit_block(size_t blk_x, size_t blk_y, Block &blk, bool keep_old) {
    Rect<int> r;
    size_t off_chx = blk_x * Block::N, off_chy = blk_y * Block::N;
    for (size_t j = 0; j < Block::N; ++j) {
        for (size_t i = 0; i < Block::N; ++i) {
            size_t ch_x = off_chx + i, ch_y = off_chy + j;
            auto &ch = blk.chunks[j][i];
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
            //this can possibly fix the issue with chunk borders;
            //BUT: the overlap must be carefully controlled to avoid data races!!!
            //(not WORLD_BOUNDS, but rather LOADED_BOUNDS, which isn't necceserily a rectangle...)
            /* ch.next_dirty_rect = ch.next_dirty_rect.intersection(WORLD_BOUNDS); */
        }
    }

    for (size_t j = 0; j < Block::N; ++j) {
        for (size_t i = 0; i < Block::N; ++i) {
            auto &ch = blk.chunks[j][i];
            ch.cur_dirty_rect = ch.next_dirty_rect;
            ch.next_dirty_rect.reset();

            if (!ch.cur_dirty_rect.is_empty()) {
                ch.needs_redrawing.include(chunk_bounds(off_chx + i, off_chy + j).intersection(ch.cur_dirty_rect));
            }
        }
    }
}

