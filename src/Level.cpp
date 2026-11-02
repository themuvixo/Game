#include "Level.h"
#include "AssetManager.h"
#include "Audio.h"
#include "Boss.h"
#include "Enemy.h"
#include "Graphics.h"
#include "ParticleSystem.h"
#include "Player.h"
#include <sstream>

namespace rayman {

namespace {
bool isSolidChar(char c) { return c == '#' || c == 'B' || c == 'I'; }
std::uint32_t cellHash(int x, int y) {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(y) * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}
void quad(sf::VertexArray& va, sf::FloatRect r, sf::IntRect uv, sf::Color c = sf::Color::White) {
    const sf::Vector2f p0(r.left, r.top), p1(r.left + r.width, r.top), p2(r.left + r.width, r.top + r.height), p3(r.left, r.top + r.height);
    const sf::Vector2f t0(static_cast<float>(uv.left), static_cast<float>(uv.top));
    const sf::Vector2f t1(static_cast<float>(uv.left + uv.width), static_cast<float>(uv.top));
    const sf::Vector2f t2(static_cast<float>(uv.left + uv.width), static_cast<float>(uv.top + uv.height));
    const sf::Vector2f t3(static_cast<float>(uv.left), static_cast<float>(uv.top + uv.height));
    va.append(sf::Vertex(p0, c, t0)); va.append(sf::Vertex(p1, c, t1)); va.append(sf::Vertex(p2, c, t2));
    va.append(sf::Vertex(p0, c, t0)); va.append(sf::Vertex(p2, c, t2)); va.append(sf::Vertex(p3, c, t3));
}
} // namespace

Level::Level() { m_static.setPrimitiveType(sf::Triangles); }
Level::~Level() = default;

bool Level::load(const AssetManager& assets, int world, int stage) {
    m_assets = &assets;
    const std::string text = assets.textFile("levels/w" + toStr(world) + "_s" + toStr(stage) + ".txt");
    if (text.empty()) {
        logLine("[level] missing level file for world " + toStr(world) + " stage " + toStr(stage));
        return false;
    }
    m_info = LevelInfo();
    m_info.world = world;
    m_info.stage = stage;
    parse(text);
    if (m_w <= 0 || m_h <= 0) return false;
    m_original = m_grid;
    m_stats = RunStats();
    buildEntities();
    return true;
}

void Level::parse(const std::string& text) {
    std::istringstream ss(text);
    std::string line;
    bool grid = false;
    std::vector<std::string> rows;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!grid) {
            if (line == "---") { grid = true; continue; }
            const auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            const std::string k = trim(line.substr(0, eq)), v = trim(line.substr(eq + 1));
            if (k == "name") m_info.name = v;
            else if (k == "music") m_info.music = v;
            else if (k == "par") m_info.par = static_cast<float>(std::atof(v.c_str()));
            else if (k == "kind") {
                if (v == "boss") m_info.kind = StageKind::Boss;
                else if (v == "secret") m_info.kind = StageKind::Secret;
                else if (v == "challenge") m_info.kind = StageKind::Challenge;
                else m_info.kind = StageKind::Normal;
            }
        } else if (!line.empty()) {
            rows.push_back(line);
        }
    }
    m_h = static_cast<int>(rows.size());
    m_w = 0;
    for (const auto& r : rows) m_w = std::max(m_w, static_cast<int>(r.size()));
    m_grid.assign(static_cast<size_t>(m_w * m_h), '.');
    for (int y = 0; y < m_h; ++y)
        for (int x = 0; x < static_cast<int>(rows[static_cast<size_t>(y)].size()); ++x)
            m_grid[static_cast<size_t>(y * m_w + x)] = rows[static_cast<size_t>(y)][static_cast<size_t>(x)];
}

char Level::at(int tx, int ty) const {
    if (tx < 0 || tx >= m_w) return '#';
    if (ty < 0 || ty >= m_h) return '.';
    return m_grid[static_cast<size_t>(ty * m_w + tx)];
}

void Level::setAt(int tx, int ty, char c) {
    if (tx < 0 || tx >= m_w || ty < 0 || ty >= m_h) return;
    m_grid[static_cast<size_t>(ty * m_w + tx)] = c;
    m_staticDirty = true;
}

