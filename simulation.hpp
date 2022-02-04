#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <SFML/System/Time.hpp>
#include "render_buffer.hpp"
#include "threadpool.hpp"
#include "xorshift.hpp"
#include "world.hpp"

//60 ticks/s
const sf::Time FIXED_TIME_STEP = sf::seconds(1.f / 60);
const size_t MAX_THREADS = 8;

class Simulation {
public:
    Simulation();

    void update();
    void render();

    const sf::Texture& get_texture() const;

    void spawn_cloud(int cx, int cy, int r, ParticleType pt);

    bool is_chunk_dirty(int ch_x, int ch_y) const;

    int num_updated_particles() const;
    int num_tested_particles() const;

private:
    std::unique_ptr<World> m_world;
    RenderBuffer m_buffer;
    ThreadPool m_pool;
    std::queue<ThreadPool::Task> m_tasks_buffer;

    //one for each thread
    XorShift m_gens[MAX_THREADS];

    int m_updated_particles[MAX_THREADS], m_tested_particles[MAX_THREADS];
    int m_water_spread;
    int8_t m_upd_vdir, m_upd_hdir, m_upd_dir_state;

    //Physics
    void update_chunk(size_t ch_x, size_t ch_y, Block::Chunk &ch, 
            size_t worker_idx);

    void update_particle(int x, int y, Block::Chunk &ch, 
            Particle &p, size_t worker_idx);
    void update_particle(int x, int y, Block::Chunk &ch, 
            Sand &p, size_t worker_idx);
    void update_particle(int x, int y, Block::Chunk &ch, 
            Water &p, size_t worker_idx);
    void update_particle(int x, int y, Block::Chunk &ch, 
            Fire &p, size_t worker_idx);

    //Graphics
    void redraw_chunk(size_t ch_x, size_t ch_y, Block::Chunk& ch,
            size_t worker_idx);
    void redraw_particle(int x, int y, Particle &p);

    //utility
    void swap(int x, int y, int xx, int yy);

    template<bool with_neighbours = true>
    void mark(int x, int y) {
        auto &ch = m_world->get_chunk(x / Block::CHUNK_SIZE, y / Block::CHUNK_SIZE);
        ch.next_dirty_rect.include<with_neighbours>(x, y);
    }
};

#endif
