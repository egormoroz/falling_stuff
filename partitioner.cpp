#include "partitioner.hpp"


Partitioner::Partitioner(int width, int height, int chunk_width, 
        int chunk_height, int blk_width, int blk_height) 
    :  m_width(width), m_height(height), m_chunk_width(chunk_width),
       m_chunk_height(chunk_height), m_blk_width(blk_width), m_blk_height(blk_height)
{
    m_xbounds.reserve((width + chunk_width - 1) / chunk_width + 1);
    m_ybounds.reserve((height + chunk_height - 1) / chunk_height + 1);
    shift(4, 4);
}

void Partitioner::shift(int offx, int offy) {
    m_xbounds.clear();
    m_ybounds.clear();
    m_xbounds.push_back(0);
    m_ybounds.push_back(0);

    for (int i = m_blk_width * offx; i < m_width; i += m_chunk_width)
        m_xbounds.push_back(i);
    m_xbounds.push_back(m_width);

    for (int j = m_blk_height * offy; j < m_height; j += m_chunk_height)
        m_ybounds.push_back(j);
    m_ybounds.push_back(m_height);
}

int Partitioner::x_bound(int k) const { return m_xbounds[k]; }

int Partitioner::y_bound(int k) const { return m_ybounds[k]; }

int Partitioner::num_xbounds() const { return m_xbounds.size(); }

int Partitioner::num_ybounds() const { return m_ybounds.size(); }


DirtyBlocks::DirtyBlocks(int width_chunks, int height_chunks)
    : m_first(width_chunks + 1, height_chunks + 1), 
      m_second(width_chunks + 1, height_chunks + 1),
      m_current(&m_first), m_next(&m_second), m_offx(4), m_offy(4)
{
}


void DirtyBlocks::new_shift(int new_offx, int new_offy) {
    std::fill(m_next->begin(), m_next->end(), Region{0, 0});
    int width = 4 * (m_current->width() - 1), height = 4 * (m_current->height() - 1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int xx = x + 4 - new_offx, yy = y + 4 - new_offy,
                ch_x = xx / 4, ch_y = yy / 4, blk_x = xx % 4, blk_y = yy % 4;

            uint16_t update = test_block<QUPDATE>(x, y),
                     redraw = test_block<QREDRAW>(x, y),
                     shift = blk_y * 4 + blk_x;
            Region &r = m_next->get(ch_x, ch_y);
            r.update_mask |= update << shift;
            r.redraw_mask |= redraw << shift;
        }
    }
    m_offx = new_offx;
    m_offy = new_offy;
    std::swap(m_current, m_next);
}