void Level::buildEntities() {
    m_platforms.clear(); m_pickups.clear(); m_cages.clear(); m_checkpoints.clear(); m_spikeballs.clear();
    m_secrets.clear(); m_enemies.clear(); m_projectiles.clear(); m_hazards.clear(); m_texts.clear(); m_decos.clear();
    m_hidden.clear();
    m_boss.reset();
    m_hasBoss = false;
    m_arenaLocked = false;
    m_exitActive = false;
    m_activeCheckpoint = -1;
    m_doorTimer = 0.f;
    m_events = LevelEvents();
    int enemyId = 0;
    int moversH = 0, moversV = 0;
    for (int y = 0; y < m_h; ++y) {
        for (int x = 0; x < m_w; ++x) {
            const char c = at(x, y);
            const sf::Vector2f cellCenter(x * kTileF + kTileF * 0.5f, y * kTileF + kTileF * 0.5f);
            const sf::Vector2f feet(x * kTileF + kTileF * 0.5f, (y + 1) * kTileF);
            bool clear = true;
            switch (c) {
            case 'P': m_start = feet; break;
            case 'D': {
                Platform p; p.type = PlatformType::Disappearing;
                p.rect = sf::FloatRect(x * kTileF, y * kTileF, kTileF, 20.f);
                p.origin = sf::Vector2f(p.rect.left, p.rect.top);
                p.phase = ((x / 4) % 2) * 1.3f;
                m_platforms.push_back(p);
                break;
            }
            case 'F': {
                Platform p; p.type = PlatformType::Falling;
                p.rect = sf::FloatRect(x * kTileF, y * kTileF, kTileF, 22.f);
                p.origin = sf::Vector2f(p.rect.left, p.rect.top);
                m_platforms.push_back(p);
                break;
            }
            case 'M': case 'V': {
                Platform p; p.type = c == 'M' ? PlatformType::MoveH : PlatformType::MoveV;
                p.rect = sf::FloatRect(x * kTileF, y * kTileF, kTileF * 2.f, 20.f);
                p.origin = sf::Vector2f(p.rect.left, p.rect.top);
                // neighbouring movers run in opposite phase so the gaps between them open and close
                const int n = (c == 'M') ? moversH++ : moversV++;
                p.phase = (n % 2 == 0) ? 0.f : (c == 'M' ? 2.25f : 2.0f);
                m_platforms.push_back(p);
                break;
            }
            case 'o': case 'g': case 'K': case 'r': case 'h': case 'u': {
                Pickup p;
                p.type = c == 'o' ? PickupType::Ting : c == 'g' ? PickupType::Gem : c == 'K' ? PickupType::Key
                       : c == 'r' ? PickupType::Relic : c == 'h' ? PickupType::Heart : PickupType::Upgrade;
                p.pos = p.home = cellCenter;
                p.phase = static_cast<float>(x) * 0.37f;
                if (p.type == PickupType::Ting) ++m_stats.tingsTotal;
                if (p.type == PickupType::Gem) ++m_stats.gemsTotal;
                if (p.type == PickupType::Relic) ++m_stats.relicsTotal;
                m_pickups.push_back(p);
                break;
            }
            case 'E': { Cage cg; cg.pos = feet; m_cages.push_back(cg); ++m_stats.cagesTotal; break; }
            case 'C': { Checkpoint cp; cp.pos = feet; m_checkpoints.push_back(cp); break; }
            case 'X': m_exitPos = feet; m_exitActive = true; break;
            case 'R': {
                SpikeBall s; s.pivot = cellCenter; s.speed = (x % 2 == 0) ? 1.7f : -1.7f; s.angle = static_cast<float>(x);
                m_spikeballs.push_back(s);
                break;
            }
            case '?': {
                SecretZone z; z.rect = sf::FloatRect((x - 1) * kTileF, (y - 1) * kTileF, kTileF * 3.f, kTileF * 2.f);
                m_secrets.push_back(z); ++m_stats.secretsTotal;
                break;
            }
            case 'Z': m_bossSpawn = feet; m_hasBoss = true; break;
            case 'H': m_hidden.push_back(sf::Vector2i(x, y)); clear = false; break;
            default: {
                bool ok = false;
                const EnemyType et = enemyTypeFromChar(c, ok);
                if (ok) {
                    sf::Vector2f f = feet;
                    if (et == EnemyType::Flyer) f.y -= 8.f;
                    m_enemies.push_back(std::make_unique<Enemy>(et, m_info.world, f, *m_assets, enemyId++));
                } else {
                    clear = false;
                }
            }
            }
            if (clear) m_grid[static_cast<size_t>(y * m_w + x)] = '.';
        }
    }
    if (m_hasBoss) {
        const int zx = toTile(m_bossSpawn.x), zy = toTile(m_bossSpawn.y - 1.f);
        int lx = zx, rx = zx;
        while (lx > 0 && !isSolidChar(at(lx, zy))) --lx;
        while (rx < m_w - 1 && !isSolidChar(at(rx, zy))) ++rx;
        m_arenaLeft = lx;
        m_arenaRight = rx;
        m_boss = std::make_unique<Boss>(m_info.world, m_bossSpawn, *arenaRect(), *m_assets);
    }
    // decorations on exposed ground (deterministic)
    for (int x = 2; x < m_w - 2; ++x) {
        for (int y = 1; y < m_h; ++y) {
            if (at(x, y) == '#' && at(x, y - 1) == '.' && at(x, y - 2) == '.' && (cellHash(x, y) % 100) < 11) {
                Deco d;
                d.pos = sf::Vector2f(x * kTileF + kTileF * 0.5f, y * kTileF + 4.f);
                d.frame = static_cast<int>(cellHash(x + 7, y) % 4);
                d.phase = static_cast<float>(cellHash(x, y + 3) % 628) / 100.f;
                d.scale = 0.75f + static_cast<float>(cellHash(x + 1, y) % 50) / 100.f;
                m_decos.push_back(d);
            }
        }
    }
    m_staticDirty = true;
}

void Level::restart(int checkpointIndex) {
    m_grid = m_original;
    m_stats = RunStats();
    buildEntities();
    if (checkpointIndex >= 0 && checkpointIndex < static_cast<int>(m_checkpoints.size())) {
        m_activeCheckpoint = checkpointIndex;
        for (int i = 0; i <= checkpointIndex; ++i) m_checkpoints[static_cast<size_t>(i)].active = true;
    }
}

std::optional<sf::FloatRect> Level::arenaRect() const {
    if (!m_hasBoss) return std::nullopt;
    const float left = (m_arenaLeft + 1) * kTileF;
    const float right = m_arenaRight * kTileF;
    return sf::FloatRect(left, kTileF, right - left, (m_h - 3) * kTileF - kTileF);
}

float Level::floorY() const { return (m_h - 3) * kTileF; }

sf::Vector2f Level::respawnPos() const {
    if (m_activeCheckpoint >= 0 && m_activeCheckpoint < static_cast<int>(m_checkpoints.size()))
        return m_checkpoints[static_cast<size_t>(m_activeCheckpoint)].pos;
    return m_start;
}

bool Level::solidAt(int tx, int ty) const {
    if (tx < 0 || tx >= m_w) return true;
    if (ty < 0 || ty >= m_h) return false;
    const char c = at(tx, ty);
    if (isSolidChar(c)) return true;
    if (c == 'd') return m_doorTimer <= 0.f;
    if (c == 't') return false;
    return false;
}

bool Level::oneWayAt(int tx, int ty) const {
    const char c = at(tx, ty);
    return c == '=' || c == 'S' || c == 'U';
}

bool Level::waterAt(sf::Vector2f p) const { return at(toTile(p.x), toTile(p.y)) == '~'; }

sf::Vector2f Level::windAt(sf::Vector2f p) const {
    switch (at(toTile(p.x), toTile(p.y))) {
    case 'w': return sf::Vector2f(0.f, -2600.f);
    case '<': return sf::Vector2f(-1150.f, 0.f);
    case '>': return sf::Vector2f(1150.f, 0.f);
    default: return sf::Vector2f(0.f, 0.f);
    }
}

bool Level::lavaOrVoid(const sf::FloatRect& b) const {
    if (b.top > m_h * kTileF) return true;
    const int y = toTile(b.top + b.height - 6.f);
    for (int x = toTile(b.left + 4.f); x <= toTile(b.left + b.width - 4.f); ++x)
        if (at(x, y) == 'L') return true;
    return false;
}

bool Level::spikesHit(const sf::FloatRect& b) const {
    const int y0 = toTile(b.top + b.height * 0.3f), y1 = toTile(b.top + b.height - 1.f);
    for (int y = y0; y <= y1; ++y)
        for (int x = toTile(b.left + 6.f); x <= toTile(b.left + b.width - 6.f); ++x)
            if (at(x, y) == '^' && b.top + b.height > y * kTileF + 22.f) return true;
    return false;
}

sf::Vector2f Level::platformDelta(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(m_platforms.size())) return sf::Vector2f(0.f, 0.f);
    return m_platforms[static_cast<size_t>(idx)].delta;
}

