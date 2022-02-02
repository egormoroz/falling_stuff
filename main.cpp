#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cassert>
#include "simulation.hpp"
#include "fps_tracker.hpp"
#include "grid_painter.hpp"

const int WIDTH = 1024;
const int HEIGHT = 1024;
const int CHUNK_SIZE = 64;
const int BLOCK_SIZE = CHUNK_SIZE / 4;


class Game {
public:
    Game(sf::RenderWindow &window)
        : m_window(window), m_sim(WIDTH, HEIGHT, CHUNK_SIZE, 0/*std::thread::hardware_concurrency()*/)
    { 
        window.setView(sf::View(sf::FloatRect(0.f, 0.f, WIDTH, HEIGHT)));
        m_window.setFramerateLimit(60); 

        m_brush.setSize(2.f * V2f(m_brush_size, m_brush_size));
        m_brush.setOutlineColor(sf::Color::White);
        m_brush.setOutlineThickness(WIDTH / float(window.getSize().x));
        m_brush.setFillColor(sf::Color::Transparent);

        m_grid.update(WIDTH, HEIGHT, BLOCK_SIZE, BLOCK_SIZE);
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

    GridPainter m_grid;


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

    void update() {
        m_sim.update(false);
        m_grid.clear_selection();
        for (int j = 0; j < HEIGHT / BLOCK_SIZE; ++j)
            for (int i = 0; i < WIDTH / BLOCK_SIZE; ++i)
                if (m_sim.is_block_dirty(i, j))
                    m_grid.add_selection(i, j);
    }

    void render() {
        m_window.clear();

        m_sim.render(false);
        sf::Sprite sp(m_sim.get_texture());
        m_window.draw(sp);

        m_window.draw(m_brush);
        m_window.draw(m_grid);

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
    srand(1337);
    sf::RenderWindow window(sf::VideoMode(900, 900), "SFML works!");
    Game game(window);
    game.run();
}

