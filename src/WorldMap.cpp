#include "WorldMap.h"
#include "AssetManager.h"
#include "Audio.h"
#include "Graphics.h"
#include "Input.h"
#include "ParticleSystem.h"
#include <deque>
#include <sstream>

namespace rayman {

namespace {
const char* kWorldNames[kWorlds] = {"DREAM FOREST", "BAND LAND", "PICTURE CITY", "FROZEN KINGDOM", "VOLCANO OF SHADOWS"};
const sf::Color kWorldColors[kWorlds] = {sf::Color(255, 150, 230), sf::Color(255, 190, 90), sf::Color(0, 230, 255), sf::Color(200, 235, 255),
                                         sf::Color(255, 120, 60)};
}

void WorldMap::setup(const AssetManager& a) {
    m_hero.setup(&a.texture("lumo"), &a.anim("lumo"));
    m_hero.play("idle");
    const sf::Texture& map = a.texture("map");
    if (map.getSize().x > 64) m_mapSize = sf::Vector2f(static_cast<float>(map.getSize().x), static_cast<float>(map.getSize().y));
    // nodes
    std::istringstream ss(a.textFile("worldmap/nodes.txt"));
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        int w = 0, s = 0;
        std::string kind;
        float x = 0.f, y = 0.f;
        ls >> w >> s >> kind >> x >> y;
        if (ls.fail() || w < 1 || w > kWorlds || s < 1 || s > kStagesPerWorld) continue;
        Node& n = m_nodes[static_cast<size_t>(idx(w, s))];
        n.pos = sf::Vector2f(x, y);
        n.kind = (s == kBossStage) ? 3 : (s == kSecretStage ? 2 : (s == 5 ? 1 : 0));
    }
    // stage names from level headers
    for (int w = 1; w <= kWorlds; ++w) {
        for (int s = 1; s <= kStagesPerWorld; ++s) {
            std::istringstream ls(a.textFile("levels/w" + toStr(w) + "_s" + toStr(s) + ".txt"));
            std::string l;
            while (std::getline(ls, l)) {
                if (l.compare(0, 5, "name=") == 0) { m_nodes[static_cast<size_t>(idx(w, s))].name = trim(l.substr(5)); break; }
                if (l == "---") break;
            }
        }
    }
    // edges: 1-2-3-4-5-boss -> next world; secret branch from stage 3
    m_edges.clear();
    for (int w = 1; w <= kWorlds; ++w) {
        for (int s = 1; s < 5; ++s) m_edges.push_back(Edge{idx(w, s), idx(w, s + 1), false});
        m_edges.push_back(Edge{idx(w, 5), idx(w, kBossStage), false});
        m_edges.push_back(Edge{idx(w, 3), idx(w, kSecretStage), true});
        if (w < kWorlds) m_edges.push_back(Edge{idx(w, kBossStage), idx(w + 1, 1), false});
    }
    // scenery props sampled from the painted map (trees on land, deterministic)
    m_props.clear();
    m_waterfalls.clear();
    if (const sf::Image* img = a.image("map")) {
        const sf::Vector2u sz = img->getSize();
        auto isWater = [&](int x, int y) {
            if (x < 0 || y < 0 || x >= static_cast<int>(sz.x) || y >= static_cast<int>(sz.y)) return true;
            const sf::Color c = img->getPixel(static_cast<unsigned>(x), static_cast<unsigned>(y));
            return c.b > c.r + 40 && c.b > c.g + 10;
        };
        Rng r(77);
        for (int k = 0; k < 900 && m_props.size() < 150; ++k) {
            const int x = r.range(40, static_cast<int>(sz.x) - 40), y = r.range(40, static_cast<int>(sz.y) - 40);
            if (isWater(x, y) || isWater(x + 30, y) || isWater(x - 30, y) || isWater(x, y + 30)) continue;
            bool nearNode = false;
            for (const auto& n : m_nodes) if (length(n.pos - sf::Vector2f(static_cast<float>(x), static_cast<float>(y))) < 70.f) nearNode = true;
            if (nearNode) continue;
            Prop p;
            p.pos = sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
            const float fx = static_cast<float>(x) / static_cast<float>(sz.x);
            p.kind = (fx > 0.68f) ? 2 : (fx < 0.33f ? 0 : 1);
            p.phase = r.uniform(0.f, 6.f);
            p.scale = r.uniform(0.7f, 1.1f);
            m_props.push_back(p);
        }
        // waterfalls where land meets the sea on the southern coast
        for (int x = 300; x < static_cast<int>(sz.x) - 300 && m_waterfalls.size() < 5; x += 37) {
            for (int y = static_cast<int>(sz.y) / 2; y < static_cast<int>(sz.y) - 60; y += 4) {
                if (!isWater(x, y) && isWater(x, y + 12) && isWater(x, y + 40)) {
                    bool far = true;
                    for (const auto& wf : m_waterfalls) if (std::abs(wf.x - static_cast<float>(x)) < 380.f) far = false;
                    if (far) m_waterfalls.push_back(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)));
                    break;
                }
            }
        }
    }
    std::sort(m_props.begin(), m_props.end(), [](const Prop& p, const Prop& q) { return p.pos.y < q.pos.y; });
}