CollisionResult Level::moveBody(sf::FloatRect& box, sf::Vector2f& vel, float dt, bool dropThrough, bool usePlatforms) {
    CollisionResult r;
    const float maxStep = 14.f;
    const float travel = std::max(std::abs(vel.x), std::abs(vel.y)) * dt;
    const int steps = std::max(1, static_cast<int>(std::ceil(travel / maxStep)));
    const float sdt = dt / static_cast<float>(steps);
    for (int s = 0; s < steps; ++s) {
        // --- X axis
        if (vel.x != 0.f) {
            box.left += vel.x * sdt;
            const int y0 = toTile(box.top + 1.f), y1 = toTile(box.top + box.height - 1.f);
            if (vel.x > 0.f) {
                const int tx = toTile(box.left + box.width);
                for (int ty = y0; ty <= y1; ++ty) {
                    if (solidAt(tx, ty)) { box.left = tx * kTileF - box.width - 0.01f; vel.x = 0.f; r.wallRight = true; break; }
                }
            } else {
                const int tx = toTile(box.left);
                for (int ty = y0; ty <= y1; ++ty) {
                    if (solidAt(tx, ty)) { box.left = (tx + 1) * kTileF + 0.01f; vel.x = 0.f; r.wallLeft = true; break; }
                }
            }
        }
        // --- Y axis
        const float prevBottom = box.top + box.height;
        box.top += vel.y * sdt;
        const int x0 = toTile(box.left + 1.f), x1 = toTile(box.left + box.width - 1.f);
        if (vel.y > 0.f) {
            const float bottom = box.top + box.height;
            const int ty = toTile(bottom);
            for (int tx = x0; tx <= x1; ++tx) {
                const bool solid = solidAt(tx, ty);
                const bool oneWay = !dropThrough && oneWayAt(tx, ty) && prevBottom <= ty * kTileF + 1.f;
                if (solid || oneWay) {
                    box.top = ty * kTileF - box.height;
                    vel.y = 0.f;
                    r.ground = true;
                    const char c = at(tx, ty);
                    if (c == 'S') r.spring = true;
                    if (c == 'U') r.bouncer = true;
                    if (c == 'I') r.onIce = true;
                    break;
                }
            }
            if (!r.ground && usePlatforms && !dropThrough) {
                for (size_t i = 0; i < m_platforms.size(); ++i) {
                    const Platform& p = m_platforms[i];
                    if (!p.solid) continue;
                    const float pb = box.top + box.height;
                    if (box.left + box.width > p.rect.left + 2.f && box.left < p.rect.left + p.rect.width - 2.f &&
                        prevBottom <= p.rect.top + 2.f + std::max(0.f, -p.delta.y) && pb >= p.rect.top) {
                        box.top = p.rect.top - box.height;
                        vel.y = 0.f;
                        r.ground = true;
                        r.platform = static_cast<int>(i);
                        break;
                    }
                }
            }
        } else if (vel.y < 0.f) {
            const int ty = toTile(box.top);
            for (int tx = x0; tx <= x1; ++tx) {
                if (solidAt(tx, ty)) { box.top = (ty + 1) * kTileF + 0.01f; vel.y = 0.f; r.ceiling = true; break; }
            }
        }
    }
    // standing check (vel.y may have been zeroed this frame)
    if (!r.ground && vel.y >= 0.f) {
        const float bottom = box.top + box.height;
        const int ty = toTile(bottom + 1.f);
        if (std::abs(bottom - ty * kTileF) < 1.5f) {
            for (int tx = toTile(box.left + 1.f); tx <= toTile(box.left + box.width - 1.f); ++tx) {
                if (solidAt(tx, ty) || oneWayAt(tx, ty)) {
                    r.ground = true;
                    if (at(tx, ty) == 'I') r.onIce = true;
                    break;
                }
            }
        }
    }
    return r;
}

void Level::spawnEnemy(int type, sf::Vector2f feet) {
    const int t = std::max(0, std::min(kEnemyTypeCount - 1, type));
    auto e = std::make_unique<Enemy>(static_cast<EnemyType>(t), m_info.world, feet, *m_assets, 1000 + static_cast<int>(m_enemies.size()));
    m_enemies.push_back(std::move(e));
}

void Level::popText(sf::Vector2f p, const std::string& s, sf::Color c) {
    FloatingText t;
    t.pos = p; t.text = s; t.color = c; t.life = 1.2f;
    m_texts.push_back(t);
}

void Level::throwFist(sf::Vector2f from, float dir, bool charged, Audio& audio) {
    if (m_fist.active()) return;
    m_fist.launch(from, dir, charged);
    audio.play("attack", charged ? 1.f : 0.8f, 0.08f, charged ? 0.8f : 1.f);
}

// ============================================================== update
void Level::updatePlatforms(float dt, const Player& player) {
    const sf::FloatRect pb = player.box();
    for (auto& p : m_platforms) {
        const sf::Vector2f before(p.rect.left, p.rect.top);
        switch (p.type) {
        case PlatformType::MoveH:
            p.phase += dt;
            p.rect.left = p.origin.x + std::sin(p.phase * (2.f * kPi / 4.5f)) * kTileF * 3.f;
            break;
        case PlatformType::MoveV:
            p.phase += dt;
            p.rect.top = p.origin.y + std::sin(p.phase * (2.f * kPi / 4.f)) * kTileF * 3.f;
            break;
        case PlatformType::Disappearing: {
            const float cycle = 2.6f;
            const float t = std::fmod(m_time + p.phase, cycle);
            p.solid = t < 1.7f;
            p.alpha = t < 1.3f ? 1.f : (t < 1.7f ? (static_cast<int>(t * 20.f) % 2 ? 0.35f : 0.9f) : 0.12f);
            break;
        }
        case PlatformType::Falling: {
            const bool standing = pb.left + pb.width > p.rect.left && pb.left < p.rect.left + p.rect.width &&
                                  std::abs(pb.top + pb.height - p.rect.top) < 3.f && player.grounded();
            if (p.state == 0 && standing) { p.state = 1; p.timer = 0.f; }
            p.timer += dt;
            if (p.state == 1) {
                p.rect.left = p.origin.x + std::sin(p.timer * 60.f) * 2.f;
                if (p.timer > 0.45f) { p.state = 2; p.timer = 0.f; p.rect.left = p.origin.x; }
            } else if (p.state == 2) {
                p.rect.top += (200.f + p.timer * 1400.f) * dt;
                if (p.timer > 1.2f) { p.state = 3; p.timer = 0.f; p.solid = false; p.alpha = 0.f; }
            } else if (p.state == 3 && p.timer > 2.5f) {
                p.state = 0; p.solid = true; p.alpha = 1.f; p.rect.top = p.origin.y; p.rect.left = p.origin.x;
            }
            if (p.state == 3 && p.timer > 2.0f) p.alpha = (p.timer - 2.0f) * 2.f;
            break;
        }
        }
        p.delta = sf::Vector2f(p.rect.left, p.rect.top) - before;
    }
}

