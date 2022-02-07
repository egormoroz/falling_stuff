#include "simulation.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <random>
#include "world.hpp"

const uint16_t TIME_STEP_MILLIS = FIXED_TIME_STEP.asMilliseconds();

//in millis
const uint16_t FIRE_LT_MEAN = 3000;
const uint16_t FIRE_LT_DEV = 1000;
const uint16_t FIRE_IGNITE_THRESHOLD = 3000;

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

const uint16_t SAND_FREEFALL_ACC = 2;
const uint16_t MAX_FREEFALL_SPD = UINT16_MAX;

using V2i = sf::Vector2i;

const V2i OFFS[8] = {
    { -1, -1 }, { 0, -1 }, { 1, -1 },
    { -1,  0 }, { 1,  0 },
    { -1,  1 }, { 0,  1 }, { 1,  1 }
};

const size_t VISIBLE_WIDTH = 1024;
const size_t VISIBLE_HEIGHT = 512;

Simulation::Simulation()
    : m_buffer(VISIBLE_WIDTH, VISIBLE_HEIGHT), m_water_spread(8), 
      m_upd_vdir(0), m_upd_hdir(0), m_upd_dir_state(1), m_world(new World()),
      m_view(0, 0, VISIBLE_WIDTH - 1, VISIBLE_HEIGHT - 1),
      m_scheduler(*this, 3)
{
    std::fill(std::begin(m_updated_particles), std::end(m_updated_particles), 0);
    std::fill(std::begin(m_tested_particles), std::end(m_tested_particles), 0);
}

void Simulation::update() {
    switch (m_upd_dir_state) {
    case 1:
        m_upd_hdir = -1;
        m_upd_vdir = -1;
        break;
    case 2:
        m_upd_hdir = 1;
        m_upd_vdir = -1;
        break;
    case 3:
        m_upd_hdir = -1;
        m_upd_vdir = 1;
        break;
    case 4:
        m_upd_hdir = 1;
        m_upd_vdir = 1;
        break;
    default:
        break;
    };
    std::fill(std::begin(m_updated_particles), std::end(m_updated_particles), 0);
    std::fill(std::begin(m_tested_particles), std::end(m_tested_particles), 0);
    m_world->fit_dirty_rects(false);

    auto f = [this](size_t blk_x, size_t blk_y, Block &blk) {
        size_t offx = blk_x * Block::N, offy = blk_y * Block::N;
        for (size_t j = 0; j < Block::N; ++j)
            for (size_t i = 0; i < Block::N; ++i)
                m_scheduler.push_chunk(offx + i, offy + j, &blk.chunks[j][i]);
    };
    m_scheduler.clear();
    m_world->enumerate_blocks(f);

    m_scheduler.run(scheduler::Prepare);
    m_scheduler.run(scheduler::Update);

    std::uniform_int<int8_t> dist(1, 4);
    m_upd_dir_state = dist(m_gens[0]);
}

void Simulation::prepare_chunk(size_t ch_x, size_t ch_y, Chunk &ch, size_t worker_idx) {
    if (!ch.is_dirty())
        return;
    auto bounds = chunk_bounds(ch_x, ch_y).intersection(ch.cur_dirty_rect);
    size_t offx = ch_x * Chunk::SIZE, offy = ch_y * Chunk::SIZE;
    Rect<size_t> r(bounds.left - offx, bounds.top - offy, 
            bounds.right - offx, bounds.bottom - offy);
    for (size_t j = r.top; j <= r.bottom; ++j)
        for (size_t i = r.left; i <= r.right; ++i)
            ch.data[j][i].set_updated<false>();
}

void Simulation::update_chunk(size_t ch_x, size_t ch_y, Chunk &ch, size_t worker_idx) {
    const Rect<int> &r = ch.cur_dirty_rect;
    /* ch.cur_dirty_rect.reset(); */
    /* Rect<int> r = chunk_bounds(ch_x, ch_y); */
    /* auto get_particle = [&ch](size_t x, size_t y) -> Particle& { */
    /*     return ch.data[y % Chunk::SIZE][x % Chunk::SIZE]; */
    /* }; */

    if (m_upd_vdir > 0) {
        if (m_upd_hdir > 0) {
            for (int y = r.top; y <= r.bottom; ++y)
                for (int x = r.left; x <= r.right; ++x)
                    update_particle(x, y, ch, m_world->get(x, y), worker_idx);
        } else {
            for (int y = r.top; y <= r.bottom; ++y)
                for (int x = r.right; x >= r.left; --x)
                    update_particle(x, y, ch, m_world->get(x, y), worker_idx);
        }
    } else {
        if (m_upd_hdir > 0) {
            for (int y = r.bottom; y >= r.top; --y)
                for (int x = r.left; x <= r.right; ++x)
                    update_particle(x, y, ch, m_world->get(x, y), worker_idx);
        } else {
            for (int y = r.bottom; y >= r.top; --y)
                for (int x = r.right; x >= r.left; --x)
                    update_particle(x, y, ch, m_world->get(x, y), worker_idx);
        }
    }
}

