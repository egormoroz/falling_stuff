#include "world.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

//helper

sf::Color interpolate_color(sf::Color zero, sf::Color one, float d) {
    sf::Color diff = one - zero;
    return sf::Color(diff.r * d, diff.g * d, diff.b * d, diff.a * d)
        + zero;
}

World::World() : m_buffer(SIZE, SIZE) {
    memset(m_grid, 0, sizeof(m_grid));
    invalidate();
}


void World::invalidate() {
    m_dirty_rect = {0, 0, SIZE - 1, SIZE - 1};
    m_needs_redrawing = m_dirty_rect;
}

void World::update() {
    for (int y = m_dirty_rect.top; y <= m_dirty_rect.bottom; ++y)
        for (int x = m_dirty_rect.left; x <= m_dirty_rect.right; ++x)
            m_grid[y][x].been_updated = false;

    MyRect dirty_rect = m_dirty_rect;
    m_dirty_rect = {SIZE, SIZE, -1, -1};

    if (m_update_pass_dir > 0) {
        for (int y = dirty_rect.bottom; y >= dirty_rect.top; --y) {
            for (int x = dirty_rect.left; x <= dirty_rect.right; ++x) {
                update_particle(x, y);
            }
        }
    } else {
        for (int y = dirty_rect.bottom; y >= dirty_rect.top; --y) {
            for (int x = dirty_rect.right; x >= dirty_rect.left; --x) {
                update_particle(x, y);
            }
        }
    }

    m_needs_redrawing.extend(m_dirty_rect);
    m_dirty_rect.extend_by_until(1, SIZE - 1, SIZE - 1);
    m_update_pass_dir *= -1;
}

void World::render() {
    for (int y = m_needs_redrawing.top; y <= m_needs_redrawing.bottom; ++y) {
        for (int x = m_needs_redrawing.left; x <= m_needs_redrawing.right; ++x) {
            redraw_particle(x, y);
        }
    }
    if (m_needs_redrawing.is_valid()) {
        m_buffer.flush(m_needs_redrawing.left, m_needs_redrawing.top, 
            m_needs_redrawing.right, m_needs_redrawing.bottom);
    }
    m_needs_redrawing = {SIZE, SIZE, -1, -1};
}

void World::spawn_particle(int x, int y, ParticleType pt) {
    Particle &p = m_grid[y][x];
    p.been_updated = false;
    p.flow_vel = m_flow_switch;
    p.life_time = FIRE_LIFETIME;
    p.pt = pt;
    p.vel = V2i();

    m_flow_switch *= -1;
    m_dirty_rect.include(x, y);
    m_needs_redrawing.include(x, y);
}

const MyRect& World::dirty_rect() const {
    return m_needs_redrawing;
}

void World::dump_buffer(const char *path) {
    m_buffer.get_texture().copyToImage().saveToFile(path);
}

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
    case Fire:
        update_fire(x, y);
    default:
        break;
    }
}