void Level::updatePickups(float dt, Player& player, Audio& audio, ParticleSystem& fx) {
    const sf::FloatRect pb = player.box();
    const sf::Vector2f pc = player.center();
    for (auto& p : m_pickups) {
        if (p.taken) continue;
        p.phase += dt;
        p.pos.y = p.home.y + std::sin(p.phase * 3.f) * 4.f;
        // gentle magnet for tings near Lumo
        if (p.type == PickupType::Ting) {
            const sf::Vector2f d = pc - p.pos;
            if (length(d) < 70.f) { p.home += normalize(d) * 260.f * dt; p.pos = p.home; }
        }
        const float r = p.type == PickupType::Ting ? 14.f : 20.f;
        const sf::FloatRect pr(p.pos.x - r, p.pos.y - r, r * 2.f, r * 2.f);
        if (!player.alive() || !pr.intersects(pb)) continue;
        p.taken = true;
        switch (p.type) {
        case PickupType::Ting:
            ++m_stats.tings; ++m_stats.tingsForLife; m_stats.score += 10;
            audio.play("ting", 0.6f, 0.04f, 1.f + 0.02f * static_cast<float>(m_stats.tingsForLife % 10));
            fx.sparkle(p.pos, sf::Color(255, 220, 120), 6);
            if (m_stats.tingsForLife >= 100) {
                m_stats.tingsForLife = 0;
                m_events.extraLife = true;
                audio.play("oneup");
                popText(pc + sf::Vector2f(0.f, -60.f), "1UP!", sf::Color(140, 255, 160));
            }
            break;
        case PickupType::Gem:
            ++m_stats.gems; m_stats.score += 250;
            audio.play("gem");
            fx.sparkle(p.pos, sf::Color(120, 200, 255), 18);
            popText(p.pos, "+250", sf::Color(150, 220, 255));
            break;
        case PickupType::Relic:
            ++m_stats.relics; m_stats.score += 1000;
            audio.play("key");
            fx.sparkle(p.pos, sf::Color(255, 180, 240), 24);
            popText(p.pos, "RELIC +1000", sf::Color(255, 190, 240));
            break;
        case PickupType::Key:
            m_stats.keyFound = true; m_stats.score += 500; m_events.key = true;
            audio.play("key");
            fx.sparkle(p.pos, sf::Color(255, 230, 100), 30);
            popText(p.pos, "SECRET KEY!", sf::Color(255, 230, 120));
            break;
        case PickupType::Heart:
            player.heal(1); m_stats.score += 50;
            audio.play("gem", 0.7f, 0.f, 1.3f);
            fx.sparkle(p.pos, sf::Color(255, 120, 160), 12);
            break;
        case PickupType::Upgrade:
            m_stats.upgradeFound = true; m_events.upgrade = true;
            player.setDoubleJump(true);
            audio.play("upgrade");
            fx.sparkle(p.pos, sf::Color(190, 150, 255), 40);
            popText(p.pos + sf::Vector2f(0.f, -30.f), "DOUBLE JUMP!", sf::Color(200, 170, 255));
            break;
        }
    }
}

void Level::killEnemy(Enemy& e, Audio& audio, ParticleSystem& fx) {
    ++m_stats.enemies;
    m_stats.score += e.scoreValue();
    m_events.enemyKills.push_back((e.world() - 1) * kEnemyTypeCount + static_cast<int>(e.type()));
    audio.play("enemy_die", 0.9f, 0.1f);
    fx.burst(e.center(), 18, sf::Color(255, 240, 200), sf::Color(200, 120, 255, 0), 300.f, 0.5f, 16.f, 1);
    popText(e.center() + sf::Vector2f(0.f, -20.f), "+" + toStr(e.scoreValue()), sf::Color(255, 240, 180));
}

void Level::updateFist(float dt, Player& player, Audio& audio, ParticleSystem& fx) {
    const sf::Vector2f hand(player.feet().x + player.facing() * 18.f, player.feet().y - 34.f);
    if (!m_fist.update(dt, hand)) return;
    if (static_cast<int>(m_time * 60.f) % 2 == 0) fx.trail(m_fist.pos(), m_fist.charged() ? sf::Color(255, 220, 120) : sf::Color(255, 255, 255, 180));
    const sf::FloatRect fb = m_fist.box();
    const float dir = m_fist.returning() ? -m_fist.dir() : m_fist.dir();
    // enemies
    for (auto& e : m_enemies) {
        if (!e->alive() || !fb.intersects(e->box())) continue;
        if (!m_fist.tryHit(e->id())) continue;
        const int res = e->hit(m_fist.damage(), dir, m_fist.charged(), audio);
        fx.burst(m_fist.pos(), 8, sf::Color(255, 255, 255), sf::Color(255, 220, 160, 0), 220.f, 0.25f, 12.f, 1);
        if (res == 2) killEnemy(*e, audio, fx);
        if (res == 0) m_fist.forceReturn();
    }
    // cages
    for (size_t i = 0; i < m_cages.size(); ++i) {
        Cage& c = m_cages[i];
        if (c.broken) continue;
        const sf::FloatRect cr(c.pos.x - 28.f, c.pos.y - 60.f, 56.f, 60.f);
        if (!fb.intersects(cr) || !m_fist.tryHit(5000 + static_cast<int>(i))) continue;
        c.hp -= m_fist.damage();
        c.shake = 0.25f;
        audio.play("break", 0.6f, 0.1f, 1.3f);
        if (c.hp <= 0) {
            c.broken = true;
            c.freed = 0.f;
            ++m_stats.cages;
            m_stats.score += 500;
            audio.play("cage_break");
            fx.sparkle(sf::Vector2f(c.pos.x, c.pos.y - 30.f), sf::Color(255, 240, 140), 30);
            popText(sf::Vector2f(c.pos.x, c.pos.y - 70.f), "LUMLING FREED! +500", sf::Color(255, 240, 150));
        }
    }
    // boss
    if (m_boss && m_boss->active()) {
        const bool weak = fb.intersects(m_boss->weakRect());
        const bool body = fb.intersects(m_boss->bodyRect());
        if ((weak || body) && m_fist.tryHit(9000)) {
            if (weak && m_boss->vulnerable()) m_boss->takeHit(m_fist.damage(), audio, fx, *this);
            else m_boss->takeHit(0, audio, fx, *this);
            m_fist.forceReturn();
        }
    }
    // projectiles can be punched away
    for (auto& p : m_projectiles) {
        if (!p.alive || p.kind == 1) continue;
        if (length(p.pos - m_fist.pos()) < p.radius + 16.f) {
            p.alive = false;
            fx.burst(p.pos, 8, sf::Color(255, 255, 255), sf::Color(255, 200, 200, 0), 160.f, 0.3f, 10.f, 1);
            m_stats.score += 20;
        }
    }
    // tiles: breakables, switches, walls
    const int tx = toTile(m_fist.pos().x), ty = toTile(m_fist.pos().y);
    const char c = at(tx, ty);
    if (c == 'B') {
        setAt(tx, ty, '.');
        audio.play("break");
        fx.burst(sf::Vector2f(tx * kTileF + 24.f, ty * kTileF + 24.f), 16, sf::Color(220, 200, 230), sf::Color(120, 100, 130, 0), 260.f, 0.6f, 14.f, 2, 600.f);
        m_stats.score += 30;
        shake(3.f, 0.12f);
    } else if (c == 't' && m_fist.tryHit(7000 + tx)) {
        m_doorTimer = 8.f;
        audio.play("door");
        popText(sf::Vector2f(tx * kTileF + 24.f, ty * kTileF - 10.f), "DOORS OPEN!", sf::Color(140, 255, 200));
    } else if (!m_fist.returning() && solidAt(tx, ty)) {
        fx.burst(m_fist.pos(), 6, sf::Color(255, 255, 255), sf::Color(200, 200, 200, 0), 150.f, 0.2f, 8.f, 1);
        m_fist.forceReturn();
    }
}

