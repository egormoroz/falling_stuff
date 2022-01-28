#include "render_buffer.hpp"
#include <cassert>
#include <algorithm>

RenderBuffer::RenderBuffer(int width, int height) {
    assert(m_texture.create(width, height));
    m_pixels.resize(width * height);
    std::fill(m_pixels.begin(), m_pixels.end(), sf::Color::Black);
}

size_t RenderBuffer::xy2idx(int x, int y) const {
    return y * m_texture.getSize().x + x;
}

void RenderBuffer::clear(const sf::Color &color) {
    std::fill(m_pixels.begin(), m_pixels.end(), color);
}

sf::Color& RenderBuffer::pixel(int x, int y) {
    return m_pixels[xy2idx(x, y)];
}

const sf::Color& RenderBuffer::pixel(int x, int y) const {
    return m_pixels[xy2idx(x, y)];
}

void RenderBuffer::flush() {
    m_texture.update((const sf::Uint8*)m_pixels.data());
}

void RenderBuffer::flush(int x, int y, int xx, int yy) {
    int width = m_texture.getSize().x;
    int height = yy - y + 1;
    m_texture.update((const sf::Uint8*)(&m_pixels[xy2idx(0, y)]),
        width, height, 0, y);
}

const sf::Texture& RenderBuffer::get_texture() const {
    return m_texture;
}

