#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cassert>
#include "simulation.hpp"
#include "fps_tracker.hpp"
#include "grid_painter.hpp"


class Game {
public:
    Game(sf::RenderWindow &window)
        : m_window(window), m_sim(WIDTH, HEIGHT, CHUNK_SIZE, std::thread::hardware_concurrency()),
          m_view(sf::FloatRect(0.f, 0.f, WIDTH, HEIGHT))
    { 
        m_window.setView(m_view);
        m_window.setFramerateLimit(60); 

        m_brush.setSize(2.f * V2f(m_brush_size, m_brush_size));
        m_brush.setOutlineColor(sf::Color::White);
        m_brush.setOutlineThickness(WIDTH / float(window.getSize().x));
        m_brush.setFillColor(sf::Color::Transparent);

        m_grid.update(WIDTH / BLOCK_SIZE, HEIGHT / BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE);

        m_font.loadFromFile("Courier Prime.ttf");
        m_text.setFont(m_font);
        m_text.setFillColor(sf::Color::White);
        /* m_text.setOutlineColor(sf::Color::White); */
        /* m_text.setOutlineThickness(1.f); */
        m_text.setCharacterSize(m_window.getSize().x / 32);
    }

    void run() {
        while (m_window.isOpen()) {
            pull_events();

            sf::Time delta = m_clk.restart();
            m_tracker.push(delta.asSeconds() * 1000.f);
            m_delta_acc += delta;
            while (m_delta_acc > FIXED_TIME_STEP) {
                update();
                m_delta_acc -= FIXED_TIME_STEP;
            }

            render();
        }
    }

private:
    sf::RenderWindow &m_window;
    Simulation m_sim;

    float m_brush_size = 1;
    sf::RectangleShape m_brush;
    ParticleType m_brush_type = ParticleType::None;

    sf::Clock m_clk, m_tracker_clock;
    sf::Time m_delta_acc;
    FpsTracker m_tracker;

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
                m_brush_size += event.mouseWheelScroll.delta;
                if (m_brush_size < 1)
                    m_brush_size = 1;
                m_brush.setSize(2.f * V2f(m_brush_size, m_brush_size));
            } else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Escape:
                    m_window.close();
                    break;
                case sf::Keyboard::Space:
                    m_draw_grid = !m_draw_grid;
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
        const float SPD = BLOCK_SIZE * 0.5f;
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
        m_grid.clear_selection();
        for (int j = 0; j < HEIGHT / BLOCK_SIZE; ++j)
            for (int i = 0; i < WIDTH / BLOCK_SIZE; ++i)
                if (m_sim.is_block_dirty(i, j))
                    m_grid.add_selection(i, j);
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

        char buf[128];
        sprintf(buf, "Updated particles this frame: %d\n"
                "FPS: %6f\nAvg frame time: %6f\nLongest frame time: %6f" , 
                m_sim.num_updated_particles(), m_tracker.avg_fps(), 
                m_tracker.avg_frame_time(), m_tracker.longest());
        m_text_str = buf;
        m_text.setString(m_text_str);
        m_text.setPosition(m_window.mapPixelToCoords(V2i(0, 0)));

        m_window.draw(m_text);
        m_window.display();
    }
};


int main() {
    sf::RenderWindow window(sf::VideoMode(900, 900), "SFML works!");
    Game game(window);
    game.run();
}