std::string WorldMap::stageName(int w, int s) const {
    if (w < 1 || w > kWorlds || s < 1 || s > kStagesPerWorld) return std::string();
    return m_nodes[static_cast<size_t>(idx(w, s))].name;
}

bool WorldMap::nodeOpen(int i) const {
    if (!m_save) return i == 0;
    const int w = i / kStagesPerWorld + 1, s = i % kStagesPerWorld + 1;
    return m_save->stage(w, s).unlocked;
}

bool WorldMap::edgeOpen(const Edge& e) const { return nodeOpen(e.a) && nodeOpen(e.b); }

void WorldMap::enter(const SaveData& save, int revealWorld, int revealStage) {
    m_save = &save;
    m_cur = idx(std::max(1, std::min(kWorlds, save.mapWorld)), std::max(1, std::min(kStagesPerWorld, save.mapStage)));
    if (!nodeOpen(m_cur)) m_cur = 0;
    m_heroPos = m_nodes[static_cast<size_t>(m_cur)].pos;
    m_cam = m_heroPos;
    m_travelTo = -1;
    m_queue.clear();
    m_revealEdge = -1;
    m_revealNode = -1;
    m_revealT = 1.f;
    m_celebrateNode = -1;
    m_t = 0.f;
    if (revealWorld > 0) {
        const int target = idx(revealWorld, revealStage);
        for (size_t i = 0; i < m_edges.size(); ++i) {
            if (m_edges[i].b == target && nodeOpen(m_edges[i].a)) { m_revealEdge = static_cast<int>(i); break; }
        }
        m_revealNode = target;
        m_revealT = 0.f;
        m_celebrateNode = m_cur;
        m_celebrateT = 0.f;
    }
    m_hero.play("idle", true);
}

std::vector<int> WorldMap::pathTo(int target) const {
    std::vector<int> prev(m_nodes.size(), -1);
    std::vector<bool> seen(m_nodes.size(), false);
    std::deque<int> q;
    q.push_back(m_cur);
    seen[static_cast<size_t>(m_cur)] = true;
    while (!q.empty()) {
        const int n = q.front();
        q.pop_front();
        if (n == target) break;
        for (const Edge& e : m_edges) {
            if (!edgeOpen(e)) continue;
            int o = -1;
            if (e.a == n) o = e.b;
            else if (e.b == n) o = e.a;
            if (o >= 0 && !seen[static_cast<size_t>(o)]) { seen[static_cast<size_t>(o)] = true; prev[static_cast<size_t>(o)] = n; q.push_back(o); }
        }
    }
    std::vector<int> path;
    if (!seen[static_cast<size_t>(target)]) return path;
    for (int n = target; n != m_cur && n >= 0; n = prev[static_cast<size_t>(n)]) path.push_back(n);
    std::reverse(path.begin(), path.end());
    return path;
}

void WorldMap::startTravel(int to, Audio& audio) {
    m_travelTo = to;
    m_travelT = 0.f;
    const float dx = m_nodes[static_cast<size_t>(to)].pos.x - m_nodes[static_cast<size_t>(m_cur)].pos.x;
    if (std::abs(dx) > 1.f) m_heroDir = dx > 0.f ? 1.f : -1.f;
    m_hero.play("run");
    audio.play("menu_move", 0.5f);
}

