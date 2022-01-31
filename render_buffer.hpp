#ifndef RENDER_BUFFER_HPP
#define RENDER_BUFFER_HPP

#include <vector>
#include <SFML/Graphics/Texture.hpp>

class RenderBuffer : public sf::NonCopyable {
    sf::Texture m_texture;
    std::vector<sf::Color> m_pixels;

    size_t xy2idx(int x, int y) const;
public:
    RenderBuffer(int width, int height);

    void clear(const sf::Color &color = sf::Color::White);

    sf::Color& pixel(int x, int y);
    const sf::Color& pixel(int x, int y) const;

    void flush();
    void flush(int y, int yy);
    const sf::Texture& get_texture() const;
};

#endif
