#ifndef WORLD_HPP
#define WORLD_HPP

#include "render_buffer.hpp"
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/System/Time.hpp>
#include <random>
#include "util.hpp"

const sf::Time FIXED_FRAME_TIME = sf::seconds(1.f / 60);
const int SCR_SIZE = 900;
const int SIZE = 512;

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

class World : public sf::Drawable {
public:
    World();

    void invalidate();

    void update();
    void render();

    void spawn_particle(int x, int y, ParticleType pt);
    void spawn_cloud(int x, int y, int r, ParticleType pt);

    const Rect& dirty_rect() const;

    void dump_buffer(const char *path);

private:
    Particle m_grid[SIZE][SIZE];
    RenderBuffer m_buffer;
    int8_t m_update_pass_dir = 1, m_flow_switch = -1;
    Rect m_dirty_rect, m_needs_redrawing;
    std::mt19937 m_gen;
    std::uniform_int_distribution<size_t> m_flicker_dist;
    std::uniform_real_distribution<float> m_probs_dist;

//Physics
    void update_particle(int x, int y);
    void update_sand(int x, int y);
    void update_water(int x, int y);
    void update_fire(int x, int y);

    void push_water_out(int x, int y, int dir);

//Utility
    void swap(int x, int y, int xx, int yy);
    void remove(int x, int y);
    void put_particle(int x, int y, ParticleType pt);

//Graphics
    void redraw_particle(int x, int y);
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};

#endif
