#pragma once
// Six enemy behaviours, each skinned per world (30 original creatures).
#include <SFML/Graphics.hpp>
#include "Animation.h"

namespace rayman {

class AssetManager;
class Audio;
class Level;
class Player;

enum class EnemyType { Walker = 0, Flyer, Armored, Exploder, Charger, Shooter };
constexpr int kEnemyTypeCount = 6;

const char* enemyTypeKey(EnemyType t);
EnemyType enemyTypeFromChar(char c, bool& ok);

class Enemy {
public:
    enum class State { Idle, Move, Telegraph, Attack, Stunned, Hit, Dying, Dead };
    Enemy(EnemyType type, int world, sf::Vector2f feet, const AssetManager& assets, int id);
    void update(float dt, Level& level, const Player& player, Audio& audio);
    void draw(sf::RenderTarget& t) const;
    sf::FloatRect box() const;
    sf::Vector2f center() const;
    // 0 = deflected, 1 = damaged, 2 = killed
    int hit(int damage, float fromDir, bool charged, Audio& audio);
    bool alive() const { return m_state != State::Dying && m_state != State::Dead; }
    bool removable() const { return m_state == State::Dead; }
    bool stompable() const { return alive() && (m_type == EnemyType::Walker || m_type == EnemyType::Exploder || m_state == State::Stunned); }
    bool harmful() const { return alive() && m_state != State::Stunned; }
    bool consumeExplosion() { const bool e = m_explodeNow; m_explodeNow = false; return e; }
    EnemyType type() const { return m_type; }
    int world() const { return m_world; }
    int id() const { return m_id; }
    int scoreValue() const;
    void kill();
private:
    void setState(State s);
    void updateWalker(float dt, Level& level, sf::Vector2f target);
    void updateFlyer(float dt, Level& level, sf::Vector2f target);
    void updateArmored(float dt, Level& level, sf::Vector2f target);
    void updateExploder(float dt, Level& level, sf::Vector2f target, Audio& audio);
    void updateCharger(float dt, Level& level, sf::Vector2f target, Audio& audio);
    void updateShooter(float dt, Level& level, sf::Vector2f target, Audio& audio);
    void groundMove(float dt, Level& level, float speed, bool turnAtLedge);

    EnemyType m_type;
    int m_world;
    int m_id;
    sf::Vector2f m_pos;   // feet
    sf::Vector2f m_vel;
    sf::Vector2f m_home;
    sf::Vector2f m_size{40.f, 40.f};
    float m_dir = -1.f;
    int m_hp = 1;
    State m_state = State::Move;
    float m_timer = 0.f;
    float m_phase = 0.f;
    float m_flash = 0.f;
    bool m_grounded = false;
    bool m_explodeNow = false;
    Animator m_anim;
};

} // namespace rayman
