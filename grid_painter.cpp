#include "grid_painter.hpp"
#include <SFML/Graphics/RenderTarget.hpp>

using V2f = sf::Vector2f;

void GridPainter::update(int width, int height, float cell_width, 
        float cell_height, const sf::Color &color) 
{
    m_width = width;
    m_height = height;
    m_cell_width = cell_width;
    m_cell_height = cell_height;

    update_vertices(color);
}

void GridPainter::clear_selection() {
    m_selected.clear();
}

void GridPainter::add_selection(int x, int y, const sf::Color &color) {
    float left = x * m_cell_width, top = y * m_cell_height,
          right = (x + 1) * m_cell_width, bottom = (y + 1) * m_cell_height;
    m_selected.emplace_back(V2f(left, top), color);
    m_selected.emplace_back(V2f(right, top), color);

    m_selected.emplace_back(V2f(right, top), color);
    m_selected.emplace_back(V2f(right, bottom), color);

    m_selected.emplace_back(V2f(right, bottom), color);
    m_selected.emplace_back(V2f(left, bottom), color);

    m_selected.emplace_back(V2f(left, bottom), color);
    m_selected.emplace_back(V2f(left, top), color);
}

void GridPainter::update_vertices(const sf::Color &color) {
    m_vertices.clear();
    for (int i = 0; i <= m_width; ++i) {
        m_vertices.emplace_back(V2f(i * m_cell_width, 0.f), color);
        m_vertices.emplace_back(V2f(i * m_cell_width, m_height * m_cell_height), color);
    }

    for (int i = 0; i <= m_height; ++i) {
        m_vertices.emplace_back(V2f(0.f, i * m_cell_height), color);
        m_vertices.emplace_back(V2f(m_width * m_cell_width, i * m_cell_height), color);
    }
}

void GridPainter::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    target.draw(m_vertices.data(), m_vertices.size(), sf::Lines, states);
    target.draw(m_selected.data(), m_selected.size(), sf::Lines, states);
}