MapResult WorldMap::update(float dt, const Input& in, Audio& audio, ParticleSystem& fx, SaveData& save) {
    MapResult res;
    m_save = &save;
    m_t += dt;
    m_hero.update(dt);
    m_lockedMsg = std::max(0.f, m_lockedMsg - dt);
    if (m_celebrateNode >= 0) m_celebrateT += dt;
    // unlock animation: path draws itself, then the node pops
    if (m_revealT < 1.f) {
        const float before = m_revealT;
        m_revealT = std::min(1.f, m_revealT + dt / 1.6f);
        if (before < 0.85f && m_revealT >= 0.85f && m_revealNode >= 0) {
            audio.play("checkpoint");
            fx.sparkle(m_nodes[static_cast<size_t>(m_revealNode)].pos, sf::Color(255, 240, 160), 30);
        }
    }
    fx.ambient(AmbientKind::MenuDust, sf::FloatRect(m_cam.x - kViewW * 0.5f, m_cam.y - kViewH * 0.5f, static_cast<float>(kViewW), static_cast<float>(kViewH)), dt);

    if (m_travelTo >= 0) {
        const sf::Vector2f a = m_nodes[static_cast<size_t>(m_cur)].pos, b = m_nodes[static_cast<size_t>(m_travelTo)].pos;
        const float dist = std::max(1.f, length(b - a));
        m_travelT += dt * 460.f / dist;
        // gentle arc along the trail
        const float t = easeInOut(clampf(m_travelT, 0.f, 1.f));
        m_heroPos = lerpv(a, b, t) + sf::Vector2f(0.f, -std::sin(t * kPi) * 14.f);
        if (static_cast<int>(m_t * 12.f) % 2 == 0) fx.dust(m_heroPos, 1, sf::Color(255, 255, 255, 120));
        if (m_travelT >= 1.f) {
            m_cur = m_travelTo;
            m_travelTo = -1;
            m_heroPos = b;
            save.mapWorld = heroWorld();
            save.mapStage = heroStage();
            if (!m_queue.empty()) {
                const int next = m_queue.front();
                m_queue.erase(m_queue.begin());
                startTravel(next, audio);
            } else {
                m_hero.play("idle");
            }
        }
    } else if (m_revealT >= 1.f) {
        // keyboard: pick the open neighbour that best matches the pressed direction
        sf::Vector2f want(0.f, 0.f);
        if (in.pressed(Action::Left)) want.x = -1.f;
        if (in.pressed(Action::Right)) want.x = 1.f;
        if (in.pressed(Action::Up)) want.y = -1.f;
        if (in.pressed(Action::Down)) want.y = 1.f;
        if (want.x != 0.f || want.y != 0.f) {
            int best = -1;
            float bestDot = 0.3f;
            bool lockedThatWay = false;
            for (const Edge& e : m_edges) {
                int o = -1;
                if (e.a == m_cur) o = e.b;
                else if (e.b == m_cur) o = e.a;
                if (o < 0) continue;
                const sf::Vector2f d = normalize(m_nodes[static_cast<size_t>(o)].pos - m_nodes[static_cast<size_t>(m_cur)].pos);
                const float dot = d.x * want.x + d.y * want.y;
                if (!edgeOpen(e)) {
                    if (dot > 0.5f && !(e.secret && !nodeOpen(o))) lockedThatWay = true;
                    continue;
                }
                if (dot > bestDot) { bestDot = dot; best = o; }
            }
            if (best >= 0) startTravel(best, audio);
            else if (lockedThatWay) { m_lockedMsg = 1.5f; audio.play("menu_back"); }
        }
        if (in.mouseClicked()) {
            const sf::Vector2f world = in.mouse() + sf::Vector2f(m_cam.x - kViewW * 0.5f, m_cam.y - kViewH * 0.5f);
            for (size_t i = 0; i < m_nodes.size(); ++i) {
                if (length(m_nodes[i].pos - world) > 36.f) continue;
                if (static_cast<int>(i) == m_cur) {
                    res.kind = MapResult::Enter; res.world = heroWorld(); res.stage = heroStage();
                    audio.play("menu_select");
                    return res;
                }
                if (!nodeOpen(static_cast<int>(i))) { m_lockedMsg = 1.5f; audio.play("menu_back"); break; }
                m_queue = pathTo(static_cast<int>(i));
                if (!m_queue.empty()) {
                    const int next = m_queue.front();
                    m_queue.erase(m_queue.begin());
                    startTravel(next, audio);
                }
                break;
            }
        }
        if (in.pressed(Action::Confirm) || in.pressed(Action::Jump)) {
            if (nodeOpen(m_cur)) {
                res.kind = MapResult::Enter;
                res.world = heroWorld();
                res.stage = heroStage();
                audio.play("menu_select");
                return res;
            }
            m_lockedMsg = 1.5f;
            audio.play("menu_back");
        }
    }
    if (in.pressed(Action::Back)) {
        res.kind = MapResult::Back;
        audio.play("menu_back");
        return res;
    }
    // camera
    m_cam = lerpv(m_cam, m_heroPos, 1.f - std::exp(-4.f * dt));
    m_cam.x = clampf(m_cam.x, kViewW * 0.5f, m_mapSize.x - kViewW * 0.5f);
    m_cam.y = clampf(m_cam.y, kViewH * 0.5f, m_mapSize.y - kViewH * 0.5f);
    return res;
}

