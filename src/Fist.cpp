#include "Fist.h"
#include "Utils.h"
#include <algorithm>

namespace rayman {

void Fist::launch(sf::Vector2f from, float dir, bool charged) {
    m_pos = from;
    m_dir = dir >= 0.f ? 1.f : -1.f;
    m_charged = charged;
    m_range = charged ? 470.f : 300.f;
    m_vel = sf::Vector2f(m_dir * (charged ? 1050.f : 900.f), 0.f);
    m_travel = 0.f;
    m_active = true;
    m_returning = false;
    m_hits.clear();
}

bool Fist::update(float dt, sf::Vector2f home) {
    if (!m_active) return false;
    if (!m_returning) {
        const float step = std::abs(m_vel.x) * dt;
        m_pos += m_vel * dt;
        m_travel += step;
        m_vel.x *= (1.f - 0.6f * dt);
        if (m_travel >= m_range) { m_returning = true; m_hits.clear(); }
    } else {
        const sf::Vector2f d = home - m_pos;
        const float dist = length(d);
        const float speed = 1100.f;
        if (dist < 26.f) {
            m_active = false;
            return false;
        }
        m_pos += normalize(d) * std::min(dist, speed * dt);
    }
    return true;
}

bool Fist::tryHit(int targetId) {
    if (std::find(m_hits.begin(), m_hits.end(), targetId) != m_hits.end()) return false;
    m_hits.push_back(targetId);
    return true;
}

void Fist::draw(sf::RenderTarget& t, const sf::Texture& tex, float time) const {
    if (!m_active) return;
    const int frame = static_cast<int>(time * 20.f) % 4;
    sf::Sprite s(tex, sf::IntRect(frame * 32, 0, 32, 32));
    s.setOrigin(16.f, 16.f);
    s.setPosition(m_pos);
    const float sc = m_charged ? 1.45f : 1.15f;
    s.setScale((m_returning ? -m_dir : m_dir) * sc, sc);
    if (m_charged) s.setColor(sf::Color(255, 240, 160));
    t.draw(s);
}

} // namespace rayman