//TODO: The code here is really messy. Refactor me!
//Or at least add some comments...
void World::update_sand(int x, int y) {
    Particle &p = m_grid[y][x];
    p.vel.y += 2; //gravity
    int vy = std::max(1, abs(p.vel.y) / 16), sy = p.vel.y > 0 ? 1 : -1,
        vx = abs(p.vel.x / 16), sx = p.vel.x > 0 ? 1 : -1,
        orig_x = x, orig_y = y;
    while (vy) {
        if (y + sy >= SIZE || y + sy < 0) {
            p.vel.y = 0;
            if (p.vel.x)
                p.vel.x -= sx;
            break;
        }

        if (m_grid[y + sy][x].pt == None
            || m_grid[y + sy][x].pt == Water) 
        {
            if (m_grid[y + sy][x].pt == Water)
                push_water_out(x, y + sy, sx);
            y += sy;
        } else if (x > 0 && (m_grid[y + sy][x - 1].pt == None
            || m_grid[y + sy][x - 1].pt == Water)) 
        {
            if (m_grid[y + sy][x - 1].pt == Water)
                push_water_out(x - 1, y + sy, -1);
            y += sy;
            --x;
            if (p.vel.y)
                p.vel.y -= sy;
            p.vel.x -= 2;
        } else if (x + 1 < SIZE && (m_grid[y + sy][x + 1].pt == None
            || m_grid[y + sy][x + 1].pt == Water)) 
        {
            if (m_grid[y + sy][x + 1].pt == Water)
                push_water_out(x + 1, y + sy, 1);
            y += sy;
            ++x;
            if (p.vel.y)
                p.vel.y -= sy;
            p.vel.x += 2;
        } else {
            V2i vel = m_grid[y + sy][x].vel;
            if (abs(vel.y) < abs(p.vel.y))
                p.vel.y = vel.y;
            if (p.vel.x)
                p.vel.x -= sx;
            break;
        }

        --vy;
    }

    vx = abs(p.vel.x) / 16; 
    sx = p.vel.x > 0 ? 1 : -1;
    while (vx) {
        if (x + sx >= SIZE || x + sx < 0 || m_grid[y][x + sx].pt != None) {
            p.vel.x = 0;
            break;
        }
        x += sx;
        --vx;
    }

    if (x != orig_x || y != orig_y)
        swap(x, y, orig_x, orig_y);
}

void World::update_water(int x, int y) {
    Particle &p = m_grid[y][x];
    float &fw = p.flow_vel;
    int orig_x = x, orig_y = y;

    //assuming p.vel.y >= 0
    p.vel.y += 2; //gravity
    int vy = std::max(1, p.vel.y / 16);

    while (vy) {
        if (y + 1 >= SIZE) {
            p.vel.y = 0;
            break;
        }

        if (m_grid[y + 1][x].pt == None) {
            ++y;
            p.water_hack_timer = WATER_HACK_TIMER;
        } else if (x > 0 && m_grid[y + 1][x - 1].pt == None) {
            ++y;
            --x;
            fw = -abs(fw);
            p.vel.y = std::max(0, p.vel.y - 1);
            p.water_hack_timer = WATER_HACK_TIMER;
        } else if (x + 1 < SIZE && m_grid[y + 1][x + 1].pt == None) {
            ++y;
            ++x;
            fw = abs(fw);
            p.vel.y = std::max(0, p.vel.y - 1);
            p.water_hack_timer = WATER_HACK_TIMER;
        } else {
            p.vel.y = std::min(m_grid[y + 1][x].vel.y, p.vel.y);
            break;
        }

        --vy;
    }

    int move_points = abs(fw), ms = fw > 0 ? 1 : -1;
    fw -= ms * FLOW_DECAY;
    if (abs(fw) < 1.f)
        fw = ms;

    while (y + 1 < SIZE && move_points > 0) {
        bool can_down_left = x > 0 && m_grid[y + 1][x - 1].pt == None,
            can_down_right = x + 1 < SIZE && m_grid[y + 1][x + 1].pt == None;
        if (m_grid[y + 1][x].pt == None) {
            ++y;
            p.water_hack_timer = WATER_HACK_TIMER;
        } else if (can_down_left) {
            if (ms > 0 && can_down_right) {
                ++y;
                ++x;
                fw = abs(fw) + FLOW_ACC;
                p.water_hack_timer = WATER_HACK_TIMER;
            } else {
                ++y;
                --x;
                fw = -abs(fw) - FLOW_ACC;
                p.water_hack_timer = WATER_HACK_TIMER;
            }
        } else if (can_down_right) {
            ++y;
            ++x;
            fw = abs(fw) + FLOW_ACC;
            p.water_hack_timer = WATER_HACK_TIMER;
        } else if (x + ms >= 0 && x + ms < SIZE && m_grid[y][x + ms].pt == None) {
            fw += ms * FLOW_ACC;
            x += ms;
            //if particle is just floating atop, out of place, we should clean it up
            if ((y < 1 || m_grid[y - 1][x].pt != Water) && abs(fw) > WATER_HACK_TH) {
                p.water_hack_timer -= FIXED_FRAME_TIME;
            }
        } else if (x - ms >= 0 && x - ms < SIZE && m_grid[y][x - ms].pt == None) {
            fw += ms * FLOW_ACC;
            fw = -fw;
            ms = -ms;
            move_points = ms;
            x += ms;
            if ((y < 1 || m_grid[y - 1][x].pt != Water) && abs(fw) > WATER_HACK_TH) {
                p.water_hack_timer -= FIXED_FRAME_TIME;
            }
        } else {
            break;
        }
        
        --move_points;
    }

    if (abs(fw) > MAX_FLOW)
        fw = ms * MAX_FLOW;


    if (p.water_hack_timer > 0) {
        if (x != orig_x || y != orig_y) {
            swap(x, y, orig_x, orig_y);
        }
    } else {
        //Dirty hack.
        if (abs(orig_x - x) == 1) {
            //fill the other cell
            m_grid[orig_y][orig_x].water_hack_timer = WATER_HACK_TIMER;
            m_grid[y][x] = m_grid[orig_y][orig_x];
            m_dirty_rect.include(x, y);
        } else {
            //remove the leftovers
            remove(orig_x, orig_y);
        }
    }
}

