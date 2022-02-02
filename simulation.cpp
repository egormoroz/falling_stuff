#include "simulation.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>

const float FIRE_SPREAD_DELAY = -1.f;

const sf::Time FIRE_LT_MEAN = sf::seconds(3.f);
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

const V2i OFFS[8] = {
    { -1, -1 }, { 0, -1 }, { 1, -1 },
    { -1,  0 }, { 1,  0 },
    { -1,  1 }, { 0,  1 }, { 1,  1 }
};

Simulation::Simulation(int width, int height, int chunk_size, size_t n_threads) 
    : m_update_dir(1), m_block_size(chunk_size / 4), m_chunk_size(chunk_size), 
      m_buffer(width, height), m_grid(width, height),
      m_chunks_width(width / chunk_size), m_chunks_height(height / chunk_size),
      m_water_spread(std::max(1, std::min(width, height) / 64)), m_pool(n_threads), 
      m_partitioner(width, height, chunk_size, chunk_size, chunk_size / 4, chunk_size / 4),
      m_db(width / chunk_size, height / chunk_size)
{
    for (auto &i: m_grid) {
        i.pt = ParticleType::None;
        i.vel.x = -1;
    }

    m_gens.reserve(n_threads);
    std::random_device rd;
    for (size_t i = 0; i < std::max(size_t(1), n_threads); ++i) {
        m_gens.emplace_back(rd());
    }
}

void Simulation::update(bool parallel) {
    std::uniform_int_distribution<> dist(1, 4);
    int off_x = dist(m_gens[0]), off_y = dist(m_gens[0]);
    m_partitioner.shift(off_x, off_y);
    m_db.new_shift(off_x, off_y);
    
    int n = m_partitioner.num_xbounds(), m = m_partitioner.num_ybounds();
    for (int j = 0; j < m - 1; j += 2) {
        for (int i = 0; i < n - 1; i += 2) {
            auto task = [=](size_t worker_idx) { update_region(i, j, worker_idx); };
            if (parallel)
                m_pool.push_task(task);
            else 
                task(0);
        }
    }
    if (parallel)
        m_pool.launch_and_wait();

    for (int j = 0; j < m - 1; j += 2) {
        for (int i = 1; i < n - 1; i += 2) {
            auto task = [=](size_t worker_idx) { update_region(i, j, worker_idx); };
            if (parallel)
                m_pool.push_task(task);
            else 
                task(0);
        }
    }
    if (parallel)
        m_pool.launch_and_wait();

    for (int j = 1; j < m - 1; j += 2) {
        for (int i = 0; i < n - 1; i += 2) {
            auto task = [=](size_t worker_idx) { update_region(i, j, worker_idx); };
            if (parallel)
                m_pool.push_task(task);
            else 
                task(0);
        }
    }
    if (parallel)
        m_pool.launch_and_wait();

    for (int j = 1; j < m - 1; j += 2) {
        for (int i = 1; i < n - 1; i += 2) {
            auto task = [=](size_t worker_idx) { update_region(i, j, worker_idx); };
            if (parallel)
                m_pool.push_task(task);
            else 
                task(0);
        }
    }
    if (parallel)
        m_pool.launch_and_wait();

    m_update_dir *= -1;
}

void Simulation::render(bool parallel) {
    int n = m_partitioner.num_xbounds() - 1, m = m_partitioner.num_ybounds() - 1;
    for (int j = 0; j < m; ++j) {
        for (int i = 0; i < n; ++i) {
            auto task = [=](size_t){ redraw_region(i, j); };
            if (parallel)
                m_pool.push_task(task);
            else
                task(0);
        }
    }
    if (parallel)
        m_pool.launch_and_wait();
    m_buffer.flush();
}