void Level::updateEnemies(float dt, Player& player, Audio& audio, ParticleSystem& fx, Camera& cam) {
    (void)cam;
    const sf::FloatRect pb = player.box();
    const sf::FloatRect activeZone(player.feet().x - 1100.f, -2000.f, 2200.f, 6000.f);
    for (auto& e : m_enemies) {
        if (!activeZone.contains(e->center())) continue;  // dormant off-screen
        e->update(dt, *this, player, audio);
        if (e->consumeExplosion()) {
            fx.explosion(e->center(), sf::Color(255, 140, 60));
            audio.play("explosion");
            shake(8.f, 0.35f);
            if (length(player.center() - e->center()) < 95.f) player.hurt(e->center().x, audio, fx);
            if (!e->alive()) killEnemy(*e, audio, fx);
            continue;
        }
        if (!e->alive() || !player.alive()) continue;
        if (pb.intersects(e->box())) {
            const bool falling = player.velocity().y > 120.f;
            const bool above = pb.top + pb.height < e->box().top + 18.f;
            if (e->stompable() && falling && above) {
                e->kill();
                if (e->consumeExplosion()) {
                    fx.explosion(e->center(), sf::Color(255, 140, 60));
                    audio.play("explosion");
                }
                killEnemy(*e, audio, fx);
                player.bounce(640.f);
            } else if (e->harmful()) {
                player.hurt(e->center().x, audio, fx);
            }
        }
    }
    m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [](const std::unique_ptr<Enemy>& e) { return e->removable(); }),
                    m_enemies.end());
}

void Level::updateProjectiles(float dt, Player& player, Audio& audio, ParticleSystem& fx) {
    const sf::FloatRect pb = player.box();
    for (auto& p : m_projectiles) {
        if (!p.alive) continue;
        p.life -= dt;
        if (p.life <= 0.f) { p.alive = false; continue; }
        if (p.kind == 2) {
            const sf::Vector2f want = normalize(player.center() - p.pos) * 250.f;
            p.vel = lerpv(p.vel, want, 1.f - std::exp(-1.6f * dt));
            fx.trail(p.pos, p.sprite == 5 ? sf::Color(170, 60, 255, 180) : sf::Color(255, 80, 220, 180));
        }
        p.vel.y += p.gravity * dt;
        p.pos += p.vel * dt;
        p.rot += dt * 6.f;
        if (p.kind == 1) {
            fx.dust(sf::Vector2f(p.pos.x, p.pos.y + p.radius), 1, sf::Color(255, 240, 220, 150));
            if (solidAt(toTile(p.pos.x + signf(p.vel.x) * p.radius), toTile(p.pos.y))) p.alive = false;
        } else if (solidAt(toTile(p.pos.x), toTile(p.pos.y))) {
            p.alive = false;
            fx.burst(p.pos, 6, sf::Color(255, 230, 200), sf::Color(255, 150, 100, 0), 120.f, 0.3f, 10.f, 1);
            continue;
        }
        if (player.alive()) {
            const float cx = clampf(p.pos.x, pb.left, pb.left + pb.width), cy = clampf(p.pos.y, pb.top, pb.top + pb.height);
            const float dx = p.pos.x - cx, dy = p.pos.y - cy;
            if (dx * dx + dy * dy < p.radius * p.radius * 0.8f) {
                if (player.hurt(p.pos.x, audio, fx) && p.kind != 1) p.alive = false;
            }
        }
        if (p.pos.y > m_h * kTileF + 100.f) p.alive = false;
    }
    m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), [](const Projectile& p) { return !p.alive; }),
                        m_projectiles.end());
}

void Level::updateHazards(float dt, Player& player, Audio& audio, ParticleSystem& fx) {
    const sf::FloatRect pb = player.box();
    for (auto& h : m_hazards) {
        if (!h.alive) continue;
        const float before = h.t;
        h.t += dt;
        const bool active = h.t >= h.telegraph;
        if (active && before < h.telegraph) {
            if (h.kind == 0) { audio.play("break", 0.7f, 0.1f, 0.7f); shake(5.f, 0.2f); }
            if (h.kind == 1) audio.play("laser");
        }
        if (h.kind == 2 && active) {
            h.rect.top += 950.f * dt;
            if (h.rect.top + h.rect.height >= floorY()) {
                h.alive = false;
                fx.burst(sf::Vector2f(h.rect.left + h.rect.width * 0.5f, floorY() - 10.f), 14, sf::Color(220, 245, 255), sf::Color(160, 200, 255, 0),
                         260.f, 0.5f, 12.f, 1, 500.f);
                audio.play("break", 0.6f, 0.2f, 1.4f);
                continue;
            }
        }
        if (active && player.alive() && pb.intersects(h.rect)) player.hurt(h.rect.left + h.rect.width * 0.5f, audio, fx);
        if (h.t > h.telegraph + h.active) h.alive = false;
    }
    m_hazards.erase(std::remove_if(m_hazards.begin(), m_hazards.end(), [](const Hazard& h) { return !h.alive; }), m_hazards.end());
}

void Level::updateBoss(float dt, Player& player, Audio& audio, ParticleSystem& fx, Camera& cam) {
    if (!m_boss) return;
    const auto arena = arenaRect();
    if (!m_arenaLocked && !m_boss->defeated() && !m_boss->dying() && arena && player.feet().x > arena->left + kTileF * 2.f) {
        m_arenaLocked = true;
        for (int y = m_h - 6; y <= m_h - 4; ++y) setAt(m_arenaLeft, y, '#');
        m_boss->wake();
        cam.setFocusRect(arena);
        shake(6.f, 0.6f);
    }
    m_boss->update(dt, *this, player, audio, fx);
    if (m_boss->harmful() && player.alive() && player.box().intersects(m_boss->bodyRect())) player.hurt(m_boss->center().x, audio, fx);
    if (m_boss->defeated() && !m_stats.bossDefeated) {
        m_stats.bossDefeated = true;
        m_events.bossDefeated = true;
        m_stats.score += 5000;
        m_arenaLocked = false;
        for (int y = m_h - 6; y <= m_h - 4; ++y) setAt(m_arenaLeft, y, '.');
        m_exitPos = sf::Vector2f(arena->left + arena->width * 0.5f, floorY());
        m_exitActive = true;
        cam.setFocusRect(std::nullopt);
        m_projectiles.clear();
        m_hazards.clear();
        for (auto& e : m_enemies) if (e->alive()) e->kill();
        for (int i = 0; i < 24; ++i) {
            Pickup p;
            p.type = PickupType::Ting;
            const float a = static_cast<float>(i) / 24.f * 2.f * kPi;
            p.home = p.pos = sf::Vector2f(m_exitPos.x + std::cos(a) * 200.f, floorY() - 140.f + std::sin(a) * 70.f);
            m_pickups.push_back(p);
        }
        popText(sf::Vector2f(m_exitPos.x, floorY() - 260.f), "BOSS DEFEATED! +5000", sf::Color(255, 230, 140));
    }
}

