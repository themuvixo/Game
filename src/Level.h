#pragma once
// Tile-based stage: loading, collision, dynamic objects, collectibles, hazards and interactions.
#include <SFML/Graphics.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "Fist.h"
#include "Utils.h"

namespace rayman {

class AssetManager;
class Audio;
class ParticleSystem;
class Camera;
class Player;
class Enemy;
class Boss;

enum class StageKind { Normal, Challenge, Secret, Boss };

struct LevelInfo {
    std::string name;
    int world = 1, stage = 1;
    StageKind kind = StageKind::Normal;
    std::string music = "world1";
    float par = 120.f;
};

enum class PlatformType { MoveH, MoveV, Falling, Disappearing };
struct Platform {
    PlatformType type = PlatformType::MoveH;
    sf::FloatRect rect;
    sf::Vector2f origin;
    sf::Vector2f delta;
    float phase = 0.f, timer = 0.f;
    int state = 0;      // falling: 0 idle, 1 shaking, 2 falling, 3 respawning
    bool solid = true;
    float alpha = 1.f;
};

enum class PickupType { Ting, Gem, Key, Relic, Heart, Upgrade };
struct Pickup {
    PickupType type = PickupType::Ting;
    sf::Vector2f pos, home;
    bool taken = false;
    float phase = 0.f;
};

struct Cage { sf::Vector2f pos; int hp = 2; bool broken = false; float shake = 0.f; float freed = -1.f; };
struct Checkpoint { sf::Vector2f pos; bool active = false; };
struct SpikeBall { sf::Vector2f pivot; float angle = 0.f; float radius = 96.f; float speed = 1.6f; };
struct SecretZone { sf::FloatRect rect; bool found = false; };

struct Projectile {
    sf::Vector2f pos, vel;
    float gravity = 0.f;
    float life = 5.f;
    int sprite = 0;        // index into projectiles.png
    float radius = 12.f;
    bool alive = true;
    int kind = 0;          // 0 normal, 1 ground shockwave, 2 homing
    float rot = 0.f;
};

struct Hazard {
    sf::FloatRect rect;
    float telegraph = 0.8f;  // seconds before active
    float active = 0.6f;     // seconds active
    int kind = 0;            // 0 root spikes, 1 laser beam, 2 falling icicle, 3 fire pillar
    float t = 0.f;
    bool alive = true;
    bool hitDone = false;
};

struct FloatingText { sf::Vector2f pos; std::string text; float life = 1.f; sf::Color color; };

struct Deco { sf::Vector2f pos; int frame = 0; float phase = 0.f; float scale = 1.f; };

struct RunStats {
    int score = 0;
    int tings = 0, tingsTotal = 0;
    int gems = 0, gemsTotal = 0;
    int cages = 0, cagesTotal = 0;
    int relics = 0, relicsTotal = 0;
    int secrets = 0, secretsTotal = 0;
    int enemies = 0;
    int deaths = 0;
    int damageTaken = 0;
    float time = 0.f;
    bool keyFound = false;
    bool upgradeFound = false;
    bool bossDefeated = false;
    int tingsForLife = 0;
};

struct CollisionResult {
    bool ground = false, wallLeft = false, wallRight = false, ceiling = false;
    bool onIce = false;
    int platform = -1;
    bool spring = false, bouncer = false;
};

struct LevelEvents {
    bool exitReached = false;
    bool checkpoint = false;
    bool secret = false;
    bool key = false;
    bool upgrade = false;
    bool bossDefeated = false;
    bool extraLife = false;
    std::vector<int> enemyKills;  // world*6+type codes
};

class Level {
public:
    Level();
    ~Level();
    bool load(const AssetManager& assets, int world, int stage);
    void restart(int checkpointIndex);
    void onPlayerRespawn();
    const LevelInfo& info() const { return m_info; }

    // physics
    CollisionResult moveBody(sf::FloatRect& box, sf::Vector2f& vel, float dt, bool dropThrough, bool usePlatforms);
    bool solidAt(int tx, int ty) const;
    bool oneWayAt(int tx, int ty) const;
    bool standable(int tx, int ty) const { return solidAt(tx, ty) || oneWayAt(tx, ty); }
    bool solidAtPoint(sf::Vector2f p) const { return solidAt(toTile(p.x), toTile(p.y)); }
    bool waterAt(sf::Vector2f p) const;
    sf::Vector2f windAt(sf::Vector2f p) const;
    bool lavaOrVoid(const sf::FloatRect& box) const;
    bool spikesHit(const sf::FloatRect& box) const;
    sf::Vector2f platformDelta(int idx) const;
    static int toTile(float v) { return static_cast<int>(std::floor(v / kTileF)); }
    sf::FloatRect bounds() const { return sf::FloatRect(0.f, 0.f, m_w * kTileF, m_h * kTileF); }
    int widthTiles() const { return m_w; }
    int heightTiles() const { return m_h; }

