#ifndef PARTITIONER_HPP
#define PARTITIONER_HPP

#include <stdint.h>
#include <vector>
#include "grid.hpp"

class Partitioner {
public:
    Partitioner(int width, int height, int chunk_width, int chunk_height,
            int blk_width, int blk_height);

    //offset in blocks
    void shift(int offx, int offy);

    int x_bound(int k) const;
    int y_bound(int k) const;

    int num_xbounds() const;
    int num_ybounds() const;

    int off_x() const;
    int off_y() const;

private:
    int m_width, m_height;
    int m_chunk_width, m_chunk_height;
    int m_blk_width, m_blk_height;
    std::vector<int> m_xbounds, m_ybounds;
};

enum BlockQuery {
    QUPDATE = 1,
    QREDRAW = 2,
    QBOTH = 3,
};

class DirtyBlocks {
public:
    DirtyBlocks(int width_chunks, int height_chunks);
    
    template<BlockQuery QUERY>
    void mark_block(int x, int y) {
        x += 4 - m_offx; y += 4 - m_offy;
        int ch_x = x / 4, ch_y = y / 4, blk_x = x % 4, blk_y = y % 4;
        Region &r = m_current->get(ch_x, ch_y);
        uint16_t b = 1u << (blk_y * 4 + blk_x);
        if (QUERY & QUPDATE)
            r.update_mask |= b;
        if (QUERY & QREDRAW)
            r.redraw_mask |= b; 
    }

    template<BlockQuery QUERY>
    bool test_block(int x, int y) const {
        x += 4 - m_offx; y += 4 - m_offy;
        int ch_x = x / 4, ch_y = y / 4, blk_x = x % 4, blk_y = y % 4;
        uint16_t b = 1u << (blk_y * 4 + blk_x);
        const Region &r = m_current->get(ch_x, ch_y);

        bool result = true;
        if (QUERY & QUPDATE)
            result = result && (r.update_mask & b);
        if (QUERY & QREDRAW)
            result = result && (r.redraw_mask & b);
        return result;
    }

    template<BlockQuery QUERY>
    void reset_region(int i, int j) {
        Region &r = m_current->get(i, j);
        if (QUERY & QUPDATE)
            r.update_mask = 0;
        if (QUERY & QREDRAW)
            r.redraw_mask = 0;
    }

    template<BlockQuery QUERY>
    bool test_region(int i, int j) const {
        bool result = true;
        Region &r = m_current->get(i, j);
        if (QUERY & QUPDATE)
            result = result && r.update_mask;
        if (QUERY & QREDRAW)
            result = result && r.redraw_mask;
        return result;
    }

    void new_shift(int new_offx, int new_offy);

private:
    struct Region {
        uint16_t update_mask;
        uint16_t redraw_mask;
    };

    //each region is 4x4 matrix of blocks
    Grid<Region> m_first, m_second;
    Grid<Region> *m_current, *m_next;
    //actual storage location is dynamic, because we need concurrent access to regions,
    //which are generated randomly by Partitioner
    int m_offx, m_offy;
};

#endif

