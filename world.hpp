#ifndef WORLD_HPP
#define WORLD_HPP

#include "render_buffer.hpp"
#include <SFML/Graphics/Drawable.hpp>

const int SCR_SIZE = 900;
const int SIZE = 256;
const float PARTICLE_SIZE = float(SCR_SIZE) / SIZE;
/* const float GRAVITY_ACC = 0.05f; */
/* const float SPREAD_ACC = 0.05; */
const float GRAVITY_ACC = 0.00f;
const float SPREAD_ACC = 0.05;
const float MAX_SPREAD = 20.f;

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
    V2f vel;
    float flow_vel;
};

class World : public sf::Drawable {
public:
    World();

    void update();
    void render();

    Particle& particle(int x, int y);
    const Particle& particle(int x, int y) const;

private:
    Particle m_grid[SIZE][SIZE];
    RenderBuffer m_buffer;
    int8_t update_pass_dir = 1;

//Physics
    void update_particle(int x, int y);
    void update_sand(int x, int y);
    void update_water(int x, int y);

//Graphics
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};

#endif
