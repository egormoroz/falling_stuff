#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <SFML/System/Time.hpp>
#include "render_buffer.hpp"
#include "xorshift.hpp"
#include "updatescheduler.hpp"
#include "particle.hpp"
#include "rect.hpp"

//60 ticks/s
const sf::Time FIXED_TIME_STEP = sf::seconds(1.f / 60);
const size_t MAX_THREADS = 8;

class World;
struct Block;
struct Chunk;

using scheduler::UpdateScheduler;

class Simulation {
    friend class UpdateScheduler;
public:
    Simulation();

    void update();
    void render();

    const sf::Texture& get_texture() const;

    void spawn_cloud(int cx, int cy, int r, ParticleType pt);

    const Rect<int>& chunk_dirty_rect_next(int ch_x, int ch_y) const;
    const Rect<int>& chunk_dirty_rect_cur(int ch_x, int ch_y) const;
    bool is_chunk_dirty(int ch_x, int ch_y) const;

    int num_updated_particles() const;
    int num_tested_particles() const;
    const std::vector<scheduler::LoadTracker>& get_load_stats() const;

private:
    std::unique_ptr<World> m_world;
    RenderBuffer m_buffer;
    UpdateScheduler m_scheduler;

    //one for each thread
    XorShift m_gens[MAX_THREADS];

    int m_updated_particles[MAX_THREADS], m_tested_particles[MAX_THREADS];
    int m_water_spread;
    int8_t m_upd_vdir, m_upd_hdir, m_upd_dir_state;

    Rect<size_t> m_view;

    //Physics
    void prepare_chunk(size_t ch_x, size_t ch_y, Chunk &ch, 
            size_t worker_idx);

    void update_chunk(size_t ch_x, size_t ch_y, Chunk &ch, 
            size_t worker_idx);

    void update_particle(int x, int y, Chunk &ch, 
            Particle &p, size_t worker_idx);
    void update_particle(int x, int y, Chunk &ch, 
            Sand &p, size_t worker_idx);
    void update_particle(int x, int y, Chunk &ch, 
            Water &p, size_t worker_idx);
    void update_particle(int x, int y, Chunk &ch, 
            Fire &p, size_t worker_idx);

    //Graphics
    void render_chunk(size_t ch_x, size_t ch_y, Chunk& ch,
            size_t worker_idx);
    void redraw_particle(int x, int y, Particle &p);

    //utility
    void swap(int x, int y, int xx, int yy);

    void mark(int x, int y);
    void mark_with_neighbours(int x, int y);
};

#endif
