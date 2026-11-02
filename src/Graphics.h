#pragma once
// Camera, screen transitions, post-processing (with non-shader fallback) and UI drawing helpers.
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include "Utils.h"

namespace rayman {

class AssetManager;

class Camera {
public:
    void reset(sf::Vector2f focus, const sf::FloatRect& worldBounds);
    void setBounds(const sf::FloatRect& b) { m_bounds = b; }
    // Optional fixed framing rectangle (boss arenas); camera eases into it.
    void setFocusRect(std::optional<sf::FloatRect> r) { m_focus = r; }
    void update(float dt, sf::Vector2f target, sf::Vector2f velocity, float facing, bool grounded);
    void shake(float strength, float duration);
    sf::View view() const;
    sf::Vector2f center() const { return m_center; }
    sf::FloatRect visibleRect() const;
private:
    sf::Vector2f clampCenter(sf::Vector2f c) const;
    sf::Vector2f m_center{640.f, 360.f};
    float m_lookAhead = 0.f;
    float m_groundY = 0.f;
    sf::FloatRect m_bounds{0.f, 0.f, 1280.f, 720.f};
    std::optional<sf::FloatRect> m_focus;
    float m_shakeTime = 0.f, m_shakeDur = 0.f, m_shakeStrength = 0.f;
    sf::Vector2f m_shakeOffset;
};

class Fader {
public:
    void fadeOut(float duration) { m_dir = 1; m_speed = 1.f / std::max(0.01f, duration); }
    void fadeIn(float duration) { m_dir = -1; m_speed = 1.f / std::max(0.01f, duration); }
    void set(float a) { m_alpha = a; m_dir = 0; }
    void update(float dt);
    bool fullyOut() const { return m_alpha >= 1.f; }
    bool busy() const { return m_dir != 0; }
    float alpha() const { return m_alpha; }
    void draw(sf::RenderTarget& t) const;
private:
    float m_alpha = 1.f;
    int m_dir = -1;
    float m_speed = 2.f;
};

class PostFX {
public:
    void init(const AssetManager& assets);
    bool shaderActive() const { return m_ok; }
    void setFlash(float f) { m_flash = f; }
    void setTint(sf::Vector3f t) { m_tint = t; }
    void present(sf::RenderTarget& window, const sf::RenderTexture& scene, const sf::FloatRect& viewport, float time,
                 const sf::Texture* vignetteFallback);
private:
    sf::Shader m_shader;
    bool m_ok = false;
    float m_flash = 0.f;
    sf::Vector3f m_tint{1.f, 1.f, 1.f};
};

namespace ui {
enum class Align { Left, Center, Right };
// Draws text with drop shadow and outline. Returns bounds of drawn text.
sf::FloatRect text(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size, sf::Vector2f pos,
                   sf::Color fill = sf::Color::White, Align align = Align::Left, float outline = 2.f,
                   sf::Color outlineColor = sf::Color(30, 16, 50), float alpha = 1.f);
void panel(sf::RenderTarget& t, const sf::Texture& tex, const sf::FloatRect& r, float alpha = 1.f, float border = 24.f);
void icon(sf::RenderTarget& t, const sf::Texture& icons, int index, sf::Vector2f pos, float size = 48.f, float alpha = 1.f, float rotation = 0.f);
void bar(sf::RenderTarget& t, const sf::FloatRect& r, float fill, sf::Color c, sf::Color back = sf::Color(20, 10, 40, 200));
void button(sf::RenderTarget& t, const AssetManager& a, const std::string& label, sf::Vector2f centerPos, bool selected,
            bool enabled, float pulse, float width = 320.f);
bool hit(const sf::FloatRect& r, sf::Vector2f p);
}

// Repeating parallax layer helper: draws a horizontally tiled texture with a given scroll factor.
void drawParallax(sf::RenderTarget& t, const sf::Texture& tex, float cameraX, float cameraY, float factorX, float factorY,
                  float refCameraY, float autoScroll = 0.f, sf::Color color = sf::Color::White);

} // namespace rayman
