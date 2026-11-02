#include "Enemy.h"
#include "AssetManager.h"
#include "Audio.h"
#include "Level.h"
#include "Player.h"
#include "Utils.h"

namespace rayman {

const char* enemyTypeKey(EnemyType t) {
    switch (t) {
    case EnemyType::Walker: return "walker";
    case EnemyType::Flyer: return "flyer";
    case EnemyType::Armored: return "armored";
    case EnemyType::Exploder: return "exploder";
    case EnemyType::Charger: return "charger";
    case EnemyType::Shooter: return "shooter";
    }
    return "walker";
}

EnemyType enemyTypeFromChar(char c, bool& ok) {
    ok = true;
    switch (c) {
    case 'a': return EnemyType::Walker;
    case 'b': return EnemyType::Flyer;
    case 'c': return EnemyType::Armored;
    case 'e': return EnemyType::Exploder;
    case 'f': return EnemyType::Charger;
    case 's': return EnemyType::Shooter;
    default: ok = false; return EnemyType::Walker;
    }
}

Enemy::Enemy(EnemyType type, int world, sf::Vector2f feet, const AssetManager& assets, int id)
    : m_type(type), m_world(world), m_id(id), m_pos(feet), m_home(feet) {
    m_anim.setup(&assets.texture("w" + toStr(world) + "_" + enemyTypeKey(type)), &assets.anim("enemy"));
    m_anim.play("move");
    m_phase = globalRng().uniform(0.f, 6.f);
    switch (type) {
    case EnemyType::Walker: m_hp = 1; m_size = {40.f, 40.f}; break;
    case EnemyType::Flyer: m_hp = 1; m_size = {38.f, 32.f}; m_pos.y -= 10.f; m_home = m_pos; break;
    case EnemyType::Armored: m_hp = 3; m_size = {52.f, 40.f}; break;
    case EnemyType::Exploder: m_hp = 1; m_size = {36.f, 38.f}; break;
    case EnemyType::Charger: m_hp = 2; m_size = {56.f, 36.f}; m_state = State::Idle; m_anim.play("idle"); break;
    case EnemyType::Shooter: m_hp = 2; m_size = {40.f, 44.f}; m_state = State::Idle; m_anim.play("idle"); break;
    }
    if (world >= 4 && type != EnemyType::Flyer) m_hp += 1;  // later worlds are tougher
}

int Enemy::scoreValue() const {
    switch (m_type) {
    case EnemyType::Armored: return 300;
    case EnemyType::Charger: return 250;
    case EnemyType::Shooter: return 200;
    default: return 100;
    }
}

sf::FloatRect Enemy::box() const { return sf::FloatRect(m_pos.x - m_size.x * 0.5f, m_pos.y - m_size.y, m_size.x, m_size.y); }
sf::Vector2f Enemy::center() const { return sf::Vector2f(m_pos.x, m_pos.y - m_size.y * 0.5f); }

void Enemy::setState(State s) {
    if (m_state == s) return;
    m_state = s;
    m_timer = 0.f;
    switch (s) {
    case State::Idle: case State::Stunned: m_anim.play("idle"); break;
    case State::Move: m_anim.play("move"); break;
    case State::Telegraph: case State::Attack: m_anim.play("attack", true); break;
    case State::Hit: m_anim.play("hit", true); break;
    case State::Dying: m_anim.play("death", true); break;
    case State::Dead: break;
    }
}

void Enemy::kill() {
    m_hp = 0;
    setState(State::Dying);
}

int Enemy::hit(int damage, float fromDir, bool charged, Audio& audio) {
    if (!alive()) return 0;
    if (m_type == EnemyType::Armored && !charged && m_state != State::Stunned) {
        // shell blocks hits from the front; fist coming from behind (fromDir == facing) gets through
        const bool fromFront = (fromDir > 0.f && m_dir < 0.f) || (fromDir < 0.f && m_dir > 0.f);
        if (fromFront) {
            audio.play("boss_hit", 0.5f, 0.1f, 1.6f);
            m_flash = 0.1f;
            m_vel.x = fromDir * 120.f;
            return 0;
        }
    }
    m_hp -= damage;
    m_flash = 0.15f;
    if (m_hp <= 0) {
        if (m_type == EnemyType::Exploder) m_explodeNow = true;
        kill();
        return 2;
    }
    audio.play("enemy_hit", 0.8f, 0.1f);
    m_vel.x = fromDir * 220.f;
    if (m_type == EnemyType::Flyer) m_vel.y = -120.f;
    if (m_type == EnemyType::Armored) m_dir = -fromDir;  // turns to face the attacker
    setState(State::Hit);
    return 1;
}

void Enemy::groundMove(float dt, Level& level, float speed, bool turnAtLedge) {
    m_vel.x = approach(m_vel.x, m_dir * speed, 1600.f * dt);
    m_vel.y = std::min(m_vel.y + 2000.f * dt, 900.f);
    sf::FloatRect b = box();
    const CollisionResult r = level.moveBody(b, m_vel, dt, false, true);
    m_pos = sf::Vector2f(b.left + b.width * 0.5f, b.top + b.height);
    m_grounded = r.ground;
    if (r.wallLeft && m_dir < 0.f) m_dir = 1.f;
    else if (r.wallRight && m_dir > 0.f) m_dir = -1.f;
    if (turnAtLedge && m_grounded) {
        const int tx = Level::toTile(m_pos.x + m_dir * (m_size.x * 0.5f + 6.f));
        const int ty = Level::toTile(m_pos.y + 4.f);
        if (!level.standable(tx, ty)) m_dir = -m_dir;
    }
}

void Enemy::updateWalker(float dt, Level& level, sf::Vector2f target) {
    const float speed = 60.f + 10.f * static_cast<float>(m_world);
    (void)target;
    groundMove(dt, level, speed, true);
}

void Enemy::updateFlyer(float dt, Level& level, sf::Vector2f target) {
    (void)level;
    m_phase += dt;
    const sf::Vector2f d = target - center();
    if (m_state == State::Attack) {
        // swoop toward the remembered target, then climb back home
        m_timer += 0.f;
        m_pos += m_vel * dt;
        m_vel.y -= 520.f * dt;
        if (m_pos.y < m_home.y && m_vel.y < 0.f) { m_pos.y = m_home.y; m_vel = sf::Vector2f(0.f, 0.f); setState(State::Move); }
        return;
    }
    const float patrol = std::sin(m_phase * 0.8f) * 120.f;
    const float tx = m_home.x + patrol;
    m_dir = tx > m_pos.x ? 1.f : -1.f;
    m_pos.x = lerpf(m_pos.x, tx, 1.f - std::exp(-2.f * dt));
    m_pos.y = m_home.y + std::sin(m_phase * 2.4f) * 12.f;
    if (m_timer > 2.0f && std::abs(d.x) < 260.f && d.y > 30.f && d.y < 320.f) {
        setState(State::Attack);
        const float t = 0.9f;
        m_vel = sf::Vector2f(d.x / t, 520.f * t * 0.5f + d.y / t);
        m_dir = d.x > 0.f ? 1.f : -1.f;
    }
}

void Enemy::updateArmored(float dt, Level& level, sf::Vector2f target) {
    const float d = target.x - m_pos.x;
    if (std::abs(d) < 300.f && std::abs(target.y - m_pos.y) < 120.f && m_timer > 1.5f) {
        m_dir = d > 0.f ? 1.f : -1.f;  // turns slowly to face Lumo
        m_timer = 0.f;
    }
    groundMove(dt, level, 45.f, true);
}

void Enemy::updateExploder(float dt, Level& level, sf::Vector2f target, Audio& audio) {
    const sf::Vector2f d = target - center();
    if (m_state == State::Telegraph) {
        m_vel.x = approach(m_vel.x, 0.f, 800.f * dt);
        sf::FloatRect b = box();
        m_vel.y = std::min(m_vel.y + 2000.f * dt, 900.f);
        level.moveBody(b, m_vel, dt, false, true);
        m_pos = sf::Vector2f(b.left + b.width * 0.5f, b.top + b.height);
        if (m_timer > 0.9f) {
            m_explodeNow = true;
            kill();
        }
        return;
    }
    if (std::abs(d.x) < 300.f && std::abs(d.y) < 100.f) {
        m_dir = d.x > 0.f ? 1.f : -1.f;
        groundMove(dt, level, 150.f, true);
        if (std::abs(d.x) < 70.f) {
            setState(State::Telegraph);
            audio.play("shoot", 0.5f, 0.f, 0.6f);
        }
    } else {
        groundMove(dt, level, 50.f, true);
    }
}

void Enemy::updateCharger(float dt, Level& level, sf::Vector2f target, Audio& audio) {
    const sf::Vector2f d = target - center();
    switch (m_state) {
    case State::Idle:
    case State::Move:
        groundMove(dt, level, 40.f, true);
        if (std::abs(d.x) < 420.f && std::abs(d.y) < 70.f && m_timer > 0.8f) {
            m_dir = d.x > 0.f ? 1.f : -1.f;
            setState(State::Telegraph);
            audio.play("boss_roar", 0.35f, 0.1f, 1.8f);
        }
        break;
    case State::Telegraph:
        m_vel.x = approach(m_vel.x, 0.f, 1200.f * dt);
        groundMove(dt, level, 0.f, false);
        if (m_timer > 0.55f) setState(State::Attack);
        break;
    case State::Attack: {
        const float before = m_dir;
        groundMove(dt, level, 420.f, false);
        const int tx = Level::toTile(m_pos.x + m_dir * 34.f);
        const bool ledge = !level.standable(tx, Level::toTile(m_pos.y + 4.f));
        if (before != m_dir || m_timer > 2.2f || ledge) {
            if (ledge) m_dir = -m_dir;
            m_vel.x = 0.f;
            setState(State::Stunned);
            level.shake(4.f, 0.2f);
        }
        break;
    }
    case State::Stunned:
        groundMove(dt, level, 0.f, false);
        if (m_timer > 1.2f) setState(State::Move);
        break;
    default: break;
    }
}

void Enemy::updateShooter(float dt, Level& level, sf::Vector2f target, Audio& audio) {
    const sf::Vector2f d = target - center();
    m_vel.x = approach(m_vel.x, 0.f, 600.f * dt);
    m_vel.y = std::min(m_vel.y + 2000.f * dt, 900.f);
    sf::FloatRect b = box();
    level.moveBody(b, m_vel, dt, false, true);
    m_pos = sf::Vector2f(b.left + b.width * 0.5f, b.top + b.height);
    if (std::abs(d.x) < 560.f && std::abs(d.y) < 260.f) {
        m_dir = d.x > 0.f ? 1.f : -1.f;
        const float cadence = 2.4f - 0.2f * static_cast<float>(m_world);
        if (m_state != State::Attack && m_timer > cadence) setState(State::Attack);
        if (m_state == State::Attack && m_timer > 0.25f && m_timer - dt <= 0.25f) {
            Projectile p;
            p.pos = center() + sf::Vector2f(m_dir * 28.f, -2.f);
            p.sprite = (m_world == 1) ? 0 : (m_world == 2 ? 1 : (m_world == 3 ? 2 : (m_world == 4 ? 3 : 4)));
            if (m_world == 1) {
                // seeds lob in an arc
                p.vel = sf::Vector2f(clampf(d.x * 1.1f, -420.f, 420.f), -420.f);
                p.gravity = 900.f;
            } else {
                p.vel = normalize(d) * (260.f + 25.f * static_cast<float>(m_world));
            }
            level.spawnProjectile(p);
            audio.play("shoot", 0.6f, 0.15f);
        }
        if (m_state == State::Attack && m_anim.finished()) setState(State::Idle);
    } else if (m_state == State::Attack && m_anim.finished()) {
        setState(State::Idle);
    }
}

void Enemy::update(float dt, Level& level, const Player& player, Audio& audio) {
    m_timer += dt;
    m_flash = std::max(0.f, m_flash - dt);
    m_anim.update(dt);
    if (m_state == State::Dead) return;
    if (m_state == State::Dying) {
        m_vel.y += 900.f * dt;
        m_pos += sf::Vector2f(m_vel.x * 0.3f, m_vel.y * 0.15f) * dt;
        if (m_anim.finished()) m_state = State::Dead;
        return;
    }
    if (m_state == State::Hit) {
        m_vel.x = approach(m_vel.x, 0.f, 900.f * dt);
        if (m_type != EnemyType::Flyer) {
            m_vel.y = std::min(m_vel.y + 2000.f * dt, 900.f);
            sf::FloatRect b = box();
            level.moveBody(b, m_vel, dt, false, true);
            m_pos = sf::Vector2f(b.left + b.width * 0.5f, b.top + b.height);
        } else {
            m_pos += m_vel * dt;
            m_vel.y = approach(m_vel.y, 0.f, 600.f * dt);
        }
        if (m_timer > 0.3f) setState(m_type == EnemyType::Shooter || m_type == EnemyType::Charger ? State::Idle : State::Move);
        return;
    }
    const sf::Vector2f target = player.center();
    switch (m_type) {
    case EnemyType::Walker: updateWalker(dt, level, target); break;
    case EnemyType::Flyer: updateFlyer(dt, level, target); break;
    case EnemyType::Armored: updateArmored(dt, level, target); break;
    case EnemyType::Exploder: updateExploder(dt, level, target, audio); break;
    case EnemyType::Charger: updateCharger(dt, level, target, audio); break;
    case EnemyType::Shooter: updateShooter(dt, level, target, audio); break;
    }
    if (m_pos.y > level.bounds().height + 200.f) m_state = State::Dead;
}

void Enemy::draw(sf::RenderTarget& t) const {
    if (m_state == State::Dead) return;
    sf::Color c = sf::Color::White;
    if (m_flash > 0.f) c = sf::Color(255, 160, 160);
    if (m_state == State::Telegraph && static_cast<int>(m_timer * 16.f) % 2 == 0) c = sf::Color(255, 120, 120);
    if (m_state == State::Stunned) c = sf::Color(200, 200, 255);
    // sprites face right by default
    m_anim.draw(t, sf::Vector2f(m_pos.x, m_pos.y + 4.f), m_dir < 0.f, 1.15f, c);
    if (m_state == State::Stunned) {
        for (int i = 0; i < 3; ++i) {
            const float a = m_timer * 5.f + static_cast<float>(i) * 2.1f;
            sf::CircleShape star(4.f, 4);
            star.setOrigin(4.f, 4.f);
            star.setFillColor(sf::Color(255, 240, 140));
            star.setPosition(m_pos.x + std::cos(a) * 20.f, m_pos.y - m_size.y - 10.f + std::sin(a) * 6.f);
            t.draw(star);
        }
    }
}

} // namespace rayman
