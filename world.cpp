#include "world.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTarget.hpp>


World::World() : m_buffer(SIZE, SIZE) {
    memset(m_grid, 0, sizeof(m_grid));
    invalidate();
}


void World::invalidate() {
    m_dirty_rect = {0, 0, SIZE - 1, SIZE - 1};
    m_needs_redrawing = m_dirty_rect;
}

void World::update() {
    for (int y = 0; y < SIZE; ++y)
        for (int x = 0; x < SIZE; ++x)
            m_grid[y][x].been_updated = false;

    MyRect dirty_rect = m_dirty_rect;
    m_dirty_rect = {SIZE, SIZE, -1, -1};

    if (update_pass_dir > 0) {
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
    update_pass_dir *= -1;
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

void World::set_particle(int x, int y, Particle p) { 
    m_dirty_rect.extend(x, y);
    m_needs_redrawing.extend(x, y);
    m_grid[y][x] = p;
}

const Particle& World::particle(int x, int y) const { return m_grid[y][x]; }

const MyRect& World::dirty_rect() const {
    return m_needs_redrawing;
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
        swap(x, y, dst_x, dst_y);
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
            p.water_hack_timer = WATER_HACK_TIMER;
        } else if (can_down_left) {
            ++y;
            --x;
            fw = -abs(fw) - FLOW_ACC;
            p.water_hack_timer = WATER_HACK_TIMER;
        } else if (can_down_right) {
            ++y;
            ++x;
            fw = abs(fw) + FLOW_ACC;
            p.water_hack_timer = WATER_HACK_TIMER;
        } else if (true || m_grid[y + 1][x].pt == Water) {
            if (x + ms >= 0 && x + ms < SIZE && m_grid[y][x + ms].pt == None) {
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
        } else {
            break;
        }
        move_points -= ms;
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
            m_dirty_rect.extend(x, y);
        } else {
            //remove the leftovers
            m_grid[orig_y][orig_x].pt = None;
            m_dirty_rect.extend(orig_x, orig_y);
        }
    }
}

void World::swap(int x, int y, int xx, int yy) {
    std::swap(m_grid[y][x], m_grid[yy][xx]);
    m_dirty_rect.extend(x, y);
    m_dirty_rect.extend(xx, yy);
}

void World::redraw_particle(int x, int y) {
    switch (m_grid[y][x].pt) {
    case Sand:
        m_buffer.pixel(x, y) = sf::Color::Yellow;
        break;
    case Water:
    {
        /* float val = (abs(m_grid[y][x].flow_vel) - 1) / MAX_FLOW; */
        /* float r = (1.f - 0.f) * val + 0.f, */
        /*     g = 0.f, */
        /*     b = (0.f - 1.f) * val + 1.f; */
        /* m_buffer.pixel(x, y) = sf::Color( */
        /*     255 * r, 255 * g, 255 * b */
        /* ); */
        m_buffer.pixel(x, y) = sf::Color::Blue;
        break;
    }
    case Wood:
        m_buffer.pixel(x, y) = sf::Color(64, 0, 0);
        break;
    default:
        m_buffer.pixel(x, y) = sf::Color::Black;
        break;
    }
}

void World::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    sf::Sprite sprite(m_buffer.get_texture());
    sprite.setScale(PARTICLE_SIZE, PARTICLE_SIZE);
    target.draw(sprite, states);
}