void Simulation::render() {
    Rect<size_t> visible_blocks(m_view.left / Block::SIZE, m_view.top / Block::SIZE,
            m_view.right / Block::SIZE, m_view.bottom / Block::SIZE),
        visible_chunks(m_view.left / Chunk::SIZE, m_view.top / Chunk::SIZE,
                m_view.right / Chunk::SIZE, m_view.bottom / Chunk::SIZE);
    auto f = [&](size_t blk_x, size_t blk_y, Block &blk) {
        if (!visible_blocks.contains(blk_x, blk_y))
            return;
        size_t offx = blk_x * Block::N, offy = blk_y * Block::N;
        Rect<size_t> r(offx, offy, offx + Block::N - 1, offy + Block::N - 1);
        r = r.intersection(visible_chunks);

        for (size_t ch_y = r.top; ch_y <= r.bottom; ++ch_y)
            for (size_t ch_x = r.left; ch_x <= r.right; ++ch_x)
                m_scheduler.push_chunk(ch_x, ch_y, &blk.chunks[ch_y - offy][ch_x - offx]);
    };

    m_scheduler.clear();
    m_world->enumerate_blocks(f);
    m_scheduler.run(scheduler::Render);
    m_buffer.flush();
}

void Simulation::render_chunk(size_t ch_x, size_t ch_y, Chunk& ch, size_t worker_idx) {
    auto get_particle = [&ch](size_t x, size_t y) -> Particle& {
        return ch.data[y % Chunk::SIZE][x % Chunk::SIZE];
    };
    Rect<int> r = ch.needs_redrawing;
    if (r.is_empty())
        return;
    ch.needs_redrawing.reset();
    /* Rect<int> r = chunk_bounds(ch_x, ch_y); */
    for (size_t y = r.top; y <= r.bottom; ++y) {
        for (size_t x = r.left; x <= r.right; ++x) {
            redraw_particle(x, y, get_particle(x, y));
        }
    }
}

void Simulation::update_particle(int x, int y, Chunk &ch, Particle &p, size_t worker_idx) {
    ++m_tested_particles[worker_idx];
    if (p.been_updated())
        return;
    /* p.set_updated<true>(); */

    switch (p.type()) {
    case ParticleType::Sand:
        update_particle(x, y, ch, p.as.sand, worker_idx);
        ++m_updated_particles[worker_idx];
        return;
    case ParticleType::Water:
        update_particle(x, y, ch, p.as.water, worker_idx);
        ++m_updated_particles[worker_idx];
        return;
    case ParticleType::Fire:
        p.set_updated<true>();
        update_particle(x, y, ch, p.as.fire, worker_idx);
        ++m_updated_particles[worker_idx];
        return;
    default:
        return;
    };
}

void Simulation::update_particle(int x, int y, Chunk &ch, Sand &p, size_t worker_idx) {
    auto test = [this](int x, int y) {
        const Particle &p = m_world->get(x, y);
        return p.is<None>() || p.is<Water>();
    };

    if (p.vy < MAX_FREEFALL_SPD)
        p.vy += SAND_FREEFALL_ACC; //gravity
    int n = std::max(1, p.vy / 16), orig_x = x, orig_y = y;
    while (n--) {
        if (!m_world->is_particle_loaded(x, y + 1)) {
            p.vy = 0;
            break;
        }

        if (test(x, y + 1)) {
            ++y;
        } else if (x > 0 && test(x - 1, y + 1)) {
            ++y; --x;
        } else if (m_world->is_particle_loaded(x + 1, y + 1) && test(x + 1, y + 1)) {
            ++y; ++x;
        } else if (m_world->get(x, y + 1).is<Sand>()) {
            p.vy = m_world->get(x, y + 1).as.sand.vy;
        } else {
            if (p.vy > SAND_FREEFALL_ACC * 2)
                p.vy -= SAND_FREEFALL_ACC * 2;
            else
                p.vy = 0;
        }
    }

    if (x != orig_x || y != orig_y) {
        m_world->get(orig_x, orig_y).set_updated<true>();
        swap(orig_x, orig_y, x, y);
    }
}

void Simulation::update_particle(int x, int y, Chunk &ch, Water &p, size_t worker_idx) {
    bool can_any = false;
    auto is_none = [this, &can_any](int x, int y) {
        bool result = m_world->get(x, y).is<None>();
        can_any |= result;
        return result;
    };

    int orig_x = x, orig_y = y;
    for (int i = 0; i < m_water_spread; ++i) {
        can_any = false;

        if (m_world->is_particle_loaded(x, y + 1)) {
            if (is_none(x, y + 1)) {
                ++y; ++i;
                continue;
            } 

            if (x > 0 && is_none(x - 1, y + 1) && p.flow_dir < 0) {
                ++y; --x;
                continue;
            }

            if (m_world->is_particle_loaded(x + 1, y + 1) && is_none(x + 1, y + 1) && p.flow_dir > 0) {
                ++y; ++x;
                continue;
            }
        }

        if (x > 0 && is_none(x - 1, y) && p.flow_dir < 0) {
            --x;
        } else if (m_world->is_particle_loaded(x + 1, y) && is_none(x + 1, y) && p.flow_dir > 0) {
            ++x;
        } else if (can_any) {
            p.flow_dir *= -1;
            mark(x, y);
        } else {
            break;
        }
    }

    if (orig_x != x || orig_y != y) {
        m_world->get(orig_x, orig_y).set_updated<true>();
        swap(orig_x, orig_y, x, y);
    }
}