void Level::update(float dt, Player& player, Audio& audio, ParticleSystem& fx, Camera& cam) {
    m_time += dt;
    if (!m_events.exitReached) m_stats.time += dt;
    m_doorTimer = std::max(0.f, m_doorTimer - dt);
    updatePlatforms(dt, player);
    updateFist(dt, player, audio, fx);
    updatePickups(dt, player, audio, fx);
    updateEnemies(dt, player, audio, fx, cam);
    updateProjectiles(dt, player, audio, fx);
    updateHazards(dt, player, audio, fx);
    updateBoss(dt, player, audio, fx, cam);

    const sf::FloatRect pb = player.box();
    // spike balls on chains
    for (auto& s : m_spikeballs) {
        s.angle += s.speed * dt;
        const sf::Vector2f bp = s.pivot + sf::Vector2f(std::cos(s.angle), std::sin(s.angle)) * s.radius;
        const float cx = clampf(bp.x, pb.left, pb.left + pb.width), cy = clampf(bp.y, pb.top, pb.top + pb.height);
        if ((bp.x - cx) * (bp.x - cx) + (bp.y - cy) * (bp.y - cy) < 26.f * 26.f) player.hurt(bp.x, audio, fx);
    }
    // switches can also be pressed by touch
    {
        const int tx = toTile(player.feet().x), ty = toTile(player.feet().y - 10.f);
        if (at(tx, ty) == 't' && m_doorTimer < 7.5f) {
            if (m_doorTimer <= 0.f) audio.play("door");
            m_doorTimer = 8.f;
        }
    }
    // checkpoints
    for (size_t i = 0; i < m_checkpoints.size(); ++i) {
        Checkpoint& c = m_checkpoints[i];
        const sf::FloatRect cr(c.pos.x - 30.f, c.pos.y - 100.f, 60.f, 100.f);
        if (!c.active && player.alive() && cr.intersects(pb)) {
            c.active = true;
            if (static_cast<int>(i) > m_activeCheckpoint) m_activeCheckpoint = static_cast<int>(i);
            m_events.checkpoint = true;
            audio.play("checkpoint");
            fx.sparkle(sf::Vector2f(c.pos.x - 14.f, c.pos.y - 86.f), sf::Color(255, 160, 220), 30);
            popText(sf::Vector2f(c.pos.x, c.pos.y - 120.f), "CHECKPOINT", sf::Color(255, 190, 230));
        }
    }
    // secrets
    for (auto& z : m_secrets) {
        if (!z.found && z.rect.intersects(pb)) {
            z.found = true;
            ++m_stats.secrets;
            m_stats.score += 300;
            m_events.secret = true;
            audio.play("achievement", 0.7f);
            popText(sf::Vector2f(z.rect.left + z.rect.width * 0.5f, z.rect.top - 20.f), "SECRET FOUND!", sf::Color(200, 170, 255));
        }
    }
    // hidden walls fade while Lumo is close
    bool nearHidden = false;
    for (const auto& h : m_hidden) {
        const sf::Vector2f hc(h.x * kTileF + 24.f, h.y * kTileF + 24.f);
        if (std::abs(hc.x - player.center().x) < 4.5f * kTileF && std::abs(hc.y - player.center().y) < 2.5f * kTileF) { nearHidden = true; break; }
    }
    m_hiddenAlpha = approach(m_hiddenAlpha, nearHidden ? 0.25f : 1.f, dt * 3.f);
    // freed lumlings float away
    for (auto& c : m_cages) {
        c.shake = std::max(0.f, c.shake - dt);
        if (c.freed >= 0.f) c.freed += dt;
    }
    // exit
    if (m_exitActive && !m_events.exitReached && player.alive()) {
        const sf::FloatRect er(m_exitPos.x - 44.f, m_exitPos.y - 260.f, 88.f, 260.f);  // tall so gliding over still counts
        if (er.intersects(pb)) {
            m_events.exitReached = true;
            audio.play("level_complete");
            fx.sparkle(sf::Vector2f(m_exitPos.x, m_exitPos.y - 70.f), sf::Color(160, 255, 230), 60);
        }
    }
    for (auto& t : m_texts) { t.life -= dt; t.pos.y -= 50.f * dt; }
    m_texts.erase(std::remove_if(m_texts.begin(), m_texts.end(), [](const FloatingText& t) { return t.life <= 0.f; }), m_texts.end());
    if (m_pendingShake > 0.f) { cam.shake(m_pendingShake, m_pendingShakeDur); m_pendingShake = 0.f; m_pendingShakeDur = 0.f; }
}

void Level::onPlayerRespawn() {
    m_projectiles.clear();
    m_hazards.clear();
    if (m_fist.active()) m_fist = Fist();
    if (m_boss && !m_boss->defeated() && !m_boss->dying()) {
        if (m_arenaLocked) {
            m_arenaLocked = false;
            for (int y = m_h - 6; y <= m_h - 4; ++y) setAt(m_arenaLeft, y, '.');
        }
        m_boss = std::make_unique<Boss>(m_info.world, m_bossSpawn, *arenaRect(), *m_assets);
        // remove summoned minions
        m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [](const std::unique_ptr<Enemy>& e) { return e->id() >= 1000; }),
                        m_enemies.end());
    }
}

// ============================================================== rendering
void Level::rebuildStatic() {
    m_static.clear();
    for (int y = 0; y < m_h; ++y) {
        for (int x = 0; x < m_w; ++x) {
            const char c = at(x, y);
            int idx = -1;
            switch (c) {
            case '#': idx = (isSolidChar(at(x, y - 1)) || at(x, y - 1) == 'H' || y == 0) ? 1 : 0; break;
            case '=': idx = 2; break;
            case 'B': idx = 3; break;
            case 'I': idx = 4; break;
            case '^': idx = 5; break;
            case 'U': idx = 22; break;
            case 'S': idx = 23; break;
            default: break;
            }
            if (idx < 0) continue;
            quad(m_static, sf::FloatRect(x * kTileF, y * kTileF, kTileF, kTileF), tileRect(idx));
            if (idx == 0 && at(x, y - 1) == '.' && (cellHash(x, y) % 3) == 0)
                quad(m_static, sf::FloatRect(x * kTileF, (y - 1) * kTileF + 10.f, kTileF, kTileF), tileRect(6));
        }
    }
    m_staticDirty = false;
}

void Level::drawBackground(sf::RenderTarget& t, const Camera& cam, float time) const {
    const int w = m_info.world;
    const sf::Vector2f c = cam.center();
    const float refY = std::max(kViewH * 0.5f, m_h * kTileF - kViewH * 0.5f);
    const sf::Texture& sky = m_assets->texture("bg" + toStr(w) + "_0");
    sf::Sprite s(sky);
    s.setPosition(c.x - kViewW * 0.5f, c.y - kViewH * 0.5f);
    t.draw(s);
    drawParallax(t, m_assets->texture("bg" + toStr(w) + "_1"), c.x, c.y, 0.12f, 0.05f, refY, time * 4.f);
    drawParallax(t, m_assets->texture("bg" + toStr(w) + "_2"), c.x, c.y, 0.3f, 0.12f, refY);
    drawParallax(t, m_assets->texture("fog"), c.x, c.y, 0.4f, 0.15f, refY + 260.f, time * 18.f, sf::Color(255, 255, 255, 70));
    drawParallax(t, m_assets->texture("bg" + toStr(w) + "_3"), c.x, c.y, 0.55f, 0.25f, refY);
}