    // gameplay
    void update(float dt, Player& player, Audio& audio, ParticleSystem& fx, Camera& cam);
    void throwFist(sf::Vector2f from, float dir, bool charged, Audio& audio);
    bool fistReady() const { return !m_fist.active(); }
    const Fist& fist() const { return m_fist; }
    void spawnProjectile(const Projectile& p) { m_projectiles.push_back(p); }
    void spawnHazard(const Hazard& h) { m_hazards.push_back(h); }
    void spawnEnemy(int type, sf::Vector2f feet);
    void popText(sf::Vector2f p, const std::string& s, sf::Color c);
    void addScore(int s) { m_stats.score += s; }
    void shake(float s, float d) { m_pendingShake = std::max(m_pendingShake, s); m_pendingShakeDur = std::max(m_pendingShakeDur, d); }
    float floorY() const;  // arena floor for bosses

    sf::Vector2f startPos() const { return m_start; }
    sf::Vector2f respawnPos() const;
    int activeCheckpoint() const { return m_activeCheckpoint; }
    RunStats& stats() { return m_stats; }
    const RunStats& stats() const { return m_stats; }
    LevelEvents& events() { return m_events; }
    Boss* boss() { return m_boss.get(); }
    bool arenaLocked() const { return m_arenaLocked; }
    std::optional<sf::FloatRect> arenaRect() const;
    bool exitOpen() const { return m_exitActive; }

    // rendering
    void drawBackground(sf::RenderTarget& t, const Camera& cam, float time) const;
    void drawWorld(sf::RenderTarget& t, const Camera& cam, float time) const;
    void drawForeground(sf::RenderTarget& t, const Camera& cam, float time) const;

private:
    void parse(const std::string& text);
    void buildEntities();
    void rebuildStatic();
    char at(int tx, int ty) const;
    void setAt(int tx, int ty, char c);
    void updatePlatforms(float dt, const Player& player);
    void updatePickups(float dt, Player& player, Audio& audio, ParticleSystem& fx);
    void updateFist(float dt, Player& player, Audio& audio, ParticleSystem& fx);
    void updateEnemies(float dt, Player& player, Audio& audio, ParticleSystem& fx, Camera& cam);
    void updateProjectiles(float dt, Player& player, Audio& audio, ParticleSystem& fx);
    void updateHazards(float dt, Player& player, Audio& audio, ParticleSystem& fx);
    void updateBoss(float dt, Player& player, Audio& audio, ParticleSystem& fx, Camera& cam);
    void killEnemy(Enemy& e, Audio& audio, ParticleSystem& fx);
    void drawTilesDynamic(sf::RenderTarget& t, const sf::FloatRect& view, float time) const;
    sf::IntRect tileRect(int index) const { return sf::IntRect((index % 8) * kTile, (index / 8) * kTile, kTile, kTile); }

    const AssetManager* m_assets = nullptr;
    LevelInfo m_info;
    int m_w = 0, m_h = 0;
    std::vector<char> m_grid, m_original;
    sf::VertexArray m_static;
    std::vector<Platform> m_platforms;
    std::vector<Pickup> m_pickups;
    std::vector<Cage> m_cages;
    std::vector<Checkpoint> m_checkpoints;
    std::vector<SpikeBall> m_spikeballs;
    std::vector<SecretZone> m_secrets;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<Projectile> m_projectiles;
    std::vector<Hazard> m_hazards;
    std::vector<FloatingText> m_texts;
    std::vector<Deco> m_decos;
    std::vector<sf::Vector2i> m_hidden;
    std::unique_ptr<Boss> m_boss;
    sf::Vector2f m_bossSpawn;
    bool m_hasBoss = false;
    int m_arenaLeft = 0, m_arenaRight = 0;
    bool m_arenaLocked = false;
    Fist m_fist;
    sf::Vector2f m_start;
    sf::Vector2f m_exitPos;
    bool m_exitActive = false;
    int m_activeCheckpoint = -1;
    float m_doorTimer = 0.f;
    bool m_staticDirty = true;
    float m_time = 0.f;
    float m_hiddenAlpha = 1.f;
    RunStats m_stats;
    LevelEvents m_events;
    float m_pendingShake = 0.f, m_pendingShakeDur = 0.f;
};

} // namespace rayman
