#include "world.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTarget.hpp>


World::World() : m_buffer(SIZE, SIZE) {
    memset(m_grid, 0, sizeof(m_grid));
}

void World::update() {
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
            m_grid[y][x].been_updated = false;

    if (update_pass_dir > 0) {
        for (int y = SIZE - 1; y >= 0; --y) {
            for (int x = 0; x < SIZE; ++x) {
                update_particle(x, y);
            }
        }
    } else {
        for (int y = SIZE - 1; y >= 0; --y) {
            for (int x = SIZE - 1; x >= 0; --x) {
                update_particle(x, y);
            }
        }
    }

    update_pass_dir *= -1;
}

void World::render() {
    m_buffer.clear();
    for (int y = 0; y < SIZE; ++y) {
        for (int x = 0; x < SIZE; ++x) {
            switch (m_grid[y][x].pt) {
            case Sand:
                m_buffer.pixel(x, y) = sf::Color::Yellow;
                break;
            case Water:
            {
                float val = (abs(m_grid[y][x].flow_vel) - 1) / MAX_FLOW;
                //Blue -> Red
                float r = (1.f - 0.f) * val + 0.f,
                    g = 0.f,
                    b = (0.f - 1.f) * val + 1.f;
                m_buffer.pixel(x, y) = sf::Color(
                    255 * r, 255 * g, 255 * b
                );
                break;
            }
            case Wood:
                m_buffer.pixel(x, y) = sf::Color(64, 0, 0);
                break;
            default:
                break;
            }
        }
    }
    m_buffer.flush();
}

Particle& World::particle(int x, int y) { return m_grid[y][x]; }
const Particle& World::particle(int x, int y) const { return m_grid[y][x]; }

void World::update_particle(int x, int y) {
    std::pair<int, int> p;
    if (m_grid[y][x].been_updated)
        return;
    m_grid[y][x].been_updated = true;
    switch (m_grid[y][x].pt) {
    case Sand:
        update_sand(x, y);
        break;
    case Water:
        update_water(x, y);
        break;
    case Wood:
        //do nothing...
        break;
    default:
        break;
    }
}

void World::update_sand(int x, int y) {
    int yy = y + 1, dst_x = x, dst_y = y;
    if (yy < SIZE) {
        if (m_grid[yy][x].pt == None || m_grid[yy][x].pt == Water) {
            dst_y = yy;
        } else if (x - 1 >= 0 && (m_grid[yy][x - 1].pt == None
            || m_grid[yy][x - 1].pt == Water)) {
            dst_y = yy;
            dst_x = x - 1;
        } else if (x + 1 < SIZE && (m_grid[yy][x + 1].pt == None
            || m_grid[yy][x + 1].pt == Water)) {
            dst_y = yy;
            dst_x = x + 1;
        }
    }
    if (x != dst_x || y != dst_y)
        std::swap(m_grid[y][x], m_grid[dst_y][dst_x]);
}

void World::update_water(int x, int y) {
    Particle &p = m_grid[y][x];
    float &fw = p.flow_vel;
    int orig_x = x, orig_y = y;
    int move_points = fw, ms = move_points / abs(move_points);

    fw -= ms * FLOW_DECAY;
    if (abs(fw) < 1.f)
        fw = ms;

    while (y + 1 < SIZE && move_points) {
        bool can_down_left = x > 0 && m_grid[y + 1][x - 1].pt == None,
            can_down_right = x + 1 < SIZE && m_grid[y + 1][x + 1].pt == None;
        if (m_grid[y + 1][x].pt == None) {
            ++y;
            //water is really flowing, reset timer
            p.lifetime = HOT_WATER_LT;
        } else if (can_down_left) {
            ++y;
            --x;
            fw = -abs(fw) - FLOW_ACC;
            p.lifetime = HOT_WATER_LT;
        } else if (can_down_right) {
            ++y;
            ++x;
            fw = abs(fw) + FLOW_ACC;
            p.lifetime = HOT_WATER_LT;
        } else if (m_grid[y + 1][x].pt == Water) {
            if (x + ms >= 0 && x + ms < SIZE && m_grid[y][x + ms].pt == None) {
                fw += ms * FLOW_ACC;
                x += ms;
                //if particle is just floating atop, out of place, we should clean it up
                if ((y < 1 || m_grid[y - 1][x].pt != Water) && abs(fw) > HOT_WATER_TH) {
                    p.lifetime -= FIXED_FRAME_TIME;
                    /* printf("%f\n", p.lifetime); */
                }
            } else if (x - ms >= 0 && x - ms < SIZE && m_grid[y][x - ms].pt == None) {
                fw += ms * FLOW_ACC;
                fw = -fw;
                ms = -ms;
                move_points = ms;
                x += ms;
                if ((y < 1 || m_grid[y - 1][x].pt != Water) && abs(fw) > HOT_WATER_TH) {
                    p.lifetime -= FIXED_FRAME_TIME;
                    /* printf("%f\n", p.lifetime); */
                }
            } else {
                break;
            }
        } else {
            break;
        }
        move_points -= ms;
    }

    if (abs(fw) > MAX_FLOW)
        fw = ms * MAX_FLOW;

    if (p.lifetime > 0) {
        if (x != orig_x || y != orig_y) {
            std::swap(m_grid[y][x], m_grid[orig_y][orig_x]);
        }
    } else {
        //remove this particle
        p.pt = None;
        /* printf("die!!!!\n"); */
    }
}

void World::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    sf::Sprite sprite(m_buffer.get_texture());
    sprite.setScale(PARTICLE_SIZE, PARTICLE_SIZE);
    target.draw(sprite, states);
}