void World::update_fire(int x, int y) {
    Particle &p = m_grid[y][x];
    p.life_time -= FIXED_FRAME_TIME;
    m_dirty_rect.include(x, y);
    if (p.life_time < 0)
        p.pt = None;
}

void World::push_water_out(int x, int y, int dir) {
    const int dx = 1, dy = 1;
    if (y < dy) {
        /* remove(x, y); */
        return;
    }
    bool top_left_empty = x > dx && m_grid[y - dy][x - dx].pt == None,
         top_right_empty = x + dx < SIZE && m_grid[y - dy][x + dx].pt == None,
         top_empty = m_grid[y - dy][x].pt == None;
    if (dir < 0 && top_left_empty)
        swap(x, y, x - dx, y - dy);
    else if (dir > 0 && top_right_empty)
        swap(x, y, x + dx, y - dy);
    else if (top_left_empty)
        swap(x, y, x - dx, y - dy);
    else if (top_right_empty)
        swap(x, y, x + dx, y - dy);
    else if (top_empty)
        swap(x, y, x, y - dy);
    /* else */
    /*     remove(x, y); */
}

void World::swap(int x, int y, int xx, int yy) {
    std::swap(m_grid[y][x], m_grid[yy][xx]);
    m_dirty_rect.include(x, y);
    m_dirty_rect.include(xx, yy);
}

void World::remove(int x, int y) {
    m_grid[y][x].pt = None;
    m_dirty_rect.include(x, y);
}

void World::redraw_particle(int x, int y) {
    switch (m_grid[y][x].pt) {
    case None:
        m_buffer.pixel(x, y) = sf::Color::Black;
        break;
    case Sand:
        m_buffer.pixel(x, y) = sf::Color::Yellow;
        break;
    case Water:
        m_buffer.pixel(x, y) = sf::Color::Blue;
        break;
    case Wood:
        m_buffer.pixel(x, y) = sf::Color(64, 0, 0);
        break;
    case Fire:
        m_buffer.pixel(x, y) = interpolate_color(
            sf::Color(255, 80, 0), //orange
            sf::Color::Yellow,
            m_grid[y][x].life_time / FIRE_LIFETIME
        );
        break;
    default:
        break;
    }
}

void World::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    sf::Sprite sprite(m_buffer.get_texture());
    sprite.setScale(PARTICLE_SIZE, PARTICLE_SIZE);
    target.draw(sprite, states);
}