void Level::drawTilesDynamic(sf::RenderTarget& t, const sf::FloatRect& view, float time) const {
    const sf::Texture& tiles = m_assets->texture("tiles" + toStr(m_info.world));
    sf::VertexArray va(sf::Triangles);
    const int x0 = std::max(0, toTile(view.left) - 1), x1 = std::min(m_w - 1, toTile(view.left + view.width) + 1);
    const int y0 = std::max(0, toTile(view.top) - 1), y1 = std::min(m_h - 1, toTile(view.top + view.height) + 1);
    const int frame = static_cast<int>(time * 6.f) % 4;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const char c = at(x, y);
            const sf::FloatRect r(x * kTileF, y * kTileF, kTileF, kTileF);
            if (c == 'L') quad(va, r, tileRect(15 + (frame + x) % 4));
            else if (c == 'd') quad(va, r, tileRect(8), m_doorTimer > 0.f ? sf::Color(255, 255, 255, 60) : sf::Color::White);
            else if (c == 't') quad(va, r, tileRect(m_doorTimer > 0.f ? 10 : 9));
        }
    }
    sf::RenderStates st;
    st.texture = &tiles;
    t.draw(va, st);
}

void Level::drawWorld(sf::RenderTarget& t, const Camera& cam, float time) const {
    const sf::FloatRect view = cam.visibleRect();
    const sf::FloatRect cull(view.left - 200.f, view.top - 200.f, view.width + 400.f, view.height + 400.f);
    const int w = m_info.world;
    // decorations sway gently (animated vegetation)
    const sf::Texture& deco = m_assets->texture("deco" + toStr(w));
    for (const auto& d : m_decos) {
        if (!cull.contains(d.pos)) continue;
        sf::Sprite s(deco, sf::IntRect(d.frame * 96, 0, 96, 96));
        s.setOrigin(48.f, 96.f);
        s.setPosition(d.pos);
        s.setScale(d.scale, d.scale);
        s.setRotation(std::sin(time * 1.4f + d.phase) * 3.5f);
        t.draw(s);
    }
    if (m_staticDirty) const_cast<Level*>(this)->rebuildStatic();
    sf::RenderStates st;
    st.texture = &m_assets->texture("tiles" + toStr(w));
    t.draw(m_static, st);
    drawTilesDynamic(t, view, time);

    const sf::Texture& tiles = m_assets->texture("tiles" + toStr(w));
    for (const auto& p : m_platforms) {
        if (p.alpha <= 0.01f || !cull.intersects(p.rect)) continue;
        const int idx = p.type == PlatformType::Disappearing ? 19 : (p.type == PlatformType::Falling ? 20 : 21);
        for (float x = 0.f; x < p.rect.width - 1.f; x += kTileF) {
            sf::Sprite s(tiles, tileRect(idx));
            s.setPosition(p.rect.left + x, p.rect.top - 2.f);
            s.setColor(withAlpha(sf::Color::White, p.alpha));
            t.draw(s);
        }
    }
    // spike balls
    const sf::Texture& ball = m_assets->texture("spikeball");
    const sf::Texture& chain = m_assets->texture("chain");
    for (const auto& s : m_spikeballs) {
        if (!cull.contains(s.pivot)) continue;
        for (int i = 0; i <= 5; ++i) {
            const float k = static_cast<float>(i) / 6.f;
            sf::Sprite c(chain);
            c.setOrigin(8.f, 8.f);
            c.setPosition(s.pivot + sf::Vector2f(std::cos(s.angle), std::sin(s.angle)) * s.radius * k);
            t.draw(c);
        }
        sf::Sprite b(ball);
        b.setOrigin(32.f, 32.f);
        b.setPosition(s.pivot + sf::Vector2f(std::cos(s.angle), std::sin(s.angle)) * s.radius);
        b.setRotation(s.angle * 120.f);
        t.draw(b);
    }
    // checkpoints
    const sf::Texture& cpTex = m_assets->texture("checkpoint");
    for (const auto& c : m_checkpoints) {
        if (!cull.contains(c.pos)) continue;
        sf::Sprite s(cpTex, sf::IntRect((static_cast<int>(time * 8.f) % 4) * 48, c.active ? 96 : 0, 48, 96));
        s.setOrigin(12.f, 96.f);
        s.setPosition(c.pos);
        t.draw(s);
    }
    // exit gate
    if (m_exitActive && cull.contains(m_exitPos)) {
        const sf::Texture& g = m_assets->texture("exit_gate");
        sf::Sprite s(g, sf::IntRect((static_cast<int>(time * 10.f) % 6) * 96, 0, 96, 144));
        s.setOrigin(48.f, 144.f);
        s.setPosition(m_exitPos);
        t.draw(s);
    }
    // cages and freed lumlings
    const sf::Texture& cage = m_assets->texture("cage");
    const sf::Texture& lum = m_assets->texture("lumling");
    for (const auto& c : m_cages) {
        if (!cull.contains(c.pos)) continue;
        sf::Sprite s(cage, sf::IntRect(c.broken ? 64 : 0, 0, 64, 64));
        s.setOrigin(32.f, 64.f);
        s.setPosition(c.pos + sf::Vector2f(c.shake > 0.f ? std::sin(time * 80.f) * 3.f : 0.f, 2.f));
        t.draw(s);
        if (c.freed >= 0.f && c.freed < 3.f) {
            sf::Sprite l(lum, sf::IntRect((static_cast<int>(time * 12.f) % 6) * 40, 0, 40, 40));
            l.setOrigin(20.f, 20.f);
            l.setPosition(c.pos.x + std::sin(c.freed * 5.f) * 20.f, c.pos.y - 34.f - c.freed * 120.f);
            l.setColor(withAlpha(sf::Color::White, 1.f - c.freed / 3.f));
            t.draw(l);
        }
    }
    // pickups
    for (const auto& p : m_pickups) {
        if (p.taken || !cull.contains(p.pos)) continue;
        const char* key = "ting";
        int fw = 32, frames = 8;
        float fps = 12.f;
        switch (p.type) {
        case PickupType::Ting: break;
        case PickupType::Gem: key = "gem"; fw = 40; frames = 6; fps = 8.f; break;
        case PickupType::Key: key = "key"; fw = 40; frames = 4; fps = 6.f; break;
        case PickupType::Relic: key = "relics"; fw = 48; frames = 1; break;
        case PickupType::Heart: key = "heart"; fw = 32; frames = 1; break;
        case PickupType::Upgrade: key = "upgrade"; fw = 48; frames = 6; fps = 10.f; break;
        }
        const sf::Texture& tex = m_assets->texture(key);
        int f = (static_cast<int>(time * fps + p.phase * 3.f)) % frames;
        if (p.type == PickupType::Relic) f = w - 1;
        sf::Sprite s(tex, sf::IntRect(f * fw, 0, fw, fw));
        s.setOrigin(fw * 0.5f, fw * 0.5f);
        s.setPosition(p.pos);
        if (p.type == PickupType::Relic || p.type == PickupType::Key) s.setScale(1.2f + 0.08f * std::sin(time * 4.f), 1.2f + 0.08f * std::sin(time * 4.f));
        t.draw(s);
    }
    for (const auto& e : m_enemies) {
        if (cull.contains(e->center())) e->draw(t);
    }
    if (m_boss) m_boss->draw(t, time);
    // projectiles
    const sf::Texture& proj = m_assets->texture("projectiles");
    const sf::Texture& wave = m_assets->texture("shockwave");
    for (const auto& p : m_projectiles) {
        if (p.kind == 1) {
            sf::Sprite s(wave);
            s.setOrigin(48.f, 48.f);
            s.setPosition(p.pos.x, p.pos.y + p.radius);
            s.setScale(p.vel.x < 0.f ? -1.f : 1.f, 1.f + 0.1f * std::sin(time * 20.f));
            if (p.sprite == 4) s.setColor(sf::Color(255, 140, 60));
            t.draw(s);
            continue;
        }
        sf::Sprite s(proj, sf::IntRect(p.sprite * 32, 0, 32, 32));
        s.setOrigin(16.f, 16.f);
        s.setPosition(p.pos);
        if (p.sprite == 3) s.setRotation(std::atan2(p.vel.y, p.vel.x) * 180.f / kPi - 90.f);
        else s.setRotation(p.rot * 30.f);
        s.setScale(1.3f, 1.3f);
        t.draw(s);
    }
    // hazards (telegraphs first, then active)
    const sf::Texture& tele = m_assets->texture("telegraph");
    const sf::Texture& laser = m_assets->texture("laser");
    for (const auto& h : m_hazards) {
        const bool active = h.t >= h.telegraph;
        if (!active) {
            const float blink = 0.5f + 0.5f * std::sin(h.t * 25.f);
            if (h.kind == 1) {
                sf::RectangleShape line(sf::Vector2f(h.rect.width, 3.f));
                line.setPosition(h.rect.left, h.rect.top + h.rect.height * 0.5f - 1.5f);
                line.setFillColor(sf::Color(255, 60, 200, static_cast<sf::Uint8>(90 + 120 * blink)));
                t.draw(line);
            } else {
                sf::Sprite s(tele);
                s.setOrigin(32.f, 8.f);
                s.setPosition(h.rect.left + h.rect.width * 0.5f, floorY() - 4.f);
                s.setScale(h.rect.width / 50.f, 1.f);
                s.setColor(withAlpha(sf::Color::White, 0.4f + 0.6f * blink));
                t.draw(s);
                if (h.kind == 2) {
                    sf::Sprite ic(proj, sf::IntRect(3 * 32, 0, 32, 32));
                    ic.setOrigin(16.f, 0.f);
                    ic.setPosition(h.rect.left + h.rect.width * 0.5f, h.rect.top + std::sin(h.t * 40.f) * 2.f);
                    ic.setScale(1.6f, 2.2f);
                    t.draw(ic);
                }
            }
            continue;
        }
        if (h.kind == 1) {
            sf::Sprite s(laser);
            s.setPosition(h.rect.left, h.rect.top - 2.f);
            s.setScale(h.rect.width / 32.f, (h.rect.height + 4.f) / 32.f);
            t.draw(s);
        } else if (h.kind == 2) {
            sf::Sprite ic(proj, sf::IntRect(3 * 32, 0, 32, 32));
            ic.setOrigin(16.f, 0.f);
            ic.setPosition(h.rect.left + h.rect.width * 0.5f, h.rect.top);
            ic.setScale(1.6f, 2.2f);
            t.draw(ic);
        } else {
            // erupting thorn roots / fire pillar
            const float k = clampf((h.t - h.telegraph) / 0.12f, 0.f, 1.f);
            sf::ConvexShape spike(3);
            const sf::Color col = (m_info.world == 5) ? sf::Color(255, 140, 40) : sf::Color(90, 170, 70);
            for (int i = 0; i < 3; ++i) {
                const float bx = h.rect.left + h.rect.width * (static_cast<float>(i) + 0.5f) / 3.f;
                spike.setPoint(0, sf::Vector2f(bx - 14.f, floorY()));
                spike.setPoint(1, sf::Vector2f(bx + 14.f, floorY()));
                spike.setPoint(2, sf::Vector2f(bx, floorY() - h.rect.height * k));
                spike.setFillColor(col);
                spike.setOutlineThickness(2.f);
                spike.setOutlineColor(sf::Color(30, 16, 40));
                t.draw(spike);
            }
        }
    }
    m_fist.draw(t, m_assets->texture("fist"), time);
}

