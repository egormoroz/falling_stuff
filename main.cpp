#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cassert>
#include "simulation.hpp"
#include "grid_painter.hpp"
#include "world.hpp"
#include "avgtracker.hpp"

using V2f = sf::Vector2f;
using V2i = sf::Vector2i;

const int WIDTH = 1024;
const int HEIGHT = 512;

class Game {
public:
    Game(sf::RenderWindow &window)
        : m_window(window),
          m_view(sf::FloatRect(0.f, 0.f, WIDTH, HEIGHT))
    { 
        m_window.setView(m_view);
        m_window.setFramerateLimit(60); 

        m_brush.setSize(2.f * V2f(m_brush_size, m_brush_size));
        m_brush.setOutlineColor(sf::Color::White);
        m_brush.setOutlineThickness(WIDTH / float(window.getSize().x));
        m_brush.setFillColor(sf::Color::Transparent);

        m_grid.update(WIDTH / Chunk::SIZE, HEIGHT / Chunk::SIZE, Chunk::SIZE, Chunk::SIZE);
        /* m_grid.update(WIDTH / BLOCK_SIZE, HEIGHT / BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE); */

        m_font.loadFromFile("Courier Prime.ttf");
        m_text.setFont(m_font);
        m_text.setFillColor(sf::Color::White);
        /* m_text.setOutlineColor(sf::Color::White); */
        /* m_text.setOutlineThickness(1.f); */
        m_text.setCharacterSize(m_view.getSize().x / 48);
    }

    void run() {
        sf::Clock clk, total_clk;
        while (m_window.isOpen()) {
            pull_events();

            m_totals.push(total_clk.restart());
            /* m_delta_acc += delta; */
            /* while (m_delta_acc > FIXED_TIME_STEP) { */

            clk.restart();
            update();
            m_upd_times.push(clk.restart());

                /* m_delta_acc -= FIXED_TIME_STEP; */
            /* } */

            render();
            m_render_times.push(clk.restart());

        }
    }

private:
    sf::RenderWindow &m_window;
    Simulation m_sim;

    float m_brush_size = 1;
    sf::RectangleShape m_brush;
    ParticleType m_brush_type = ParticleType::None;

    AvgTracker<sf::Time, 64> m_totals;
    AvgTracker<sf::Time, 64> m_upd_times;
    AvgTracker<sf::Time, 64> m_render_times;
    AvgTracker<int, 64> m_upd_particles;

    bool m_draw_grid = true;
    GridPainter m_grid;

    sf::Font m_font;
    sf::Text m_text;
    sf::String m_text_str;

    sf::View m_view;

