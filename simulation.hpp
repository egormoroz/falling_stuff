#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "render_buffer.hpp"
#include "util.hpp"
#include <SFML/System/Time.hpp>
#include <random>
#include "grid.hpp"

//60 ticks/s
const sf::Time FIXED_TIME_STEP = sf::seconds(1.f / 60);
using V2f = sf::Vector2f;
using V2i = sf::Vector2i;

enum class ParticleType {
    None = 0,
    Sand,
    Water,
    Wood,
    Fire,
};

struct Particle {
    ParticleType pt = ParticleType::None;
    bool been_updated;
    V2i vel;
    sf::Time lifetime;
};

struct ChunkFlags {
    //dirty rect complicates things quite a bit
    bool needs_update = true, needs_redraw = true;
};

struct ChunkFlagsBuffers {
    Grid<ChunkFlags> first, second;
    Grid<ChunkFlags> *current, *next;

    ChunkFlagsBuffers(int width, int height)
        : first(width, height), second(width, height),
          current(&first), next(&second) {}

    void swap_buffers() {
        std::swap(current, next);
    }
};

class Simulation {
public:
    Simulation(int width, int height, int chunk_size);

    void update();
    void render();
    const sf::Texture& get_texture() const;

    void spawn_cloud(int cx, int cy, int r, ParticleType pt);

    bool is_chunk_dirty(int ch_x, int ch_y) const;

private:
    Grid<Particle> m_grid;
    ChunkFlagsBuffers m_flags;
    RenderBuffer m_buffer;
    std::mt19937 m_gen;
    int m_update_dir, m_water_spread, m_chunk_size;
    int m_chunks_width, m_chunks_height;

    //Physics
    void update_particle(int x, int y);
    void update_sand(int x, int y);
    void update_water(int x, int y);
    void update_fire(int x, int y);

    //Graphics
    void redraw_particle(int x, int y);

    //utility
    bool test(int x, int y, ParticleType pt) const;
    Rect bounds() const;

    void put_particle(int x, int y, ParticleType pt);
    void swap(int x, int y, int xx, int yy);
    void reset(int x, int y, bool with_neighbours = false);

    void update_row(int start, int end, int ch_y);
};

#endif