void Level::drawForeground(sf::RenderTarget& t, const Camera& cam, float time) const {
    const sf::FloatRect view = cam.visibleRect();
    const sf::Texture& tiles = m_assets->texture("tiles" + toStr(m_info.world));
    sf::VertexArray va(sf::Triangles);
    const int x0 = std::max(0, toTile(view.left) - 1), x1 = std::min(m_w - 1, toTile(view.left + view.width) + 1);
    const int y0 = std::max(0, toTile(view.top) - 1), y1 = std::min(m_h - 1, toTile(view.top + view.height) + 1);
    const int frame = static_cast<int>(time * 6.f) % 4;
    const sf::Color hiddenCol = withAlpha(sf::Color::White, m_hiddenAlpha);
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const char c = at(x, y);
            const sf::FloatRect r(x * kTileF, y * kTileF, kTileF, kTileF);
            if (c == '~') quad(va, r, tileRect(at(x, y - 1) == '~' ? 11 + (frame + 2) % 4 : 11 + (frame + x) % 4), sf::Color(255, 255, 255, 200));
            else if (c == 'H') quad(va, r, tileRect(7), hiddenCol);
        }
    }
    sf::RenderStates st;
    st.texture = &tiles;
    t.draw(va, st);
    // wind streaks
    const sf::Texture& wind = m_assets->texture("wind");
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const char c = at(x, y);
            if (c != 'w' && c != '<' && c != '>') continue;
            if ((cellHash(x, y) % 3) != 0) continue;
            sf::Sprite s(wind);
            const float ph = std::fmod(time * 2.f + static_cast<float>(cellHash(x, y) % 100) / 100.f, 1.f);
            if (c == 'w') {
                s.setRotation(-90.f);
                s.setPosition(x * kTileF + 24.f, y * kTileF + kTileF - ph * kTileF * 2.f);
            } else {
                const float dir = c == '<' ? -1.f : 1.f;
                s.setPosition(x * kTileF + 24.f + dir * (ph - 0.5f) * kTileF * 2.f, y * kTileF + 20.f);
            }
            s.setColor(withAlpha(sf::Color::White, std::sin(ph * kPi) * 0.8f));
            t.draw(s);
        }
    }
    for (const auto& tx : m_texts) {
        ui::text(t, m_assets->font("ui"), tx.text, 22, tx.pos, tx.color, ui::Align::Center, 2.f, sf::Color(30, 16, 50), clampf(tx.life, 0.f, 1.f));
    }
}

} // namespace rayman
