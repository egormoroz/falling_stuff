#ifndef WORLD_HPP
#define WORLD_HPP

#include "render_buffer.hpp"
#include <SFML/Graphics/Drawable.hpp>

const float FIXED_FRAME_TIME = 1.f / 60;
const int SCR_SIZE = 900;
const int SIZE = 256;
const float PARTICLE_SIZE = float(SCR_SIZE) / SIZE;
const float FLOW_ACC = 0.08f;
const float FLOW_DECAY = 0.04f;
const float MAX_FLOW = SIZE / 6.f;
const float WATER_HACK_TH = MAX_FLOW * 0.75f;
const float WATER_HACK_TIMER = 0.5f;
const float FIRE_LIFETIME = 2.f; //secs

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
    ParticleType pt = None;
    bool been_updated = false;
    float flow_vel = -1.f;
    float water_hack_timer = WATER_HACK_TIMER;
    V2i vel;
    float life_time;
};

struct MyRect {
    int left, top, right, bottom;

    bool is_valid() const {
        return right >= left && bottom >= top;
    }

    void extend_by_until(int d, int max_right, int max_bottom) {
        left = std::max(left - d, 0);
        top = std::max(top - d, 0);
        right = std::min(right + d, max_right);
        bottom = std::min(bottom + d, max_bottom);
    }

    void include(int x, int y) {
        left = std::min(left, x);
        top = std::min(top, y);
        right = std::max(right, x);
        bottom = std::max(bottom, y);
    }

    void extend(const MyRect &other) {
        left = std::min(left, other.left);
        top = std::min(top, other.top);
        right = std::max(right, other.right);
        bottom = std::max(bottom, other.bottom);
    }
};

class World : public sf::Drawable {
public:
    World();

    void invalidate();

    void update();
    void render();

    void spawn_particle(int x, int y, ParticleType pt);
    const MyRect& dirty_rect() const;

    void dump_buffer(const char *path);

private:
    Particle m_grid[SIZE][SIZE];
    RenderBuffer m_buffer;
    int8_t m_update_pass_dir = 1, m_flow_switch = -1;
    MyRect m_dirty_rect, m_needs_redrawing;

//Physics
    void update_particle(int x, int y);
    void update_sand(int x, int y);
    void update_water(int x, int y);
    void update_fire(int x, int y);

    void push_water_out(int x, int y, int dir);

//Utility
    void swap(int x, int y, int xx, int yy);
    void remove(int x, int y);

//Graphics
    void redraw_particle(int x, int y);
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};

#endif
