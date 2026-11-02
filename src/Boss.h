#pragma once
// Five multi-phase world bosses with telegraphed attacks and vulnerability windows.
#include <SFML/Graphics.hpp>
#include <string>
#include "Animation.h"

namespace rayman {

class AssetManager;
class Audio;
class Level;
class Player;
class ParticleSystem;

const char* bossName(int world);

class Boss {
public:
    enum class State { Dormant, Intro, Idle, Telegraph, Attack, Vulnerable, Recover, Dying, Dead };
    Boss(int world, sf::Vector2f spawn, const sf::FloatRect& arena, const AssetManager& assets);
    void wake();
    void update(float dt, Level& level, const Player& player, Audio& audio, ParticleSystem& fx);
    void draw(sf::RenderTarget& t, float time) const;
    // Returns true if the fist dealt damage (false = clank / not hittable)
    bool takeHit(int dmg, Audio& audio, ParticleSystem& fx, Level& level);
    sf::FloatRect bodyRect() const;
    sf::FloatRect weakRect() const;
    bool vulnerable() const { return m_state == State::Vulnerable; }
    bool active() const { return m_state != State::Dormant && m_state != State::Dying && m_state != State::Dead; }
    bool defeated() const { return m_state == State::Dead; }
    bool dying() const { return m_state == State::Dying; }
    bool harmful() const { return active() && m_state != State::Vulnerable && m_state != State::Intro; }
    float healthFrac() const { return static_cast<float>(m_hp) / static_cast<float>(m_maxHp); }
    int phase() const { return m_phase; }
    int world() const { return m_world; }
    sf::Vector2f center() const { return m_pos + sf::Vector2f(0.f, -90.f); }
    std::string name() const { return bossName(m_world); }
private:
    void setState(State s, const char* anim);
    void chooseAttack(const Player& player);
    void runAttack(float dt, Level& level, const Player& player, Audio& audio, ParticleSystem& fx);
    void moveTo(sf::Vector2f target, float speed, float dt);
    float floorY() const { return m_arena.top + m_arena.height; }

    int m_world;
    sf::Vector2f m_pos;      // bottom-center
    sf::Vector2f m_vel;
    sf::FloatRect m_arena;
    State m_state = State::Dormant;
    int m_attack = 0;
    int m_attackCount = 0;
    int m_combo = 0;
    int m_hp = 10, m_maxHp = 10;
    int m_phase = 1;
    float m_timer = 0.f;
    float m_sub = 0.f;
    int m_step = 0;
    float m_dir = -1.f;
    float m_flash = 0.f;
    float m_hover = 0.f;
    sf::Vector2f m_target;
    Animator m_anim;
};

} // namespace rayman
