#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <vector>


class GridPainter : public sf::Drawable {
public:
    GridPainter() = default;

    void update(int width, int height, float cell_width, float cell_height,
            const sf::Color &color = sf::Color::White);

    void clear_selection();
    void add_selection(int x, int y, const sf::Color &color = sf::Color::Red);

private:
    std::vector<sf::Vertex> m_vertices;
    std::vector<sf::Vertex> m_selected;
    int m_width = 0, m_height = 0;
    float m_cell_width = 0.f, m_cell_height = 0.f;

    void update_vertices(const sf::Color &color);

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};