sf::View WorldMap::camera() const {
    sf::View v(sf::FloatRect(0.f, 0.f, static_cast<float>(kViewW), static_cast<float>(kViewH)));
    v.setCenter(std::round(m_cam.x), std::round(m_cam.y));
    return v;
}

void WorldMap::draw(sf::RenderTarget& t, const AssetManager& a, const ParticleSystem& fx, float time) const {
    const sf::View old = t.getView();
    t.setView(camera());
    // sea shimmer behind the island texture
    sf::Sprite map(a.texture("map"));
    t.draw(map);
    // waterfalls
    const sf::Texture& wf = a.texture("map_waterfall");
    for (const auto& p : m_waterfalls) {
        sf::Sprite s(wf, sf::IntRect((static_cast<int>(time * 10.f) % 4) * 48, 0, 48, 192));
        s.setOrigin(24.f, 8.f);
        s.setPosition(p);
        s.setScale(0.8f, 0.55f);
        t.draw(s);
    }
    // paths: dotted trails, gold when open, grey when locked; the newly unlocked one draws itself
    for (size_t i = 0; i < m_edges.size(); ++i) {
        const Edge& e = m_edges[i];
        const bool secretHidden = e.secret && !nodeOpen(e.b);
        if (secretHidden) continue;  // secret paths stay invisible until their key is found
        const sf::Vector2f pa = m_nodes[static_cast<size_t>(e.a)].pos, pb = m_nodes[static_cast<size_t>(e.b)].pos;
        const float len = length(pb - pa);
        const int dots = std::max(2, static_cast<int>(len / 22.f));
        float limit = 1.f;
        bool open = edgeOpen(e);
        if (static_cast<int>(i) == m_revealEdge) limit = clampf(m_revealT / 0.85f, 0.f, 1.f);
        for (int d = 1; d < dots; ++d) {
            const float k = static_cast<float>(d) / static_cast<float>(dots);
            const sf::Vector2f perp = normalize(sf::Vector2f(-(pb - pa).y, (pb - pa).x));
            const sf::Vector2f p = lerpv(pa, pb, k) + perp * std::sin(k * kPi * 2.f) * 18.f;
            const bool lit = open && k <= limit;
            sf::CircleShape dot(lit ? 6.f : 4.5f);
            dot.setOrigin(dot.getRadius(), dot.getRadius());
            dot.setPosition(p);
            dot.setFillColor(lit ? (e.secret ? sf::Color(210, 160, 255) : sf::Color(255, 226, 130)) : sf::Color(90, 80, 100, 200));
            dot.setOutlineThickness(1.5f);
            dot.setOutlineColor(sf::Color(40, 20, 50, 200));
            t.draw(dot);
        }
    }
    // props (y-sorted), gently swaying
    const sf::Texture* trees[3] = {&a.texture("map_tree0"), &a.texture("map_tree1"), &a.texture("map_tree2")};
    for (const auto& p : m_props) {
        sf::Sprite s(*trees[p.kind]);
        s.setOrigin(32.f, 96.f);
        s.setPosition(p.pos);
        s.setScale(p.scale * 0.8f, p.scale * 0.8f);
        s.setRotation(std::sin(time * 1.3f + p.phase) * 3.f);
        t.draw(s);
    }
    // nodes
    const sf::Texture& nodes = a.texture("map_nodes");
    const sf::Texture& flag = a.texture("map_flag");
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const Node& n = m_nodes[i];
        const int w = static_cast<int>(i) / kStagesPerWorld + 1, s = static_cast<int>(i) % kStagesPerWorld + 1;
        const bool open = nodeOpen(static_cast<int>(i));
        if (n.kind == 2 && !open) continue;  // undiscovered secret stage
        const StageRecord& rec = m_save ? m_save->stage(w, s) : StageRecord();
        int frame = 0;
        if (open) frame = rec.completed ? 2 : 1;
        if (open && n.kind == 3) frame = rec.completed ? 2 : 3;
        if (open && n.kind == 2) frame = 4;
        if (open && n.kind == 1 && !rec.completed) frame = 5;
        float scale = 1.f;
        if (static_cast<int>(i) == m_revealNode && m_revealT < 1.f) scale = m_revealT < 0.85f ? 0.f : easeOutBack((m_revealT - 0.85f) / 0.15f);
        if (static_cast<int>(i) == m_cur) scale *= 1.f + 0.06f * std::sin(time * 5.f);
        if (n.kind == 3) scale *= 1.25f;
        sf::Sprite sp(nodes, sf::IntRect(frame * 64, 0, 64, 64));
        sp.setOrigin(32.f, 40.f);
        sp.setPosition(n.pos);
        sp.setScale(scale, scale);
        t.draw(sp);
        if (open && scale > 0.5f) {
            ui::text(t, a.font("ui"), s <= 5 ? toStr(s) : (s == kSecretStage ? "S" : "B"), 16, sf::Vector2f(n.pos.x, n.pos.y - 66.f),
                     sf::Color::White, ui::Align::Center, 2.f);
        }
        if (rec.completed) {
            sf::Sprite f(flag, sf::IntRect((static_cast<int>(time * 10.f + static_cast<float>(i)) % 6) * 48, 0, 48, 72));
            const float rise = (static_cast<int>(i) == m_celebrateNode) ? easeOutBack(clampf(m_celebrateT / 0.8f, 0.f, 1.f)) : 1.f;
            f.setOrigin(8.f, 72.f);
            f.setPosition(n.pos.x + 20.f, n.pos.y - 6.f);
            f.setScale(0.7f, 0.7f * rise);
            t.draw(f);
            for (int k = 0; k < rec.stars; ++k)
                ui::icon(t, a.texture("icons"), 4, sf::Vector2f(n.pos.x - 16.f + static_cast<float>(k) * 16.f, n.pos.y + 18.f), 18.f);
        }
    }
    // hero
    m_hero.draw(t, m_heroPos + sf::Vector2f(0.f, -4.f), m_heroDir < 0.f, 0.8f);
    fx.draw(t);
    // clouds drift over everything with soft shadows
    for (int k = 0; k < 7; ++k) {
        const sf::Texture& ct = a.texture("map_cloud" + toStr(k % 3));
        const float span = m_mapSize.x + 700.f;
        const float x = std::fmod(static_cast<float>(k) * 410.f + time * (14.f + 4.f * static_cast<float>(k)), span) - 350.f;
        const float y = 120.f + static_cast<float>((k * 263) % 1300);
        sf::Sprite sh(ct);
        sh.setPosition(x + 40.f, y + 90.f);
        sh.setColor(sf::Color(0, 0, 40, 40));
        t.draw(sh);
        sf::Sprite c(ct);
        c.setPosition(x, y);
        c.setColor(sf::Color(255, 255, 255, 190));
        t.draw(c);
    }
    t.setView(t.getDefaultView());
    // HUD: region banner + node info
    const int w = (m_travelTo >= 0 ? m_travelTo : m_cur) / kStagesPerWorld + 1;
    ui::panel(t, a.texture("panel"), sf::FloatRect(20.f, 16.f, 470.f, 62.f), 0.9f, 20.f);
    ui::text(t, a.font("display"), "WORLD " + toStr(w) + " - " + kWorldNames[w - 1], 26, sf::Vector2f(40.f, 28.f), kWorldColors[w - 1]);
    if (m_save) {
        ui::text(t, a.font("ui"), toStr(static_cast<int>(m_save->totalCompletion() * 100.f)) + "% complete", 20, sf::Vector2f(kViewW - 30.f, 30.f),
                 sf::Color(255, 240, 200), ui::Align::Right);
    }
    if (m_travelTo < 0 && m_save) {
        const int s = heroStage();
        const StageRecord& rec = m_save->stage(w, s);
        const sf::FloatRect r(330.f, kViewH - 168.f, 620.f, 150.f);
        ui::panel(t, a.texture("panel"), r, 0.94f);
        const char* kinds[4] = {"Stage", "Challenge Stage", "Secret Stage", "BOSS"};
        const Node& n = m_nodes[static_cast<size_t>(m_cur)];
        ui::text(t, a.font("display"), toStr(w) + "-" + (s <= 5 ? toStr(s) : (s == kSecretStage ? std::string("S") : std::string("B"))) + "  " + n.name, 26,
                 sf::Vector2f(r.left + 24.f, r.top + 14.f), sf::Color(255, 236, 160));
        ui::text(t, a.font("ui"), kinds[n.kind], 18, sf::Vector2f(r.left + r.width - 24.f, r.top + 20.f), sf::Color(220, 200, 255), ui::Align::Right);
        if (rec.completed) {
            const std::string l1 = "Tings " + toStr(rec.bestTings) + "/" + toStr(rec.tingsTotal) + "    Lumlings " + toStr(rec.bestCages) + "/" +
                                   toStr(rec.cagesTotal) + "    Gems " + toStr(rec.bestGems) + "/" + toStr(rec.gemsTotal);
            const std::string l2 = "Secrets " + toStr(rec.secretsFound) + "/" + toStr(rec.secretsTotal) + "    Best " + padNumber(rec.bestScore, 7) +
                                   "    " + toStr(static_cast<int>(rec.completion * 100.f + 0.5f)) + "%";
            ui::text(t, a.font("body"), l1, 19, sf::Vector2f(r.left + 24.f, r.top + 58.f), sf::Color::White);
            ui::text(t, a.font("body"), l2, 19, sf::Vector2f(r.left + 24.f, r.top + 88.f), sf::Color::White);
            for (int k = 0; k < 3; ++k) ui::icon(t, a.texture("icons"), k < rec.stars ? 4 : 5, sf::Vector2f(r.left + r.width - 110.f + static_cast<float>(k) * 36.f, r.top + 106.f), 32.f);
        } else {
            ui::text(t, a.font("body"), "Not yet completed - press Enter to begin!", 20, sf::Vector2f(r.left + 24.f, r.top + 66.f), sf::Color::White);
            if (s == 3 && !m_save->stage(w, kSecretStage).unlocked)
                ui::text(t, a.font("body"), "A secret key is said to be hidden here...", 18, sf::Vector2f(r.left + 24.f, r.top + 100.f), sf::Color(210, 170, 255));
        }
    }
    if (m_lockedMsg > 0.f)
        ui::text(t, a.font("ui"), "That path is still locked!", 24, sf::Vector2f(kViewW * 0.5f, 100.f), sf::Color(255, 150, 160), ui::Align::Center, 2.f,
                 sf::Color(30, 16, 50), clampf(m_lockedMsg, 0.f, 1.f));
    ui::text(t, a.font("body"), "Arrows/WASD: move   Enter: play   Esc: menu   Mouse: click a stage", 17, sf::Vector2f(kViewW - 30.f, 60.f),
             sf::Color(230, 220, 255), ui::Align::Right, 2.f);
    t.setView(old);
}

} // namespace rayman
