#include "simulation.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>

const float FIRE_SPREAD_DELAY = -2.f;

const sf::Time FIRE_LT_MEAN = sf::seconds(4.f);
const sf::Time FIRE_LT_DEV = sf::seconds(1.f);

const size_t NUM_FIRE_FLICKERS = 5;
const sf::Time FLICKER_DURATION = sf::milliseconds(100);
const sf::Color FIRE_FLICKER_COLORS[NUM_FIRE_FLICKERS] = {
    //  R,   G,   B,   A
    { 255, 255,   0, 255 },
    { 255, 200,   0, 255 },
    { 255, 150,   0, 255 },
    { 255, 100,   0, 255 },
    { 255,  50,   0, 255 },
};

Simulation::Simulation(int width, int height, int chunk_size) 
    : m_update_dir(-1), m_chunk_size(chunk_size), 
      m_buffer(width, height), m_grid(width, height), 
      m_flags(width / chunk_size, height / chunk_size),
      m_chunks_width(width / chunk_size), m_chunks_height(height / chunk_size),
      m_water_spread(std::min(6, std::max(1, std::min(width, height) / 32)))
{
    for (auto &i: m_grid) {
        i.pt = ParticleType::None;
        i.vel.x = -1;
    }
}

void Simulation::update() {
    for (int i = 0; i < m_chunks_height; ++i) {
        for (int j = 0; j < m_chunks_width; ++j) {
            if (!m_flags.current->get(j, i).needs_update)
                continue;
            for (int y = i * m_chunk_size; y < (i + 1) * m_chunk_size; ++y)
                for (int x = j * m_chunk_size; x < (j + 1) * m_chunk_size; ++x)
                    m_grid.get(x, y).been_updated = false;
        }
    }

    for (int ch_y = m_chunks_height - 1; ch_y >= 0; --ch_y) {
        for (int end = 0; end < m_chunks_width; ++end) {
            auto &ch = m_flags.current->get(end, ch_y);
            if (!ch.needs_update)
                continue;
            ch.needs_update = false;
            ch.needs_redraw = true;
            int start = end;
            while (++end < m_chunks_width && m_flags.current->get(end, ch_y).needs_update) {
                auto &ch = m_flags.current->get(end, ch_y);
                ch.needs_update = false;
                ch.needs_redraw = true;
            }
            --end;

            update_row(start, end, ch_y);
        }
    }

    m_update_dir *= -1;
    m_flags.swap_buffers();
}

void Simulation::render() {
    for (int ch_y = 0; ch_y < m_chunks_height; ++ch_y) {
        for (int ch_x = 0; ch_x < m_chunks_width; ++ch_x) {
            if (!m_flags.next->get(ch_x, ch_y).needs_redraw)
                continue;
            m_flags.next->get(ch_x, ch_y).needs_redraw = false;
            for (int y = ch_y * m_chunk_size; y < (ch_y + 1) * m_chunk_size; ++y) 
                for (int x = ch_x * m_chunk_size; x < (ch_x + 1) * m_chunk_size; ++x)
                    redraw_particle(x, y);
        }
    }
    m_buffer.flush();
}

void Simulation::update_particle(int x, int y) {
    Particle &p = m_grid.get(x, y);
    if (p.been_updated)
        return;
    p.been_updated = true;

    switch (p.pt) {
    case ParticleType::Sand:
        update_sand(x, y);
        break;
    case ParticleType::Water:
        update_water(x, y);
        break;
    case ParticleType::Fire:
        update_fire(x, y);
        break;
    default:
        break;
    };
}

void Simulation::update_sand(int x, int y) {
    auto test = [this](int x, int y) {
        return this->test(x, y, ParticleType::None)
            || this->test(x, y, ParticleType::Water);
    };
    Particle &p = m_grid.get(x, y);
    ++p.vel.y; //gravity
    int n = p.vel.y / 16, orig_x = x, orig_y = y;
    while (n--) {
        if (!test(x, y + 1)) {
            p.vel.y -= 2;
            p.vel.y = std::max(p.vel.y, 0);
            break;
        }
        ++y;
    }

    if (test(x, y + 1)) {
        ++y;
    } else if (test(x - 1, y + 1)) {
        --x; ++y;
    } else if (test(x + 1, y + 1)) {
        ++x; ++y;
    }

    if (x != orig_x || y != orig_y)
        swap(x, y, orig_x, orig_y);
}

void Simulation::update_water(int x, int y) {
    bool can_any = false;
    auto test = [this, &can_any](int x, int y) {
        bool result = this->test(x, y, ParticleType::None);
        can_any |= result;
        return result;
    };

    Particle &p = m_grid.get(x, y);
    int orig_x = x, orig_y = y;
    for (int i = 0; i < m_water_spread; ++i) {
        can_any = false;
        if (test(x, y + 1)) {
            ++y;
            ++i;
        } else if (test(x - 1, y + 1) && p.vel.x < 0) {
            ++y;
            --x;
        } else if (test(x + 1, y + 1) && p.vel.x > 0) {
            ++y;
            ++x;
        } else if (test(x - 1, y) && p.vel.x < 0) {
            --x;
        } else if (test(x + 1, y) && p.vel.x > 0) {
            ++x;
        } else if (can_any) {
            p.vel.x *= -1;
        } else {
            break;
        }
    }

    if (orig_x != x || orig_y != y)
        swap(orig_x, orig_y, x, y);
}

