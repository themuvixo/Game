#pragma once
// Lumo's controller: acceleration-based running, variable jump with coyote time and buffering,
// double jump (upgrade), helicopter glide, swimming, wind, magic fist, damage and death.
#include <SFML/Graphics.hpp>
#include "Animation.h"
#include "Input.h"

namespace rayman {

class AssetManager;
class Audio;
class Level;
class ParticleSystem;

class Player {
public:
    // Tunables (mirrored by tools/build_levels.py reachability envelope)
    static constexpr float kRunSpeed = 330.f;
    static constexpr float kWalkSpeed = 150.f;
    static constexpr float kGroundAccel = 2600.f;
    static constexpr float kGroundFriction = 3000.f;
    static constexpr float kIceAccel = 650.f;
    static constexpr float kIceFriction = 260.f;
    static constexpr float kAirAccel = 1900.f;
    static constexpr float kGravity = 2200.f;
    static constexpr float kMaxFall = 900.f;
    static constexpr float kJumpVel = 900.f;
    static constexpr float kDoubleJumpVel = 760.f;
    static constexpr float kJumpCut = 0.45f;
    static constexpr float kCoyote = 0.10f;
    static constexpr float kJumpBuffer = 0.12f;
    static constexpr float kGlideFall = 115.f;
    static constexpr float kSpringVel = 1350.f;
    static constexpr float kBouncerVel = 1500.f;
    static constexpr float kChargeTime = 0.45f;
    static constexpr int kMaxHealth = 3;

    enum class State { Normal, Hurt, Dying, Dead, Celebrating, Frozen };

    void setup(const AssetManager& assets);
    void setDoubleJump(bool v) { m_hasDoubleJump = v; }
    bool hasDoubleJump() const { return m_hasDoubleJump; }
    void spawn(sf::Vector2f feet, bool resetHealth);
    void update(float dt, const Input& in, Level& level, Audio& audio, ParticleSystem& fx);
    void draw(sf::RenderTarget& t, float time) const;

    sf::FloatRect box() const { return sf::FloatRect(m_pos.x - 15.f, m_pos.y - 62.f, 30.f, 62.f); }
    sf::Vector2f feet() const { return m_pos; }
    sf::Vector2f center() const { return sf::Vector2f(m_pos.x, m_pos.y - 31.f); }
    sf::Vector2f velocity() const { return m_vel; }
    float facing() const { return m_facing; }
    bool grounded() const { return m_grounded; }
    bool invulnerable() const { return m_iframes > 0.f || m_state != State::Normal; }
    bool alive() const { return m_state != State::Dying && m_state != State::Dead; }
    bool deathFinished() const { return m_state == State::Dead; }
    bool celebrating() const { return m_state == State::Celebrating; }
    int health() const { return m_health; }
    void heal(int n) { m_health = std::min(kMaxHealth, m_health + n); }
    // Returns true if damage was applied.
    bool hurt(float sourceX, Audio& audio, ParticleSystem& fx);
    void kill(Audio& audio);
    void bounce(float vy) { m_vel.y = -vy; m_grounded = false; m_canDouble = true; m_jumpHeld = false; }
    void celebrate();
    void freeze(bool f) { if (f) m_state = State::Frozen; else if (m_state == State::Frozen) m_state = State::Normal; }
    void pushBack(float vx) { m_vel.x = vx; }
    int jumpsMade() const { return m_jumps; }
    bool charging() const { return m_chargeTime > kChargeTime; }
private:
    void physics(float dt, const Input& in, Level& level, Audio& audio, ParticleSystem& fx);
    void chooseAnimation();

    sf::Vector2f m_pos, m_vel;
    float m_facing = 1.f;
    bool m_grounded = false, m_wasGrounded = false;
    bool m_onIce = false;
    bool m_inWater = false;
    bool m_gliding = false;
    bool m_jumpHeld = false;
    bool m_canDouble = false;
    bool m_hasDoubleJump = false;
    float m_coyote = 0.f, m_buffer = 0.f;
    float m_iframes = 0.f;
    float m_stateTimer = 0.f;
    float m_landTimer = 0.f;
    float m_attackTimer = 0.f;
    float m_chargeTime = 0.f;
    bool m_chargingFist = false;
    float m_dustTimer = 0.f;
    float m_fallPeak = 0.f;
    float m_glideSfx = 0.f;
    int m_platform = -1;
    int m_health = kMaxHealth;
    int m_jumps = 0;
    State m_state = State::Normal;
    Animator m_anim;
    const sf::Texture* m_sparkTex = nullptr;
};

} // namespace rayman
