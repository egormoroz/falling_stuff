#include <SFML/Graphics.hpp>
#include <algorithm>
#include <memory>
#include <cassert>
#include "world.hpp"
#include "perf_tracker.hpp"

class Game {
public:
    Game(sf::RenderWindow &window)
        : m_window(window), m_world(new World()) 
    { 
        window.setView(sf::View(sf::FloatRect(0.f, 0.f, SIZE, SIZE)));
        m_window.setFramerateLimit(60); 

        m_brush.setSize(2.f * V2f(m_brush_size, m_brush_size));
        m_brush.setOutlineColor(sf::Color::White);
        m_brush.setOutlineThickness(1.f);
        m_brush.setFillColor(sf::Color::Transparent);

        m_dirty_rect.setFillColor(sf::Color::Transparent);
        m_dirty_rect.setOutlineColor(sf::Color::Red);
        m_dirty_rect.setOutlineThickness(1.f);
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
    std::unique_ptr<World> m_world;
    float m_brush_size = 1;
    sf::RectangleShape m_dirty_rect;
    sf::RectangleShape m_brush;
    ParticleType m_brush_type;
    sf::Clock m_clk, m_tracker_clock;
    sf::Time m_delta_acc;
    FpsTracker m_tracker;
    bool m_draw_rect;

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
                    m_world->dump_buffer("asdf.png");
                    break;
                case sf::Keyboard::Num0:
                    m_brush_type = None;
                    m_brush.setOutlineColor(sf::Color::White);
                    break;
                case sf::Keyboard::Num1:
                    m_brush.setOutlineColor(sf::Color::Yellow);
                    m_brush_type = Sand;
                    break;
                case sf::Keyboard::Num2:
                    m_brush.setOutlineColor(sf::Color::Blue);
                    m_brush_type = Water;
                    break;
                case sf::Keyboard::Num3:
                    m_brush.setOutlineColor(sf::Color(64, 0, 0));
                    m_brush_type = Wood;
                    break;
                case sf::Keyboard::Num4:
                    m_brush.setOutlineColor(sf::Color(255, 80, 0));
                    m_brush_type = Fire;
                    break;
                default:
                    break;
                };
            }
        }

        V2f pos = m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window));
        m_brush.setPosition(pos - V2f(m_brush_size, m_brush_size));

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

            int x = std::max(0, int(pos.x)) % SIZE, 
                y = std::max(0, int(pos.y)) % SIZE;
            int R = m_brush_size;
            m_world->spawn_cloud(x, y, R, m_brush_type);
            /* int lx = std::max(0, x - R), rx = std::min(x + R, SIZE - 1), */
            /*     ly = std::max(0, y - R), ry = std::min(y + R, SIZE - 1); */
            /* for (y = ly; y <= ry; ++y) { */
            /*     for (x = lx; x <= rx; ++x) { */
            /*         m_world->spawn_particle(x, y, m_brush_type); */
            /*     } */
            /* } */
        }
    }

    void update() {
        m_world->update();
        Rect r = m_world->dirty_rect();
        V2f pos = V2f(r.left, r.top),
            size = V2f(r.right - r.left + 1, r.bottom - r.top + 1);
        m_draw_rect = !r.is_empty();

        m_dirty_rect.setPosition(pos);
        m_dirty_rect.setSize(size);
    }

    void render() {
        m_window.clear();

        m_world->render();
        m_window.draw(*m_world);
        m_window.draw(m_brush);
        if (m_draw_rect)
            m_window.draw(m_dirty_rect);

        m_window.display();

        if (m_tracker_clock.getElapsedTime().asSeconds() 
            >= m_tracker.recorded_period() / 1000.f) 
        {
            printf("FPS: %6f, Avg frame time: %6f, Longest frame time: %6f\n", 
                m_tracker.avg_fps(), m_tracker.avg_frame_time(), m_tracker.longest());
            m_tracker_clock.restart();
        }
    }
};


int main() {
    sf::RenderWindow window(sf::VideoMode(SCR_SIZE, SCR_SIZE), "SFML works!");
    Game game(window);
    game.run();
}