void Simulation::update_particle(int x, int y, size_t worker_idx) {
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
        update_fire(x, y, worker_idx);
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
    p.vel.y += 1; //gravity
    int n = p.vel.y / 16, orig_x = x, orig_y = y;
    while (n--) {
        if (!test(x, y + 1)) {
            if (this->test(x, y + 1, ParticleType::Sand)) {
                p.vel.y = std::min(p.vel.y, m_grid.get(x, y + 1).vel.y);
            } else {
                /* p.vel.y = 0; */
                p.vel.y -= 2;
                p.vel.y = std::max(p.vel.y, 0);
            }
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
        swap(orig_x, orig_y, x, y);
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

void Simulation::update_fire(int x, int y, size_t worker_idx) {
    Particle &p = m_grid.get(x, y);
    std::uniform_real_distribution<float> dist;
    auto &gen = m_gens[worker_idx];

    if (exp(FIRE_SPREAD_DELAY * p.lifetime / FIRE_LT_MEAN) 
            >= dist(gen))
    {
        size_t idx = std::uniform_int_distribution<size_t>(0, 7)(gen);

        V2i pos = OFFS[idx] + V2i(x, y);
        if (test(pos.x, pos.y, ParticleType::Wood)) {
            Particle &q = m_grid.get(pos.x, pos.y);
            q.pt = ParticleType::Fire;
            q.lifetime = FIRE_LT_MEAN + (2 * dist(gen) - 1) * FIRE_LT_DEV;
            mark_pixel(pos.x, pos.y, true);
        }
    }

    p.lifetime -= FIXED_TIME_STEP;
    bool with_neighbours = false;
    if (p.lifetime <= sf::Time::Zero) {
        p.pt = ParticleType::None;
        with_neighbours = true;
    }

    mark_pixel(x, y, with_neighbours);
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
    auto distance = [=](int x, int y) {
        int dx = x - cx, dy = y - cy;
        return dx * dx + dy * dy;
    };
    Rect rect(cx - r, cy - r, cx + r, cy + r);
    rect = rect.intersection(bounds());
    std::uniform_real_distribution<float> dist(-1.f, 1.f);

    for (int y = rect.top; y <= rect.bottom; ++y) {
        for (int x = rect.left; x <= rect.right; ++x) {
            if (distance(x, y) <= r*r) {
                Particle p;
                p.pt = pt;
                p.lifetime = FIRE_LT_MEAN + dist(m_gens[0]) * FIRE_LT_DEV;
                m_grid.get(x, y) = p;
                mark_pixel(x, y, true);
            }
        }
    }
}

bool Simulation::is_block_dirty(int blk_x, int blk_y) const {
    return m_db.test_block<QREDRAW>(blk_x, blk_y);
}

bool Simulation::test(int x, int y, ParticleType pt) const {
    return bounds().contains(x, y) && m_grid.get(x, y).pt == pt;
}

Rect Simulation::bounds() const {
    return Rect(0, 0, m_grid.width() - 1, m_grid.height() - 1);
}

void Simulation::swap(int x, int y, int xx, int yy) {
    m_grid.swap(x, y, xx, yy);
    mark_pixel(x, y, true);
    mark_pixel(xx, yy, true);
}

void Simulation::update_region(int i, int j, size_t worker_idx) {
    if (!m_db.test_region<QUPDATE>(i, j))
        return;
    m_db.reset_region<QUPDATE>(i, j);
    int left = m_partitioner.x_bound(i), top = m_partitioner.y_bound(j),
        right = m_partitioner.x_bound(i + 1) - 1, bottom = m_partitioner.y_bound(j + 1) - 1;

    for (int y = top; y <= bottom; ++y)
        for (int x = left; x <= right; ++x)
            m_grid.get(x, y).been_updated = false;

    if (m_update_dir > 0) {
        for (int y = bottom; y >= top; --y)
            for (int x = left; x <= right; ++x)
                update_particle(x, y, worker_idx);
    } else {
        for (int y = bottom; y >= top; --y)
            for (int x = right; x >= left; --x)
                update_particle(x, y, worker_idx);
    }
}

void Simulation::redraw_region(int i, int j) {
    if (!m_db.test_region<QREDRAW>(i, j))
        return;
    m_db.reset_region<QREDRAW>(i, j);

    int left = m_partitioner.x_bound(i), top = m_partitioner.y_bound(j),
        right = m_partitioner.x_bound(i + 1) - 1, bottom = m_partitioner.y_bound(j + 1) - 1;
    for (int y = top; y <= bottom; ++y)
        for (int x = left; x <= right; ++x)
            redraw_particle(x, y);
}

