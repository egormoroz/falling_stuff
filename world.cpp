#include "world.hpp"
#include <memory>
#include <cassert>

World::World() {
    m_block.left = 0;
    m_block.top = 0;

    memset(&m_block, 0, sizeof(m_block));
    for (auto &i: m_block.chunks)
        for (auto &j: i) {
            j.cur_dirty_rect.reset();
            j.next_dirty_rect.reset();
        }
}

bool World::contains_chunk(size_t ch_x, size_t ch_y) const {
    return ch_x < Block::N && ch_y < Block::N;
}

Block::Chunk& World::get_chunk(size_t ch_x, size_t ch_y) {
    return m_block.chunks[ch_y][ch_x];
}

const Block::Chunk& World::get_chunk(size_t ch_x, size_t ch_y) const {
    return m_block.chunks[ch_y][ch_x];
}

Particle& World::get(size_t x, size_t y) {
    return m_block.get(x, y);
}

const Particle& World::get(size_t x, size_t y) const {
    return m_block.get(x, y);
}

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
                if (contains_chunk(ch_x - 1, ch_y + 1)) {
                    r = chunk_bounds(ch_x - 1, ch_y + 1);
                    if (bounds.intersects(r))
                        get_chunk(ch_x - 1, ch_y + 1).next_dirty_rect.include(bounds.intersection(r));
                }
            }

            if (contains_chunk(ch_x + 1, ch_y)) {
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
                if (contains_chunk(ch_x + 1, ch_y + 1)) {
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
            if (contains_chunk(ch_x, ch_y + 1)) {
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

