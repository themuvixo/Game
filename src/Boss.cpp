#include "Boss.h"
#include "AssetManager.h"
#include "Audio.h"
#include "Level.h"
#include "ParticleSystem.h"
#include "Player.h"
#include "Utils.h"

namespace rayman {

const char* bossName(int world) {
    static const char* n[5] = {"Queen Thornmother", "Maestro Bongobrute", "Neon Sentinel X", "Glacius the Frost Wyrm", "Umbra, the Ember King"};
    return (world >= 1 && world <= 5) ? n[world - 1] : "???";
}

// Attack ids
enum : int {
    kSeedSpray = 0, kRootSpikes, kSummon,          // Thornmother
    kLeapSlam, kNoteVolley,                        // Bongobrute
    kLaserSweep, kHomingOrbs, kDash,               // Sentinel
    kIcicleRain, kFrostBreath, kSlideCharge,       // Glacius
    kFireRain, kLavaWave, kShadowOrbs, kTeleport   // Umbra
};

Boss::Boss(int world, sf::Vector2f spawn, const sf::FloatRect& arena, const AssetManager& assets)
    : m_world(world), m_pos(spawn), m_arena(arena) {
    m_anim.setup(&assets.texture("boss" + toStr(world)), &assets.anim("boss"));
    m_anim.play("idle");
    const int hp[5] = {9, 10, 12, 12, 16};
    m_maxHp = m_hp = hp[std::max(0, std::min(4, world - 1))];
    m_pos.y = floorY();
    if (world == 3) m_pos.y = floorY() - 200.f;
    m_hover = m_pos.y;
}

void Boss::wake() {
    if (m_state == State::Dormant) setState(State::Intro, "telegraph");
}

void Boss::setState(State s, const char* anim) {
    m_state = s;
    m_timer = 0.f;
    m_sub = 0.f;
    m_step = 0;
    if (anim) m_anim.play(anim, true);
}

sf::FloatRect Boss::bodyRect() const {
    const float w = (m_world == 3) ? 150.f : 130.f;
    const float h = (m_world == 3) ? 110.f : 160.f;
    const float yoff = (m_world == 3) ? -45.f : 0.f;
    return sf::FloatRect(m_pos.x - w * 0.5f, m_pos.y - h + yoff, w, h);
}

sf::FloatRect Boss::weakRect() const {
    // the glowing core / open mouth that is exposed during vulnerability
    switch (m_world) {
    case 1: return sf::FloatRect(m_pos.x - 40.f, m_pos.y - 150.f, 80.f, 90.f);
    case 3: return sf::FloatRect(m_pos.x - 45.f, m_pos.y - 140.f, 90.f, 90.f);
    default: return sf::FloatRect(m_pos.x - 60.f, m_pos.y - 160.f, 120.f, 150.f);
    }
}

void Boss::moveTo(sf::Vector2f target, float speed, float dt) {
    const sf::Vector2f d = target - m_pos;
    const float dist = length(d);
    if (dist < 1.f) return;
    m_pos += normalize(d) * std::min(dist, speed * dt);
}

bool Boss::takeHit(int dmg, Audio& audio, ParticleSystem& fx, Level& level) {
    if (!active()) return false;
    if (m_state != State::Vulnerable) {
        audio.play("boss_hit", 0.6f, 0.05f, 1.7f);
        fx.burst(center(), 6, sf::Color(255, 255, 255), sf::Color(200, 200, 255, 0), 200.f, 0.25f, 10.f, 1);
        return false;
    }
    m_hp -= dmg;
    m_flash = 0.2f;
    audio.play("boss_hit");
    fx.sparkle(center(), sf::Color(255, 220, 140), 16);
    level.shake(8.f, 0.25f);
    level.addScore(250 * dmg);
    const int newPhase = (m_hp * 3 <= m_maxHp) ? 3 : ((m_hp * 3 <= m_maxHp * 2) ? 2 : 1);
    if (m_hp <= 0) {
        setState(State::Dying, "death");
        audio.play("boss_roar");
        level.shake(14.f, 1.2f);
        return true;
    }
    if (newPhase != m_phase) {
        m_phase = newPhase;
        audio.play("boss_roar", 1.f, 0.f, 0.9f + 0.1f * static_cast<float>(m_phase));
        level.shake(10.f, 0.6f);
        level.popText(center() + sf::Vector2f(0.f, -80.f), "PHASE " + toStr(m_phase), sf::Color(255, 120, 120));
        setState(State::Recover, "hit");
    } else {
        m_anim.play("hit", true);
        // a second hit window stays open but shortens
        m_timer = std::max(m_timer, 1.2f);
    }
    return true;
}

void Boss::chooseAttack(const Player& player) {
    const float px = player.center().x;
    ++m_attackCount;
    switch (m_world) {
    case 1: {
        const int cycle[3] = {kSeedSpray, kRootSpikes, m_phase >= 3 ? kSummon : kSeedSpray};
        m_attack = cycle[m_attackCount % 3];
        break;
    }
    case 2: m_attack = (m_phase >= 2 && m_attackCount % 3 == 0) ? kNoteVolley : kLeapSlam; break;
    case 3: {
        const int cycle[3] = {kLaserSweep, m_phase >= 2 ? kHomingOrbs : kLaserSweep, kDash};
        m_attack = cycle[m_attackCount % 3];
        break;
    }
    case 4: {
        const int cycle[3] = {kIcicleRain, kFrostBreath, kSlideCharge};
        m_attack = cycle[m_attackCount % 3];
        break;
    }
    default: {
        const int cycle[4] = {kFireRain, kLavaWave, m_phase >= 2 ? kShadowOrbs : kFireRain, kTeleport};
        m_attack = cycle[m_attackCount % 4];
        break;
    }
    }
    m_target = sf::Vector2f(px, floorY());
    m_dir = px < m_pos.x ? -1.f : 1.f;
}

void Boss::runAttack(float dt, Level& level, const Player& player, Audio& audio, ParticleSystem& fx) {
    const float speedMul = 1.f + 0.25f * static_cast<float>(m_phase - 1);
    const sf::Vector2f pc = player.center();
    m_sub += dt;
    switch (m_attack) {
    case kSeedSpray: {
        const int volleys = 2 + m_phase;
        if (m_sub > 0.45f / speedMul && m_step < volleys) {
            m_sub = 0.f;
            ++m_step;
            for (int i = 0; i < 3 + m_phase; ++i) {
                Projectile p;
                p.pos = center() + sf::Vector2f(-40.f, -40.f);
                const float spread = static_cast<float>(i) / static_cast<float>(2 + m_phase);
                p.vel = sf::Vector2f(-150.f - 520.f * spread + globalRng().uniform(-40.f, 40.f), -620.f + globalRng().uniform(-80.f, 80.f));
                if (pc.x > m_pos.x) p.vel.x = -p.vel.x;
                p.gravity = 1100.f;
                p.sprite = 0;
                p.radius = 14.f;
                level.spawnProjectile(p);
            }
            audio.play("shoot", 0.8f, 0.1f, 0.7f);
        }
        if (m_step >= volleys && m_sub > 0.8f) setState(State::Idle, "idle");
        break;
    }
    case kRootSpikes: {
        if (m_step == 0) {
            for (int i = -1; i <= 1; ++i) {
                Hazard h;
                h.kind = 0;
                h.rect = sf::FloatRect(pc.x - 40.f + static_cast<float>(i) * 150.f * static_cast<float>(m_phase), floorY() - 110.f, 80.f, 110.f);
                h.telegraph = 0.9f / speedMul;
                h.active = 0.7f;
                level.spawnHazard(h);
            }
            m_step = 1;
            audio.play("shockwave", 0.6f);
        }
        if (m_sub > 1.8f) setState(State::Vulnerable, "vulnerable");  // bulb opens after the roots
        break;
    }
    case kSummon: {
        if (m_step == 0) {
            level.spawnEnemy(3, sf::Vector2f(m_arena.left + 120.f, floorY()));
            level.spawnEnemy(0, sf::Vector2f(m_arena.left + m_arena.width * 0.5f, floorY()));
            m_step = 1;
            audio.play("boss_roar", 0.7f, 0.f, 1.3f);
        }
        if (m_sub > 1.2f) setState(State::Idle, "idle");
        break;
    }
    case kLeapSlam: {
        // step 0 crouch (telegraph done by state), step 1 airborne, step 2 landed
        if (m_step == 0) {
            m_vel = sf::Vector2f((clampf(pc.x, m_arena.left + 100.f, m_arena.left + m_arena.width - 100.f) - m_pos.x) / 1.0f, -1150.f);
            m_step = 1;
            audio.play("jump", 1.f, 0.f, 0.5f);
        } else if (m_step == 1) {
            m_vel.y += 2300.f * dt;
            m_pos += m_vel * dt;
            if (m_pos.y >= floorY() && m_vel.y > 0.f) {
                m_pos.y = floorY();
                m_step = 2;
                m_sub = 0.f;
                level.shake(12.f, 0.4f);
                audio.play("shockwave");
                fx.dust(m_pos, 24, sf::Color(255, 230, 200, 220));
                for (int d = -1; d <= 1; d += 2) {
                    Projectile p;
                    p.kind = 1;
                    p.pos = sf::Vector2f(m_pos.x + static_cast<float>(d) * 70.f, floorY() - 22.f);
                    p.vel = sf::Vector2f(static_cast<float>(d) * 430.f * speedMul, 0.f);
                    p.radius = 22.f;
                    p.life = 3.f;
                    level.spawnProjectile(p);
                }
            }
        } else if (m_sub > 0.5f) {
            ++m_combo;
            if (m_combo >= 2) { m_combo = 0; setState(State::Vulnerable, "vulnerable"); }
            else setState(State::Idle, "idle");
        }
        break;
    }
    case kNoteVolley: {
        if (m_sub > 0.3f && m_step < 6) {
            m_sub = 0.f;
            ++m_step;
            Projectile p;
            p.pos = center();
            p.vel = normalize(pc - center()) * 330.f * speedMul;
            p.vel.y += static_cast<float>(m_step % 3 - 1) * 90.f;
            p.sprite = 1;
            level.spawnProjectile(p);
            audio.play("shoot", 0.6f, 0.2f, 1.2f);
        }
        if (m_step >= 6 && m_sub > 0.6f) setState(State::Idle, "idle");
        break;
    }
    case kLaserSweep: {
        if (m_step == 0) {
            Hazard h;
            h.kind = 1;
            const float y = (m_attackCount % 2 == 0) ? floorY() - 40.f : floorY() - 150.f;
            h.rect = sf::FloatRect(m_arena.left, y - 14.f, m_arena.width, 28.f);
            h.telegraph = 1.0f / speedMul;
            h.active = 0.6f;
            level.spawnHazard(h);
            if (m_phase >= 3) {
                Hazard h2 = h;
                h2.rect.top = (y > floorY() - 100.f ? floorY() - 250.f : floorY() - 40.f) - 14.f;
                h2.telegraph += 0.9f;
                level.spawnHazard(h2);
            }
            m_step = 1;
            audio.play("laser", 0.6f);
        }
        if (m_sub > 2.2f + (m_phase >= 3 ? 0.9f : 0.f)) setState(State::Idle, "idle");
        break;
    }
    case kHomingOrbs: {
        if (m_sub > 0.5f && m_step < 2 + m_phase) {
            m_sub = 0.f;
            ++m_step;
            Projectile p;
            p.kind = 2;
            p.pos = center();
            p.vel = sf::Vector2f(globalRng().uniform(-200.f, 200.f), -200.f);
            p.sprite = 2;
            p.life = 4.f;
            level.spawnProjectile(p);
            audio.play("laser", 0.4f, 0.2f, 1.5f);
        }
        if (m_step >= 2 + m_phase && m_sub > 1.0f) setState(State::Idle, "idle");
        break;
    }
    case kDash: {
        if (m_step == 0) {
            m_target = sf::Vector2f(m_pos.x < m_arena.left + m_arena.width * 0.5f ? m_arena.left + m_arena.width - 120.f : m_arena.left + 120.f,
                                    floorY() - 60.f);
            m_step = 1;
            audio.play("laser", 0.8f, 0.f, 0.6f);
        } else if (m_step == 1) {
            moveTo(m_target, 900.f * speedMul, dt);
            fx.trail(center(), sf::Color(0, 240, 255, 200));
            if (length(m_target - m_pos) < 4.f) {
                m_step = 2;
                level.shake(10.f, 0.3f);
                audio.play("explosion", 0.7f);
                setState(State::Vulnerable, "vulnerable");  // crashed, core exposed
            }
        }
        break;
    }
    case kIcicleRain: {
        if (m_sub > 0.35f / speedMul && m_step < 4 + m_phase * 2) {
            m_sub = 0.f;
            ++m_step;
            Hazard h;
            h.kind = 2;
            const float x = (m_step % 2 == 0) ? pc.x : globalRng().uniform(m_arena.left + 60.f, m_arena.left + m_arena.width - 60.f);
            h.rect = sf::FloatRect(x - 22.f, m_arena.top + 20.f, 44.f, 70.f);
            h.telegraph = 0.9f;
            h.active = 1.2f;
            level.spawnHazard(h);
        }
        if (m_step >= 4 + m_phase * 2 && m_sub > 1.4f) setState(State::Idle, "idle");
        break;
    }
    case kFrostBreath: {
        if (m_sub > 0.12f && m_step < 10) {
            m_sub = 0.f;
            ++m_step;
            Projectile p;
            p.pos = center() + sf::Vector2f(m_dir * 80.f, 10.f);
            const float ang = (m_dir < 0.f ? kPi : 0.f) + (static_cast<float>(m_step) - 5.f) * 0.06f + 0.15f;
            p.vel = sf::Vector2f(std::cos(ang), std::sin(ang)) * 420.f * speedMul;
            p.sprite = 3;
            p.rot = ang;
            level.spawnProjectile(p);
            audio.play("glide", 0.5f, 0.2f, 1.5f);
        }
        if (m_step >= 10 && m_sub > 0.6f) setState(State::Idle, "idle");
        break;
    }
    case kSlideCharge: {
        if (m_step == 0) {
            m_vel.x = m_dir * 700.f * speedMul;
            m_step = 1;
            audio.play("boss_roar", 0.8f, 0.f, 0.8f);
        } else if (m_step == 1) {
            m_pos.x += m_vel.x * dt;
            fx.dust(m_pos, 1, sf::Color(230, 245, 255, 200));
            if (m_pos.x < m_arena.left + 90.f || m_pos.x > m_arena.left + m_arena.width - 90.f) {
                m_pos.x = clampf(m_pos.x, m_arena.left + 90.f, m_arena.left + m_arena.width - 90.f);
                level.shake(14.f, 0.5f);
                audio.play("explosion", 0.8f);
                for (int i = 0; i < 3; ++i) {
                    Hazard h;
                    h.kind = 2;
                    h.rect = sf::FloatRect(globalRng().uniform(m_arena.left + 60.f, m_arena.left + m_arena.width - 60.f), m_arena.top + 20.f, 44.f, 70.f);
                    h.telegraph = 0.7f;
                    h.active = 1.2f;
                    level.spawnHazard(h);
                }
                setState(State::Vulnerable, "vulnerable");  // dazed against the wall
            }
        }
        break;
    }
    case kFireRain: {
        if (m_sub > 0.22f / speedMul && m_step < 8 + m_phase * 3) {
            m_sub = 0.f;
            ++m_step;
            Projectile p;
            p.pos = sf::Vector2f(globalRng().uniform(m_arena.left + 40.f, m_arena.left + m_arena.width - 40.f), m_arena.top + 10.f);
            if (m_step % 3 == 0) p.pos.x = pc.x;
            p.vel = sf::Vector2f(0.f, 160.f);
            p.gravity = 500.f;
            p.sprite = 4;
            p.radius = 16.f;
            level.spawnProjectile(p);
        }
        if (m_step >= 8 + m_phase * 3 && m_sub > 0.8f) setState(State::Vulnerable, "vulnerable");
        break;
    }
    case kLavaWave: {
        if (m_step == 0) {
            const int waves = m_phase;
            for (int i = 0; i < waves; ++i) {
                Projectile p;
                p.kind = 1;
                const float fromLeft = (i % 2 == 0) == (pc.x > m_pos.x);
                p.pos = sf::Vector2f(fromLeft ? m_arena.left + 30.f : m_arena.left + m_arena.width - 30.f, floorY() - 26.f);
                p.vel = sf::Vector2f((fromLeft ? 1.f : -1.f) * (380.f + 60.f * static_cast<float>(i)), 0.f);
                p.radius = 26.f;
                p.life = 4.f;
                p.sprite = 4;
                level.spawnProjectile(p);
            }
            m_step = 1;
            level.shake(6.f, 0.4f);
            audio.play("shockwave");
        }
        if (m_sub > 1.8f) setState(State::Idle, "idle");
        break;
    }
    case kShadowOrbs: {
        if (m_sub > 0.4f && m_step < 3 + m_phase) {
            m_sub = 0.f;
            ++m_step;
            Projectile p;
            p.kind = 2;
            p.pos = center();
            p.vel = sf::Vector2f(globalRng().uniform(-250.f, 250.f), -250.f);
            p.sprite = 5;
            p.life = 4.5f;
            level.spawnProjectile(p);
        }
        if (m_step >= 3 + m_phase && m_sub > 0.8f) {
            if (m_phase >= 3) level.spawnEnemy(0, sf::Vector2f(m_arena.left + 100.f + globalRng().uniform(0.f, m_arena.width - 200.f), floorY()));
            setState(State::Idle, "idle");
        }
        break;
    }
    case kTeleport: {
        if (m_step == 0) {
            fx.burst(center(), 30, sf::Color(150, 40, 200), sf::Color(20, 0, 40, 0), 300.f, 0.6f, 24.f);
            const float side = pc.x < m_arena.left + m_arena.width * 0.5f ? 0.78f : 0.22f;
            m_pos.x = m_arena.left + m_arena.width * side;
            fx.burst(center(), 30, sf::Color(255, 120, 40), sf::Color(40, 0, 0, 0), 300.f, 0.6f, 24.f);
            audio.play("laser", 0.7f, 0.f, 0.5f);
            m_step = 1;
        }
        if (m_sub > 0.6f) setState(State::Idle, "idle");
        break;
    }
    default: setState(State::Idle, "idle"); break;
    }
}

void Boss::update(float dt, Level& level, const Player& player, Audio& audio, ParticleSystem& fx) {
    m_timer += dt;
    m_flash = std::max(0.f, m_flash - dt);
    m_anim.update(dt);
    const float speedMul = 1.f + 0.25f * static_cast<float>(m_phase - 1);
    switch (m_state) {
    case State::Dormant: return;
    case State::Intro:
        if (m_timer < 0.05f) audio.play("boss_roar");
        if (m_timer > 2.0f) setState(State::Idle, "idle");
        break;
    case State::Idle: {
        // drift / reposition between attacks
        if (m_world == 3) {
            m_hover = floorY() - 220.f;
            const float tx = clampf(player.center().x + std::sin(m_timer * 1.3f) * 160.f, m_arena.left + 120.f, m_arena.left + m_arena.width - 120.f);
            moveTo(sf::Vector2f(tx, m_hover + std::sin(m_timer * 2.f) * 18.f), 220.f, dt);
        } else if (m_world == 5) {
            m_pos.y = floorY() - 20.f - std::abs(std::sin(m_timer * 2.f)) * 30.f;
        } else if (m_world == 4) {
            m_pos.y = floorY();
        }
        m_dir = player.center().x < m_pos.x ? -1.f : 1.f;
        if (m_timer > 1.3f / speedMul) {
            chooseAttack(player);
            setState(State::Telegraph, "telegraph");
            if (m_attack == kLeapSlam || m_attack == kSlideCharge || m_attack == kDash) audio.play("boss_roar", 0.5f, 0.1f, 1.2f);
        }
        break;
    }
    case State::Telegraph:
        if (m_timer > 0.7f / speedMul) setState(State::Attack, "attack");
        break;
    case State::Attack:
        runAttack(dt, level, player, audio, fx);
        break;
    case State::Vulnerable: {
        if (m_world == 3) moveTo(sf::Vector2f(m_pos.x, floorY() - 40.f), 400.f, dt);  // sinks within reach
        if (m_world == 5) m_pos.y = floorY();
        if (static_cast<int>(m_timer * 10.f) % 3 == 0) fx.trail(center() + sf::Vector2f(0.f, -40.f), sf::Color(255, 255, 160));
        const float window = 2.8f - 0.3f * static_cast<float>(m_phase - 1);
        if (m_timer > window) setState(State::Recover, "idle");
        break;
    }
    case State::Recover:
        if (m_world == 2 || m_world == 4) m_pos.y = floorY();
        if (m_timer > 0.8f) setState(State::Idle, "idle");
        break;
    case State::Dying:
        if (static_cast<int>(m_timer * 8.f) != static_cast<int>((m_timer - dt) * 8.f)) {
            fx.explosion(center() + sf::Vector2f(globalRng().uniform(-70.f, 70.f), globalRng().uniform(-70.f, 70.f)), sf::Color(255, 200, 120), 0.8f);
            audio.play("explosion", 0.6f, 0.2f);
        }
        if (m_timer > 2.6f) {
            fx.sparkle(center(), sf::Color(255, 240, 180), 60);
            m_state = State::Dead;
        }
        break;
    case State::Dead: return;
    }
    if (m_world != 3 && m_state != State::Attack && m_state != State::Dying) m_pos.y = std::min(m_pos.y, floorY());
    m_pos.x = clampf(m_pos.x, m_arena.left + 80.f, m_arena.left + m_arena.width - 80.f);
}

void Boss::draw(sf::RenderTarget& t, float time) const {
    if (m_state == State::Dead || m_state == State::Dormant) {
        if (m_state == State::Dormant) {
            m_anim.draw(t, m_pos, m_dir < 0.f, 1.15f, sf::Color(150, 150, 170, 220));
        }
        return;
    }
    sf::Color c = sf::Color::White;
    if (m_flash > 0.f) c = sf::Color(255, 180, 180);
    if (m_state == State::Telegraph && static_cast<int>(time * 14.f) % 2 == 0) c = sf::Color(255, 150, 150);
    if (m_state == State::Vulnerable) {
        // pulsing golden ring telegraphs the weak spot
        const sf::FloatRect w = weakRect();
        sf::CircleShape ring(w.width * 0.6f);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(w.left + w.width * 0.5f, w.top + w.height * 0.5f);
        ring.setFillColor(sf::Color(255, 240, 150, static_cast<sf::Uint8>(40 + 30 * std::sin(time * 10.f))));
        ring.setOutlineThickness(3.f);
        ring.setOutlineColor(sf::Color(255, 240, 150, 180));
        t.draw(ring);
    }
    const float bob = (m_world == 3) ? 0.f : std::sin(time * 3.f) * 2.f;
    m_anim.draw(t, sf::Vector2f(m_pos.x, m_pos.y + 10.f + bob), m_dir < 0.f, 1.15f, c);
}

} // namespace rayman
