#pragma once
// Pooled particle system rendered as one textured triangle batch per blend mode.
#include <SFML/Graphics.hpp>
#include <vector>
#include "Utils.h"

namespace rayman {

struct Particle {
    sf::Vector2f pos, vel;
    float life = 0.f, maxLife = 1.f;
    float size0 = 8.f, size1 = 0.f;
    sf::Color c0 = sf::Color::White, c1 = sf::Color(255, 255, 255, 0);
    float rot = 0.f, rotVel = 0.f;
    float gravity = 0.f, drag = 0.f;
    int tex = 0;       // 0 soft dot, 1 star, 2 dust
    bool additive = true;
};

enum class AmbientKind { None, Petals, Notes, Neon, Snow, Embers, MenuDust };

class ParticleSystem {
public:
    ParticleSystem();
    void setTextures(const sf::Texture* soft, const sf::Texture* star, const sf::Texture* dust);
    Particle* spawn();
    void update(float dt);
    void draw(sf::RenderTarget& target) const;
    void clear() { m_count = 0; }
    size_t count() const { return m_count; }

    // presets
    void burst(sf::Vector2f p, int n, sf::Color a, sf::Color b, float speed, float life, float size, int tex = 0, float gravity = 0.f);
    void dust(sf::Vector2f feet, int n, sf::Color c);
    void sparkle(sf::Vector2f p, sf::Color c, int n = 10);
    void trail(sf::Vector2f p, sf::Color c);
    void explosion(sf::Vector2f p, sf::Color c, float scale = 1.f);
    void ambient(AmbientKind kind, const sf::FloatRect& view, float dt);
private:
    std::vector<Particle> m_pool;
    size_t m_count = 0;
    const sf::Texture* m_tex[3] = {nullptr, nullptr, nullptr};
    mutable sf::VertexArray m_va[3][2];
    float m_ambientAcc = 0.f;
};

} // namespace rayman