void Simulation::update_fire(int x, int y) {
    Particle &p = m_grid.get(x, y);
    std::uniform_real_distribution<float> dist;

    if (exp(FIRE_SPREAD_DELAY * p.lifetime / FIRE_LT_MEAN) 
            >= dist(m_gen))
    {
        std::uniform_int_distribution<size_t> dist(0, 7);
        const V2i offs[8] = { 
            { 1, 0}, {1,-1}, { 0,-1}, 
            {-1,-1},         {-1, 0}, 
            {-1, 1}, {0, 1}, { 1, 1} 
        };

        V2i pos = offs[dist(m_gen)] + V2i(x, y);
        if (test(pos.x, pos.y, ParticleType::Wood)) {
            put_particle(pos.x, pos.y, ParticleType::Fire);
            reset(pos.x, pos.y, false);
        }
    }

    p.lifetime -= FIXED_TIME_STEP;
    bool notify_neighbours = false;
    if (p.lifetime <= sf::Time::Zero) {
        p.pt = ParticleType::None;
        notify_neighbours = true;
    }

    reset(x, y, notify_neighbours);
}

void Simulation::redraw_particle(int x, int y) {
    const Particle &p = m_grid.get(x, y);
    switch (p.pt) {
    case ParticleType::None:
        m_buffer.pixel(x, y) = sf::Color::Black;
        break;
    case ParticleType::Sand:
        m_buffer.pixel(x, y) = sf::Color::Yellow;
        break;
    case ParticleType::Water:
        m_buffer.pixel(x, y) = sf::Color::Blue;
        break;
    case ParticleType::Wood:
        m_buffer.pixel(x, y) = sf::Color(80, 0, 0);
        break;
    case ParticleType::Fire:
    {
        size_t idx = p.lifetime / FLICKER_DURATION;
        m_buffer.pixel(x, y) = FIRE_FLICKER_COLORS[idx % NUM_FIRE_FLICKERS];
        break;
    }
    default:
        break;
    }
}

const sf::Texture& Simulation::get_texture() const {
    return m_buffer.get_texture();
}

void Simulation::spawn_cloud(int cx, int cy, int r, ParticleType pt) {
    auto dist = [=](int x, int y) {
        int dx = x - cx, dy = y - cy;
        return dx * dx + dy * dy;
    };
    Rect rect(cx - r, cy - r, cx + r, cy + r);
    rect = rect.intersection(bounds());

    for (int y = rect.top; y <= rect.bottom; ++y) {
        for (int x = rect.left; x <= rect.right; ++x) {
            if (dist(x, y) <= r*r) {
                put_particle(x, y, pt);
                reset(x, y, true);
            }
        }
    }
}

bool Simulation::is_chunk_dirty(int ch_x, int ch_y) const {
    return m_flags.current->get(ch_x, ch_y).needs_update;
}

bool Simulation::test(int x, int y, ParticleType pt) const {
    return bounds().contains(x, y) && m_grid.get(x, y).pt == pt;
}

Rect Simulation::bounds() const {
    return Rect(0, 0, m_grid.width() - 1, m_grid.height() - 1);
}

void Simulation::put_particle(int x, int y, ParticleType pt) {
    Particle &p = m_grid.get(x, y);
    p.pt = pt;
    p.vel = V2i(-1, 0);

    if (p.pt == ParticleType::Fire) {
        std::uniform_real<float> dist(-1, 1);
        p.lifetime = FIRE_LT_MEAN + dist(m_gen) * FIRE_LT_DEV;
    }
}


void Simulation::swap(int x, int y, int xx, int yy) {
    m_grid.swap(x, y, xx, yy);
    //Optimization: this can be done only once, every chunk
    reset(x, y, true);
    reset(xx, yy, true);
}

void Simulation::reset(int x, int y, bool with_neighbours) {
    int ch_x = x / m_chunk_size, ch_y = y / m_chunk_size;
    auto &next_flags = *m_flags.next;
    next_flags.get(ch_x, ch_y).needs_update = true;
    if (!with_neighbours)
        return;
    int rem_x = x % m_chunk_size, rem_y = y % m_chunk_size;
    if (rem_x && rem_x != m_chunk_size - 1 && rem_y && rem_y != m_chunk_size - 1)
        return;

    int left = (x - 1) / m_chunk_size, right = (x + 1) / m_chunk_size,
        top = (y - 1) / m_chunk_size, bottom = (y + 1) / m_chunk_size;

    if (left >= 0) {
        next_flags.get(left, ch_y).needs_update = true;
        if (top >= 0)
            next_flags.get(left, top).needs_update = true;
        if (bottom < m_chunks_height)
            next_flags.get(left, bottom).needs_update = true;
    }
    if (right < m_chunks_width) {
        next_flags.get(right, ch_y).needs_update = true;
        if (top >= 0)
            next_flags.get(right, top).needs_update = true;
        if (bottom < m_chunks_height)
            next_flags.get(right, bottom).needs_update = true;
    }
    if (top >= 0)
        next_flags.get(ch_x, top).needs_update = true;
    if (bottom < m_chunks_height)
        next_flags.get(ch_x, bottom).needs_update = true;
}

void Simulation::update_row(int start, int end, int ch_y) {
    if (m_update_dir > 0) {
        for (int y = (ch_y + 1) * m_chunk_size - 1; y >= ch_y * m_chunk_size; --y)
            for (int x = start * m_chunk_size; x < (end + 1) * m_chunk_size; ++x)
                update_particle(x, y);
    } else {
        for (int y = (ch_y + 1) * m_chunk_size - 1; y >= ch_y * m_chunk_size; --y)
            for (int x = (end + 1) * m_chunk_size - 1; x >= start * m_chunk_size; --x)
                update_particle(x, y);
    }
}

