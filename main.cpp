#include <SFML/Graphics.hpp>
#include <algorithm>
#include <memory>
#include <cassert>
#include "world.hpp"
#include "perf_tracker.hpp"

const sf::Time FIXED_FRAME_TIME = sf::seconds(1.f / 60);

class Game {
public:
    Game(sf::RenderWindow &window)
        : m_window(window), m_world(new World()) 
    { 
        m_brush.setSize(2.f * V2f(m_brush_size, m_brush_size));
        m_brush.setOutlineColor(sf::Color::White);
        m_brush.setOutlineThickness(1.f);
        m_brush.setFillColor(sf::Color::Transparent);
    }

    void run() {
        m_window.setFramerateLimit(60); //TODO: don't rely on this

        while (m_window.isOpen()) {
            pull_events();

            sf::Time delta = m_clk.restart();
            m_tracker.push(delta.asSeconds() * 1000.f);
            m_delta += delta;
            while (m_delta > FIXED_FRAME_TIME) {
                update();
                m_delta -= FIXED_FRAME_TIME;
            }

            render();
        }
    }

private:
    sf::RenderWindow &m_window;
    std::unique_ptr<World> m_world;
    float m_brush_size = 1;
    sf::RectangleShape m_brush;
    ParticleType m_brush_type;
    sf::Clock m_clk, m_tracker_clock;
    sf::Time m_delta;
    FpsTracker m_tracker;

    void pull_events() {
        sf::Event event;
        while (m_window.pollEvent(event)) {

            if (event.type == sf::Event::Closed) {
                m_window.close();
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                m_brush_size += event.mouseWheelScroll.delta;
                m_brush.setSize(2 * PARTICLE_SIZE * V2f(m_brush_size, m_brush_size));
            } else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Escape:
                    m_window.close();
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
                default:
                    break;
                };
            }
        }

        V2i pos = sf::Mouse::getPosition(m_window);
        float scaled_bsize = m_brush_size * PARTICLE_SIZE;
        m_brush.setPosition(pos.x - scaled_bsize, pos.y - scaled_bsize);

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            int x = std::max(0, int(pos.x / PARTICLE_SIZE)) % SIZE, 
                y = std::max(0, int(pos.y / PARTICLE_SIZE)) % SIZE;
            int R = m_brush_size;
            int lx = std::max(0, x - R), rx = std::min(x + R, SIZE - 1),
                ly = std::max(0, y - R), ry = std::min(y + R, SIZE - 1);
            for (y = ly; y <= ry; ++y) {
                for (x = lx; x <= rx; ++x) {
                    Particle &p = m_world->grid[y][x];
                    p.vel = V2f(0, 1);
                    p.pt = m_brush_type;
                    p.flow_vel = -1.;
                }
            }
        }
    }

    void update() {
        /* m_world->grid[0][SIZE / 2].pt = Water; */
        m_world->update();
    }

    void render() {
        m_window.clear();

        m_world->render();
        m_window.draw(*m_world);
        m_window.draw(m_brush);

        m_window.display();

        if (m_tracker_clock.getElapsedTime().asSeconds() > 0.5f) {
            printf("FPS: %6f, Avg frame time: %6f\n", 
                m_tracker.avg_fps(), m_tracker.avg_frame_time());
            m_tracker_clock.restart();
        }
    }
};


int main() {
    sf::RenderWindow window(sf::VideoMode(SCR_SIZE, SCR_SIZE), "SFML works!");
    Game game(window);
    game.run();
}

