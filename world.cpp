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
                m_buffer.pixel(x, y) = sf::Color::Blue;
                break;
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
    bool is_falling = false;
    if (yy < SIZE) {
        if (m_grid[yy][x].pt == None || m_grid[yy][x].pt == Water) {
            dst_y = yy;
            is_falling = true;
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
    if (!is_falling)
        m_grid[dst_y][dst_x].vel.y /= 2;
}

void World::update_water(int x, int y) {
    float &fw = m_grid[y][x].flow_vel;
    int orig_x = x, orig_y = y;
    int move_points = fw, ms = move_points / abs(move_points);
    while (y + 1 < SIZE && move_points) {
        bool can_down_left = x > 0 && m_grid[y + 1][x - 1].pt == None,
            can_down_right = x + 1 < SIZE && m_grid[y + 1][x + 1].pt == None;
        if (m_grid[y + 1][x].pt == None) {
            ++y;
        } else if (can_down_left) {
            ++y;
            --x;
            fw = -abs(fw);
        } else if (can_down_right) {
            ++y;
            ++x;
            fw = abs(fw);
        } else if (m_grid[y + 1][x].pt == Water) {
            float new_fw = fw + SPREAD_ACC * ms;
            if (abs(new_fw) < MAX_SPREAD) {
                fw = new_fw;
            }
            if (x + ms >= 0 && x + ms < SIZE && m_grid[y][x + ms].pt == None) {
                x += ms;
            } else if (x - ms >= 0 && x - ms < SIZE && m_grid[y][x - ms].pt == None) {
                fw = -fw;
                ms = -ms;
                move_points = ms;
                x += ms;
            } else {
                break;
            }
        } else {
            break;
        }
        move_points -= ms;
    }

    if (x != orig_x || y != orig_y) {
        std::swap(m_grid[y][x], m_grid[orig_y][orig_x]);
    } else {
        m_grid[y][x].flow_vel = ms;
    }
    //if it wasn't just a straight fall
    if (x != orig_x) {
        m_grid[y][x].vel.y /= 2;
    }
}

void World::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    sf::Sprite sprite(m_buffer.get_texture());
    sprite.setScale(PARTICLE_SIZE, PARTICLE_SIZE);
    target.draw(sprite, states);
}