    void pull_events() {
        sf::Event event;
        while (m_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                m_window.close();
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                float mult = 1.f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                    mult = 10.f;
                m_brush_size += event.mouseWheelScroll.delta * mult;
                if (m_brush_size < 0)
                    m_brush_size = 0;
                m_brush.setSize(2.f * V2f(m_brush_size, m_brush_size));
            } else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Escape:
                    m_window.close();
                    break;
                case sf::Keyboard::Enter:
                    m_draw_grid = !m_draw_grid;
                    break;
                case sf::Keyboard::Space:
                    m_sim.update();
                    m_sim.render();
                    break;
                case sf::Keyboard::Num0:
                    m_brush_type = ParticleType::None;
                    m_brush.setOutlineColor(sf::Color::White);
                    break;
                case sf::Keyboard::Num1:
                    m_brush.setOutlineColor(sf::Color::Yellow);
                    m_brush_type = ParticleType::Sand;
                    break;
                case sf::Keyboard::Num2:
                    m_brush.setOutlineColor(sf::Color::Blue);
                    m_brush_type = ParticleType::Water;
                    break;
                case sf::Keyboard::Num3:
                    m_brush.setOutlineColor(sf::Color(80, 0, 0));
                    m_brush_type = ParticleType::Wood;
                    break;
                case sf::Keyboard::Num4:
                    m_brush.setOutlineColor(sf::Color(255, 200, 0));
                    m_brush_type = ParticleType::Fire;
                    break;
                default:
                    break;
                };
            }
        }

        V2f pos = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window));
        m_brush.setPosition(pos - V2f(m_brush_size, m_brush_size));

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

            int x = std::max(0, int(pos.x)) % WIDTH, 
                y = std::max(0, int(pos.y)) % HEIGHT,
                r = static_cast<int>(m_brush_size);

            m_sim.spawn_cloud(x, y, r, m_brush_type);
        }
    }

    void handle_camera_movement() {
        using Kbd = sf::Keyboard;
        const float SPD = Chunk::SIZE * 0.1f;
        const float ZOOM_SPD = 1.f;
        if (Kbd::isKeyPressed(Kbd::W))
            m_view.move(0.f, -SPD);
        if (Kbd::isKeyPressed(Kbd::S))
            m_view.move(0.f, SPD);
        if (Kbd::isKeyPressed(Kbd::A))
            m_view.move(-SPD, 0.f);
        if (Kbd::isKeyPressed(Kbd::D))
            m_view.move(SPD, 0.f);
        if (Kbd::isKeyPressed(Kbd::Add))
            m_view.zoom(0.9f);
        if (Kbd::isKeyPressed(Kbd::Subtract))
            m_view.zoom(1.1f);
    }

    void update() {
        handle_camera_movement();
        m_sim.update();
    }

    void render() {
        m_window.setView(m_view);
        m_window.clear();

        m_sim.render();
        sf::Sprite sp(m_sim.get_texture());
        m_window.draw(sp);

        m_window.draw(m_brush);
        if (m_draw_grid)
            m_window.draw(m_grid);

        sf::RectangleShape rs;
        rs.setFillColor(sf::Color::Transparent);
        rs.setOutlineThickness(1.f);
        for (int j = 0; j < HEIGHT / Chunk::SIZE; ++j) {
            for (int i = 0; i < WIDTH / Chunk::SIZE; ++i) {
                auto r = m_sim.chunk_dirty_rect_next(i, j);
                rs.setSize(V2f(r.width(), r.height()));
                rs.setPosition(r.left, r.top);
                rs.setOutlineColor(sf::Color::Red);
                m_window.draw(rs);
            }
        }

        char buf[512];
        int n_updated = m_sim.num_updated_particles(), n_tested = m_sim.num_tested_particles();
        float ratio = n_tested ? float(n_updated) / n_tested : 0.f;
        m_upd_particles.push(n_updated);
        float avg_particles = m_upd_particles.average() / m_upd_times.average().asSeconds();
        float fps = 1.f / m_totals.average().asSeconds();

        int n = snprintf(buf, sizeof(buf), "FPS: %6.2f, frame time: %6.2f\n"
                "Avg update time: %6.2fms, avg render time: %6.2fms\n"
                "Updated / tested particles this frame: %dk / %dk = %4.2f\n"
                "Avg updated particles: %6.2fmil/s -- %dk/frame\n"
                "Thread load distribution:\n",
                fps, m_totals.last().asSeconds() * 1000.f,
                m_upd_times.average().asSeconds() * 1e3f, m_render_times.average().asSeconds() * 1e3f,
                n_updated / 1000, n_tested / 1000, ratio,
                avg_particles / 1e6f, static_cast<int>(avg_particles / 1e3f / 60)
                );

        auto &stats = m_sim.get_load_stats();
        for (int i = 0; i < stats.size() && n < sizeof(buf); ++i) {
            n += snprintf(buf + n, sizeof(buf) - n, "%d: %4.2f\n",
                    i + 1, stats[i].average());
        }

        m_text_str = buf;
        m_text.setString(m_text_str);
        m_text.setPosition(m_window.mapPixelToCoords(V2i(0, 0)));

        m_window.draw(m_text);
        m_window.display();
    }
};


int main() {
    const int SCR_WIDTH = 900;
    const float RATIO = float(WIDTH) / float(HEIGHT);
    const int SCR_HEIGHT = static_cast<int>(SCR_WIDTH / RATIO);
    sf::RenderWindow window(sf::VideoMode(SCR_WIDTH, SCR_HEIGHT), "SFML works!");
    Game game(window);
    game.run();
}