void Simulation::update_particle(int x, int y, Chunk &ch, Fire &p, size_t worker_idx) {
    std::uniform_int_distribution<uint16_t> ignite_roll(0, p.lifetime);
    auto &gen = m_gens[worker_idx];
    if (ignite_roll(gen) < FIRE_IGNITE_THRESHOLD) {
        size_t idx = std::uniform_int_distribution<size_t>(0, 7)(gen);

        V2i pos = OFFS[idx] + V2i(x, y);
        if (pos.x >= 0 && pos.y >= 0 && m_world->is_particle_loaded(pos.x, pos.y) 
                && m_world->get(pos.x, pos.y).is<Wood>())
        {
            Particle q = Particle::create<Fire>();
            std::uniform_int_distribution<uint16_t> dist(0, 2 * FIRE_LT_DEV);
            q.as.fire.lifetime = FIRE_LT_MEAN + dist(gen) - FIRE_LT_DEV;
            m_world->get(pos.x, pos.y) = q;
            mark(pos.x, pos.y);
        }
    }
    
    if (p.lifetime < TIME_STEP_MILLIS) {
        ch.get(x, y) = Particle();
        mark_with_neighbours(x, y);
    } else {
        p.lifetime -= TIME_STEP_MILLIS;
        mark(x, y);
    }
}

void Simulation::redraw_particle(int x, int y, Particle &p) {
    switch (p.type()) {
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
        size_t idx = p.as.fire.lifetime / FLICKER_DURATION.asMilliseconds();
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
    auto distance = [=](int x, int y) {
        int dx = x - cx, dy = y - cy;
        return dx * dx + dy * dy;
    };
    Rect<int> rect(cx - r, cy - r, cx + r, cy + r);
    rect = rect.intersection(static_cast<Rect<int>>(m_view));
    std::uniform_int_distribution<uint16_t> dist(0, FIRE_LT_DEV * 2);

    int st = pt == ParticleType::Water || pt == ParticleType::Sand ? 2 : 1;
    st = 1;
    for (int y = rect.top; y <= rect.bottom; y += st) {
        for (int x = rect.left; x <= rect.right; x += st) {
            if (distance(x, y) <= r*r) {
                Particle p;
                switch (pt) {
                case ParticleType::None:
                    p = Particle::create<None>();
                    break;
                case ParticleType::Sand:
                    p = Particle::create<Sand>();
                    break;
                case ParticleType::Water:
                    p = Particle::create<Water>();
                    break;
                case ParticleType::Wood:
                    p = Particle::create<Wood>();
                    break;
                case ParticleType::Fire:
                    p = Particle::create<Fire>();
                    p.as.fire.lifetime = FIRE_LT_MEAN + dist(m_gens[0]) - FIRE_LT_DEV;
                    break;
                default:
                    break;
                };
                m_world->get(x, y) = p;
                mark_with_neighbours(x, y);
            }
        }
    }
//    m_world->fit_dirty_rects();
}

const Rect<int>& Simulation::chunk_dirty_rect_next(int ch_x, int ch_y) const {
    return m_world->get_chunk(ch_x, ch_y).next_dirty_rect;
}

const Rect<int>& Simulation::chunk_dirty_rect_cur(int ch_x, int ch_y) const {
    return m_world->get_chunk(ch_x, ch_y).cur_dirty_rect;
}

bool Simulation::is_chunk_dirty(int ch_x, int ch_y) const {
    return m_world->get_chunk(ch_x, ch_y).is_dirty();
}

int Simulation::num_updated_particles() const { 
    return std::accumulate(std::begin(m_updated_particles), std::end(m_updated_particles), 0); 
}

int Simulation::num_tested_particles() const { 
    return std::accumulate(std::begin(m_tested_particles), std::end(m_tested_particles), 0); 
}

const std::vector<scheduler::LoadTracker>& Simulation::get_load_stats() const {
    return m_scheduler.load_balance();
}

void Simulation::swap(int x, int y, int xx, int yy) {
    std::swap(m_world->get(x, y), m_world->get(xx, yy));
    mark_with_neighbours(x, y);
    mark_with_neighbours(xx, yy);
}

void Simulation::mark(int x, int y) {
    size_t xx = static_cast<size_t>(x), yy = static_cast<size_t>(y);
    auto &ch = m_world->get_chunk(xx / Chunk::SIZE, yy / Chunk::SIZE);
    ch.next_dirty_rect.include<false>(x, y);
}

void Simulation::mark_with_neighbours(int x, int y) {
    size_t xx = static_cast<size_t>(x), yy = static_cast<size_t>(y);
    auto &ch = m_world->get_chunk(xx / Chunk::SIZE, yy / Chunk::SIZE);
    ch.next_dirty_rect.include<true>(x, y);
}

