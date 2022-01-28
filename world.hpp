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
const float MAX_FLOW = SIZE / 12.f;
const float WATER_HACK_TH = MAX_FLOW * 0.75f;
const float WATER_HACK_TIMER = 0.5f;

using V2f = sf::Vector2f;
using V2i = sf::Vector2i;

enum ParticleType {
    None = 0,
    Sand,
    Water,
    Wood,
};

struct Particle {
    ParticleType pt;
    bool been_updated;
    float flow_vel;
    float water_hack_timer;
};

struct MyRect {
    int left, top, right, bottom;

    void extend_by_until(int d, int max_right, int max_bottom) {
        left = std::max(left - d, 0);
        top = std::max(top - d, 0);
        right = std::min(right + d, max_right);
        bottom = std::min(bottom + d, max_bottom);
    }

    void extend(int x, int y) {
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

    void set_particle(int x, int y, Particle p);
    const Particle& particle(int x, int y) const;

    const MyRect& dirty_rect() const;

private:
    Particle m_grid[SIZE][SIZE];
    RenderBuffer m_buffer;
    int8_t update_pass_dir = 1;
    MyRect m_dirty_rect, m_needs_redrawing;

//Physics
    void update_particle(int x, int y);
    void update_sand(int x, int y);
    void update_water(int x, int y);

//Utility
    void swap(int x, int y, int xx, int yy);

//Graphics
    void redraw_particle(int x, int y);
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};

#endif
