#include "Player.h"
#include "AssetManager.h"
#include "Audio.h"
#include "Level.h"
#include "ParticleSystem.h"
#include "Utils.h"

namespace rayman {

void Player::setup(const AssetManager& assets) {
    m_anim.setup(&assets.texture("lumo"), &assets.anim("lumo"));
    m_anim.play("idle");
    m_sparkTex = &assets.texture("particle_star");
}

void Player::spawn(sf::Vector2f feet, bool resetHealth) {
    m_pos = feet;
    m_vel = sf::Vector2f(0.f, 0.f);
    m_state = State::Normal;
    m_iframes = 1.0f;
    m_grounded = false;
    m_gliding = false;
    m_platform = -1;
    m_coyote = m_buffer = 0.f;
    m_chargeTime = 0.f;
    m_chargingFist = false;
    m_attackTimer = 0.f;
    m_facing = 1.f;
    if (resetHealth) m_health = kMaxHealth;
    m_anim.play("idle", true);
}

bool Player::hurt(float sourceX, Audio& audio, ParticleSystem& fx) {
    if (invulnerable()) return false;
    --m_health;
    audio.play("hurt");
    fx.burst(center(), 14, sf::Color(255, 120, 160), sf::Color(255, 255, 255, 0), 260.f, 0.5f, 16.f, 1);
    if (m_health <= 0) {
        kill(audio);
        return true;
    }
    m_state = State::Hurt;
    m_stateTimer = 0.f;
    m_iframes = 1.5f;
    const float dir = (m_pos.x >= sourceX) ? 1.f : -1.f;
    m_vel = sf::Vector2f(dir * 320.f, -480.f);
    m_grounded = false;
    m_gliding = false;
    m_anim.play("hurt", true);
    return true;
}

void Player::kill(Audio& audio) {
    if (!alive()) return;
    m_health = 0;
    m_state = State::Dying;
    m_stateTimer = 0.f;
    m_vel = sf::Vector2f(0.f, -380.f);
    m_anim.play("death", true);
    audio.play("death");
}

void Player::celebrate() {
    m_state = State::Celebrating;
    m_stateTimer = 0.f;
    m_vel.x = 0.f;
}

void Player::physics(float dt, const Input& in, Level& level, Audio& audio, ParticleSystem& fx) {
    const bool controllable = m_state == State::Normal;
    float move = 0.f;
    if (controllable) {
        if (in.held(Action::Left)) move -= 1.f;
        if (in.held(Action::Right)) move += 1.f;
    }
    if (move != 0.f && m_attackTimer <= 0.f) m_facing = move;

    const sf::Vector2f c = center();
    m_inWater = level.waterAt(c);
    const sf::Vector2f wind = level.windAt(c);

    // horizontal
    const float maxSpeed = (in.held(Action::Walk) ? kWalkSpeed : kRunSpeed) * (m_inWater ? 0.6f : 1.f);
    float accel, friction;
    if (m_grounded) {
        accel = m_onIce ? kIceAccel : kGroundAccel;
        friction = m_onIce ? kIceFriction : kGroundFriction;
    } else {
        accel = kAirAccel;
        friction = 700.f;
    }
    if (move != 0.f) {
        const bool turning = (m_vel.x * move) < 0.f;
        m_vel.x = approach(m_vel.x, move * maxSpeed, (turning && !m_onIce ? accel * 1.6f : accel) * dt);
    } else {
        m_vel.x = approach(m_vel.x, 0.f, friction * dt);
    }
    m_vel.x += wind.x * dt;

    // jump input
    if (controllable && in.pressed(Action::Jump)) m_buffer = kJumpBuffer;
    else m_buffer = std::max(0.f, m_buffer - dt);
    m_coyote = m_grounded ? kCoyote : std::max(0.f, m_coyote - dt);

    if (m_inWater) {
        // a stroke near the surface (or holding Jump while rising to it) becomes a leap onto the bank
        const bool nearSurface = !level.waterAt(sf::Vector2f(c.x, c.y - 60.f));
        const bool autoLeap = nearSurface && controllable && in.held(Action::Jump) && m_vel.y < 0.f && !m_jumpHeld;
        if (m_buffer > 0.f || autoLeap) {
            m_vel.y = nearSurface ? -800.f : -360.f;
            m_jumpHeld = nearSurface;
            m_buffer = 0.f;
            audio.play("splash", 0.4f, 0.2f);
            fx.burst(sf::Vector2f(m_pos.x, m_pos.y - 10.f), 6, sf::Color(200, 240, 255, 200), sf::Color(200, 240, 255, 0), 80.f, 0.5f, 10.f);
        }
        if (!in.held(Action::Jump)) m_jumpHeld = false;
        m_vel.y = std::min(m_vel.y + 600.f * dt, 160.f);
        if (m_vel.y < -400.f) m_vel.y += 600.f * dt * 2.f;  // leap decays like a jump
        else m_vel.y *= (1.f - 1.2f * dt);
        m_gliding = false;
        m_canDouble = m_hasDoubleJump;
    } else {
        if (m_buffer > 0.f && m_coyote > 0.f) {
            m_vel.y = -kJumpVel;
            m_buffer = 0.f;
            m_coyote = 0.f;
            m_grounded = false;
            m_jumpHeld = true;
            m_canDouble = m_hasDoubleJump;
            ++m_jumps;
            audio.play("jump", 0.8f, 0.05f);
            fx.dust(m_pos, 6, sf::Color(255, 255, 255, 170));
            m_anim.play("jump", true);
        } else if (m_buffer > 0.f && !m_grounded && m_canDouble) {
            m_vel.y = -kDoubleJumpVel;
            m_buffer = 0.f;
            m_canDouble = false;
            m_jumpHeld = true;
            ++m_jumps;
            audio.play("double_jump", 0.8f);
            fx.burst(m_pos, 16, sf::Color(200, 170, 255), sf::Color(255, 255, 255, 0), 200.f, 0.4f, 14.f, 1);
            m_anim.play("jump", true);
        }
        if (m_jumpHeld && !in.held(Action::Jump)) {
            if (m_vel.y < 0.f) m_vel.y *= kJumpCut;  // variable jump height
            m_jumpHeld = false;
        }
        if (m_vel.y >= 0.f) m_jumpHeld = false;
        // helicopter glide: hold jump while falling
        m_gliding = controllable && !m_grounded && m_vel.y > 0.f && in.held(Action::Jump) && !m_jumpHeld;
        float g = kGravity;
        if (m_gliding) {
            m_vel.y = std::min(m_vel.y + g * dt, kGlideFall);
            m_glideSfx -= dt;
            if (m_glideSfx <= 0.f) { audio.play("glide", 0.25f, 0.1f); m_glideSfx = 0.12f; }
        } else {
            m_vel.y = std::min(m_vel.y + g * dt, kMaxFall);
        }
        // updraft
        if (wind.y < 0.f) {
            m_vel.y += wind.y * dt * (m_gliding ? 1.6f : 0.7f);
            m_vel.y = std::max(m_vel.y, m_gliding ? -380.f : -200.f);
        }
    }

    // ride moving platforms
    if (m_platform >= 0) m_pos += level.platformDelta(m_platform);

    sf::FloatRect b = box();
    const float prevVy = m_vel.y;
    const CollisionResult r = level.moveBody(b, m_vel, dt, false, true);
    m_pos = sf::Vector2f(b.left + b.width * 0.5f, b.top + b.height);
    m_wasGrounded = m_grounded;
    m_grounded = r.ground;
    m_onIce = r.onIce;
    m_platform = r.platform;

    if (!m_wasGrounded && !m_grounded) m_fallPeak = std::max(m_fallPeak, prevVy);
    if (m_grounded && !m_wasGrounded) {
        if (r.spring) {
            m_vel.y = -kSpringVel;
            m_grounded = false;
            m_canDouble = m_hasDoubleJump;
            audio.play("spring");
            fx.sparkle(m_pos, sf::Color(255, 240, 160), 8);
            m_anim.play("jump", true);
        } else if (r.bouncer) {
            m_vel.y = -kBouncerVel;
            m_grounded = false;
            m_canDouble = m_hasDoubleJump;
            audio.play("spring", 1.f, 0.f, 0.8f);
            fx.burst(m_pos, 14, sf::Color(255, 160, 230), sf::Color(255, 255, 255, 0), 220.f, 0.5f, 18.f, 0);
            level.shake(3.f, 0.12f);
            m_anim.play("jump", true);
        } else {
            if (m_fallPeak > 500.f) {
                audio.play("land", 0.7f, 0.1f);
                fx.dust(m_pos, 10, sf::Color(255, 255, 255, 190));
                m_landTimer = 0.15f;
                if (m_state == State::Normal) m_anim.play("land", true);
            } else if (m_fallPeak > 200.f) {
                fx.dust(m_pos, 4, sf::Color(255, 255, 255, 140));
            }
        }
        m_fallPeak = 0.f;
    }
    if (m_grounded && std::abs(m_vel.x) > 200.f) {
        m_dustTimer -= dt;
        if (m_dustTimer <= 0.f) { fx.dust(m_pos, 1, sf::Color(255, 255, 255, 120)); m_dustTimer = 0.18f; }
    }
    if (m_gliding && static_cast<int>(m_stateTimer * 30.f) % 3 == 0) fx.trail(sf::Vector2f(m_pos.x - m_facing * 6.f, m_pos.y - 80.f), sf::Color(190, 220, 255, 160));
}

void Player::update(float dt, const Input& in, Level& level, Audio& audio, ParticleSystem& fx) {
    m_stateTimer += dt;
    m_iframes = std::max(0.f, m_iframes - dt);
    m_landTimer = std::max(0.f, m_landTimer - dt);
    m_attackTimer = std::max(0.f, m_attackTimer - dt);
    m_anim.update(dt);

    switch (m_state) {
    case State::Dying:
        m_vel.y += 900.f * dt;
        m_pos.y += m_vel.y * dt * 0.25f;
        if (m_stateTimer > 1.7f) m_state = State::Dead;
        return;
    case State::Dead:
        return;
    case State::Frozen:
        m_vel = sf::Vector2f(0.f, 0.f);
        m_anim.play("idle");
        return;
    case State::Celebrating:
        m_vel.x = 0.f;
        physics(dt, in, level, audio, fx);
        if (m_grounded && static_cast<int>(m_stateTimer / 0.6f) % 2 == 0 && m_stateTimer < 2.4f) {
            if (m_vel.y >= 0.f) m_vel.y = -520.f;
        }
        m_anim.play(m_grounded ? "idle" : "jump");
        return;
    case State::Hurt:
        if (m_stateTimer > 0.35f) m_state = State::Normal;
        break;
    case State::Normal:
        break;
    }

    // fist: tap to throw, hold to charge
    if (m_state == State::Normal) {
        if (in.pressed(Action::Attack) && level.fistReady()) {
            m_chargingFist = true;
            m_chargeTime = 0.f;
        }
        if (m_chargingFist) {
            m_chargeTime += dt;
            if (m_chargeTime > kChargeTime && static_cast<int>(m_chargeTime * 30.f) % 2 == 0)
                fx.trail(sf::Vector2f(m_pos.x + m_facing * 20.f, m_pos.y - 34.f), sf::Color(255, 230, 140));
            if (!in.held(Action::Attack)) {
                const bool charged = m_chargeTime > kChargeTime;
                level.throwFist(sf::Vector2f(m_pos.x + m_facing * 24.f, m_pos.y - 34.f), m_facing, charged, audio);
                m_chargingFist = false;
                m_chargeTime = 0.f;
                m_attackTimer = 0.25f;
                m_anim.play("attack", true);
            }
        }
    }

    physics(dt, in, level, audio, fx);

    if (level.lavaOrVoid(box())) {
        kill(audio);
        return;
    }
    if (level.spikesHit(box())) {
        if (hurt(m_pos.x - m_facing * 10.f, audio, fx) && alive()) m_vel.y = -620.f;
    }
    chooseAnimation();
}

void Player::chooseAnimation() {
    if (m_state == State::Hurt) return;
    const std::string& cur = m_anim.current();
    if (m_attackTimer > 0.f && cur == "attack" && !m_anim.finished()) return;
    if (m_landTimer > 0.f && cur == "land" && !m_anim.finished()) return;
    if (m_inWater) { m_anim.play("swim"); return; }
    if (!m_grounded) {
        if (m_gliding) m_anim.play("glide");
        else if (m_vel.y < -60.f) { if (cur != "jump") m_anim.play("jump"); }
        else m_anim.play("fall");
        return;
    }
    const float s = std::abs(m_vel.x);
    if (s > 220.f) m_anim.play("run");
    else if (s > 25.f) m_anim.play("walk");
    else m_anim.play("idle");
}

void Player::draw(sf::RenderTarget& t, float time) const {
    if (m_state == State::Dead) return;
    sf::Color c = sf::Color::White;
    if (m_iframes > 0.f && m_state == State::Normal && static_cast<int>(time * 20.f) % 2 == 0) c.a = 90;
    if (m_chargeTime > kChargeTime) {
        const float k = 0.5f + 0.5f * std::sin(time * 30.f);
        c = mixColor(c, sf::Color(255, 240, 150), k * 0.5f);
    }
    // soft glow under Lumo for readability against busy backgrounds
    sf::CircleShape shadow(18.f);
    shadow.setOrigin(18.f, 18.f);
    shadow.setScale(1.f, 0.3f);
    shadow.setPosition(m_pos.x, m_pos.y + 2.f);
    shadow.setFillColor(sf::Color(0, 0, 0, m_grounded ? 70 : 30));
    t.draw(shadow);
    // sprite faces right; frame is 96x96 with feet ~12px above the bottom edge
    m_anim.draw(t, sf::Vector2f(m_pos.x, m_pos.y + 12.f), m_facing < 0.f, 1.0f, c);
}

} // namespace rayman
