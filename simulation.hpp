#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "render_buffer.hpp"
#include "util.hpp"
#include <SFML/System/Time.hpp>
#include <random>
#include "grid.hpp"
#include "threadpool.hpp"
#include "partitioner.hpp"

//60 ticks/s
const sf::Time FIXED_TIME_STEP = sf::seconds(1.f / 60);
using V2f = sf::Vector2f;
using V2i = sf::Vector2i;

const int WIDTH = 2048;
const int HEIGHT = 2048;
const int CHUNK_SIZE = 256;
const int BLOCK_SIZE = CHUNK_SIZE / 4;
const bool MULTITHREADING = false;


enum class ParticleType {
    None = 0,
    Sand,
    Water,
    Wood,
    Fire,
};

struct Particle {
    ParticleType pt = ParticleType::None;
    bool been_updated = false;
    V2i vel = V2i(-1, 0);
    sf::Time lifetime;

    static Particle create(ParticleType pt);
};

class Simulation {
public:
    Simulation(int width, int height, int chunk_size, size_t n_threads);

    void update();
    void update_columnwise();

    void render();
    const sf::Texture& get_texture() const;

    void spawn_cloud(int cx, int cy, int r, ParticleType pt);

    bool is_block_dirty(int blk_x, int blk_y) const;
    int num_updated_particles() const;

private:
    Grid<Particle> m_grid;
    RenderBuffer m_buffer;
    int m_update_dir, m_water_spread;
    int m_chunk_size, m_block_size;
    int m_chunks_width, m_chunks_height;
    ThreadPool m_pool;
    //one for each thread
    std::vector<std::mt19937> m_gens;
    Partitioner m_partitioner;
    DirtyBlocks m_db;
    int m_updated_particles;

    //Physics
    void prepare_region(int i, int j);
    int update_region(int i, int j, size_t worker_idx);

    int update_particle(int x, int y, size_t worker_idx);
    void update_sand(int x, int y);
    void update_water(int x, int y);
    void update_fire(int x, int y, size_t worker_idx);

    //Graphics
    void redraw_region(int i, int j);
    void redraw_particle(int x, int y);

    //utility
    void swap(int x, int y, int xx, int yy);
    bool test(int x, int y, ParticleType pt) const;

    //division here is somewhat expensive
    //making m_block_size const leads to a noticable reduction of cpu-time spent here
    template<BlockQuery QUERY = QBOTH>
    void mark_pixel(int x, int y, bool with_neighbours = false) {
        int blk_x = x / BLOCK_SIZE, blk_y = y / BLOCK_SIZE,
            rem_x = x % BLOCK_SIZE, rem_y = y % BLOCK_SIZE;
        m_db.mark_block<QUERY>(blk_x, blk_y);

        if (!with_neighbours)
            return;

        if (x && !rem_x) {
            m_db.mark_block<QUERY>(blk_x - 1, blk_y);
            if (y && !rem_y)
                m_db.mark_block<QUERY>(blk_x - 1, blk_y - 1);
            if (rem_y + 1 == BLOCK_SIZE && y + 1 < m_grid.height())
                m_db.mark_block<QUERY>(blk_x - 1, blk_y + 1);
        }

        if (x + 1 < m_grid.width() && rem_x + 1 == BLOCK_SIZE) {
            m_db.mark_block<QUERY>(blk_x + 1, blk_y);
            if (y && !rem_y)
                m_db.mark_block<QUERY>(blk_x + 1, blk_y - 1);
            if (rem_y + 1 == BLOCK_SIZE && y + 1 < m_grid.height())
                m_db.mark_block<QUERY>(blk_x + 1, blk_y + 1);

        }

        if (y && !rem_y)
            m_db.mark_block<QUERY>(blk_x, blk_y - 1);
        if (rem_y + 1 == BLOCK_SIZE && y + 1 < m_grid.height())
            m_db.mark_block<QUERY>(blk_x, blk_y + 1);
    }
};

#endif
