#pragma once
// Lumo's magic fist: thrown forward, returns like a boomerang. Charged throws fly further and hit harder.
#include <SFML/Graphics.hpp>
#include <vector>

namespace rayman {

class Fist {
public:
    void launch(sf::Vector2f from, float dir, bool charged);
    // Returns true while active. `home` is the hand position it returns to.
    bool update(float dt, sf::Vector2f home);
    void forceReturn() { m_returning = true; }
    bool active() const { return m_active; }
    bool returning() const { return m_returning; }
    bool charged() const { return m_charged; }
    int damage() const { return m_charged ? 2 : 1; }
    float dir() const { return m_dir; }
    sf::Vector2f pos() const { return m_pos; }
    sf::FloatRect box() const { return sf::FloatRect(m_pos.x - 14.f, m_pos.y - 14.f, 28.f, 28.f); }
    // Each target id is hit at most once per outward and once per return trip.
    bool tryHit(int targetId);
    void draw(sf::RenderTarget& t, const sf::Texture& tex, float time) const;
private:
    sf::Vector2f m_pos, m_vel;
    float m_dir = 1.f;
    float m_travel = 0.f;
    float m_range = 300.f;
    bool m_active = false;
    bool m_returning = false;
    bool m_charged = false;
    std::vector<int> m_hits;
};

} // namespace rayman
