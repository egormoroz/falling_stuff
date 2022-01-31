#ifndef WORLD_HPP
#define WORLD_HPP

#include "render_buffer.hpp"
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/System/Time.hpp>
#include <random>
#include "util.hpp"

//60 ticks per second
const sf::Time FIXED_TIME_STEP = sf::seconds(1.f / 60);
const int SIZE = 256;
const int CHUNK_SIZE = 256;
const int SIZE_IN_CHUNKS = SIZE / CHUNK_SIZE;

using V2f = sf::Vector2f;
using V2i = sf::Vector2i;

enum ParticleType {
    None = 0,
    Sand,
    Water,
    Wood,
    Fire,
};

//can pack those into union if memory usage is an issue
struct Particle {
    ParticleType pt;
    bool been_updated;
    float flow_vel;
    float water_hack_timer;
    V2i vel;
    sf::Time lifetime;
};

struct Chunk {
    Rect needs_update, needs_redraw;
};

class World : public sf::Drawable {
public:
    World();

    void invalidate();

    void update();
    void render();

    void spawn_particle(int x, int y, ParticleType pt);
    void spawn_cloud(int x, int y, int r, ParticleType pt);

    void dump_buffer(const char* path);

private:
    Particle m_grid[SIZE][SIZE];
    Chunk m_chunks[SIZE_IN_CHUNKS * SIZE_IN_CHUNKS];
    std::vector<int> m_col_indices;

    RenderBuffer m_buffer;

    int8_t m_update_pass_dir = 1, m_flow_switch = -1;
    std::mt19937 m_gen;
    std::uniform_real_distribution<float> m_probs_dist;

    //Physics
    void update_chunk(int ch_x, int ch_y);
    void update_particle(int x, int y);
    void update_sand(int x, int y);
    void update_water(int x, int y);
    void update_fire(int x, int y);

    void push_water_out(int x, int y, int dir);

    //Utility
    void swap(int x, int y, int xx, int yy);
    void remove(int x, int y);
    void put_particle(int x, int y, ParticleType pt);

    void mark_for_update(int x, int y);
    void mark_for_update(const Rect& r);

    void mark_for_redraw(int x, int y);
    void mark_for_redraw(const Rect& r);

    Chunk& chunk(int x, int y);
    const Chunk& chunk(int x, int y) const;
    size_t chunk_by_pos(int x, int y) const;

    void reshuffle_indices();

    void check_sanity();

    //Graphics
    void redraw_particle(int x, int y);
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
};

#endif

