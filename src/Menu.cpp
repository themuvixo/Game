#include "Menu.h"
#include "AssetManager.h"
#include "Audio.h"
#include "Boss.h"
#include "Graphics.h"
#include "Input.h"
#include "ParticleSystem.h"

namespace rayman {

namespace {
const sf::Color kGold(255, 226, 130);
const sf::Color kText(240, 232, 255);
const sf::Color kDim(190, 175, 220);
}

const char* enemyDisplayName(int world, int type) {
    static const char* n[5][6] = {
        {"Mossling", "Pollen Moth", "Barkback", "Puffcap", "Thornboar", "Seedspitter"},
        {"Drumbug", "Flutternote", "Tuba Turtle", "Cymbomb", "Bass Bull", "Horn Turret"},
        {"Glitchbot", "Neon Drone", "Riot Cube", "Volt Bomb", "Turbo Rat", "Laser Eye"},
        {"Snowpup", "Frost Bat", "Glacier Crab", "Snowbomb", "Yakcharger", "Icicle Mage"},
        {"Ember Imp", "Ash Wraith", "Magma Golem", "Lava Blob", "Fire Hound", "Pyre Totem"}};
    if (world < 1 || world > 5 || type < 0 || type > 5) return "?";
    return n[world - 1][type];
}

void drawMenuBackdrop(sf::RenderTarget& t, const AssetManager& a, float time, int world, float dim) {
    const sf::View old = t.getView();
    t.setView(t.getDefaultView());
    const float cx = kViewW * 0.5f + time * 40.f, cy = kViewH * 0.5f;
    sf::Sprite sky(a.texture("bg" + toStr(world) + "_0"));
    t.draw(sky);
    const sf::View v(sf::FloatRect(cx - kViewW * 0.5f, 0.f, static_cast<float>(kViewW), static_cast<float>(kViewH)));
    t.setView(v);
    drawParallax(t, a.texture("bg" + toStr(world) + "_1"), cx, cy, 0.15f, 0.f, cy);
    drawParallax(t, a.texture("bg" + toStr(world) + "_2"), cx, cy, 0.35f, 0.f, cy);
    drawParallax(t, a.texture("fog"), cx, cy, 0.5f, 0.f, cy - 300.f, time * 25.f, sf::Color(255, 255, 255, 80));
    drawParallax(t, a.texture("bg" + toStr(world) + "_3"), cx, cy, 0.7f, 0.f, cy);
    t.setView(t.getDefaultView());
    if (dim > 0.f) {
        sf::RectangleShape r(sf::Vector2f(static_cast<float>(kViewW), static_cast<float>(kViewH)));
        r.setFillColor(sf::Color(14, 6, 30, static_cast<sf::Uint8>(dim * 255.f)));
        t.draw(r);
    }
    t.setView(old);
}

// ---------------------------------------------------------------- ListMenu
void ListMenu::setItems(const std::vector<MenuItem>& items) {
    m_items = items;
    m_sel = 0;
    while (m_sel < static_cast<int>(m_items.size()) && !m_items[static_cast<size_t>(m_sel)].enabled) ++m_sel;
    if (m_sel >= static_cast<int>(m_items.size())) m_sel = 0;
}

void ListMenu::setSelected(int i) {
    if (i >= 0 && i < static_cast<int>(m_items.size()) && m_items[static_cast<size_t>(i)].enabled) m_sel = i;
}

void ListMenu::move(int d, Audio& audio) {
    if (m_items.empty()) return;
    const int n = static_cast<int>(m_items.size());
    for (int k = 0; k < n; ++k) {
        m_sel = (m_sel + d + n) % n;
        if (m_items[static_cast<size_t>(m_sel)].enabled) break;
    }
    audio.play("menu_move", 0.7f);
}

int ListMenu::update(float dt, const Input& in, Audio& audio, sf::Vector2f origin, float spacing, float width) {
    (void)dt;
    if (m_items.empty()) return -1;
    if (in.pressed(Action::Up)) move(-1, audio);
    if (in.pressed(Action::Down)) move(1, audio);
    for (size_t i = 0; i < m_items.size(); ++i) {
        const sf::FloatRect r(origin.x - width * 0.5f, origin.y + static_cast<float>(i) * spacing - 28.f, width, 56.f);
        if (r.contains(in.mouse())) {
            if (in.mouseMovedSinceFrame() && static_cast<int>(i) != m_sel && m_items[i].enabled) {
                m_sel = static_cast<int>(i);
                audio.play("menu_move", 0.5f);
            }
            if (in.mouseClicked() && m_items[i].enabled) {
                m_sel = static_cast<int>(i);
                audio.play("menu_select");
                return m_sel;
            }
        }
    }
    if (in.pressed(Action::Confirm) || in.pressed(Action::Jump)) {
        if (m_items[static_cast<size_t>(m_sel)].enabled) {
            audio.play("menu_select");
            return m_sel;
        }
    }
    return -1;
}

void ListMenu::draw(sf::RenderTarget& t, const AssetManager& a, sf::Vector2f origin, float spacing, float time, float width, float appear) const {
    for (size_t i = 0; i < m_items.size(); ++i) {
        const float k = easeOutBack(clampf(appear * 1.6f - static_cast<float>(i) * 0.08f, 0.f, 1.f));
        const sf::Vector2f p(origin.x + (1.f - k) * 400.f, origin.y + static_cast<float>(i) * spacing);
        ui::button(t, a, m_items[i].label, p, static_cast<int>(i) == m_sel, m_items[i].enabled, time, width);
    }
}

// ---------------------------------------------------------------- MainMenu
void MainMenu::setup(const AssetManager& a) {
    m_hero.setup(&a.texture("lumo"), &a.anim("lumo"));
    m_hero.play("idle");
}

void MainMenu::open(bool hasSave) {
    m_t = 0.f;
    m_list.setItems({{"NEW GAME", true}, {"CONTINUE", hasSave}, {"LEVEL SELECT", hasSave}, {"OPTIONS", true},
                     {"EXTRAS", true}, {"CREDITS", true}, {"QUIT GAME", true}});
    if (hasSave) m_list.setSelected(1);
}

MenuAction MainMenu::update(float dt, const Input& in, Audio& audio, ParticleSystem& fx) {
    m_t += dt;
    m_hero.update(dt);
    // the hero occasionally waves (attack pose) then returns to idle
    const float cycle = std::fmod(m_t, 7.f);
    if (cycle < dt) m_hero.play("attack", true);
    if (m_hero.current() == "attack" && m_hero.finished()) m_hero.play("idle");
    fx.ambient(AmbientKind::MenuDust, sf::FloatRect(0.f, 0.f, static_cast<float>(kViewW), static_cast<float>(kViewH)), dt);
    if (m_t < 0.4f) return MenuAction::None;
    const int r = m_list.update(dt, in, audio, sf::Vector2f(940.f, 250.f), 66.f, 300.f);
    switch (r) {
    case 0: return MenuAction::NewGame;
    case 1: return MenuAction::Continue;
    case 2: return MenuAction::LevelSelect;
    case 3: return MenuAction::Options;
    case 4: return MenuAction::Extras;
    case 5: return MenuAction::Credits;
    case 6: return MenuAction::Quit;
    default: break;
    }
    if (in.pressed(Action::Back)) { m_list.setSelected(6); audio.play("menu_move"); }
    return MenuAction::None;
}

void MainMenu::draw(sf::RenderTarget& t, const AssetManager& a, const ParticleSystem& fx, float time) const {
    drawMenuBackdrop(t, a, time, 1, 0.15f);
    // floating island with the hero
    const float bob = std::sin(time * 1.2f) * 8.f;
    sf::ConvexShape island(6);
    island.setPoint(0, sf::Vector2f(170.f, 560.f + bob)); island.setPoint(1, sf::Vector2f(470.f, 560.f + bob));
    island.setPoint(2, sf::Vector2f(430.f, 610.f + bob)); island.setPoint(3, sf::Vector2f(350.f, 690.f + bob));
    island.setPoint(4, sf::Vector2f(280.f, 700.f + bob)); island.setPoint(5, sf::Vector2f(200.f, 615.f + bob));
    island.setFillColor(sf::Color(90, 60, 110));
    island.setOutlineThickness(3.f);
    island.setOutlineColor(sf::Color(30, 16, 40));
    t.draw(island);
    sf::RectangleShape grass(sf::Vector2f(300.f, 18.f));
    grass.setPosition(170.f, 552.f + bob);
    grass.setFillColor(sf::Color(120, 220, 120));
    t.draw(grass);
    const sf::Texture& deco = a.texture("deco1");
    for (int i = 0; i < 3; ++i) {
        sf::Sprite d(deco, sf::IntRect((i % 2) * 96, 0, 96, 96));
        d.setOrigin(48.f, 96.f);
        d.setPosition(200.f + static_cast<float>(i) * 115.f, 558.f + bob);
        d.setRotation(std::sin(time * 1.5f + static_cast<float>(i)) * 4.f);
        d.setScale(0.8f + 0.15f * static_cast<float>(i % 2), 0.8f + 0.15f * static_cast<float>(i % 2));
        t.draw(d);
    }
    m_hero.draw(t, sf::Vector2f(330.f, 566.f + bob), false, 2.0f);
    // lumlings orbiting the island
    const sf::Texture& lum = a.texture("lumling");
    for (int i = 0; i < 4; ++i) {
        const float ang = time * 0.8f + static_cast<float>(i) * 1.57f;
        sf::Sprite l(lum, sf::IntRect((static_cast<int>(time * 12.f) + i) % 6 * 40, 0, 40, 40));
        l.setOrigin(20.f, 20.f);
        l.setPosition(320.f + std::cos(ang) * 220.f, 470.f + std::sin(ang) * 60.f + bob);
        t.draw(l);
    }
    fx.draw(t);
    // logo
    const sf::Texture& logo = a.texture("logo");
    sf::Sprite ls(logo);
    ls.setOrigin(static_cast<float>(logo.getSize().x) * 0.5f, static_cast<float>(logo.getSize().y) * 0.5f);
    const float in = easeOutBack(clampf(m_t / 0.8f, 0.f, 1.f));
    ls.setPosition(330.f, 170.f + std::sin(time * 1.6f) * 6.f);
    ls.setScale(0.62f * in, 0.62f * in);
    t.draw(ls);
    m_list.draw(t, a, sf::Vector2f(940.f, 250.f), 66.f, time, 300.f, m_t);
    ui::text(t, a.font("body"), "Arrows/WASD + Enter  |  Mouse supported", 18, sf::Vector2f(940.f, 690.f), kDim, ui::Align::Center);
    ui::text(t, a.font("body"), "v1.0", 16, sf::Vector2f(20.f, 690.f), kDim);
}

// ---------------------------------------------------------------- OptionsMenu
void OptionsMenu::open(Settings& s, bool startOnControls) {
    m_s = &s;
    m_controls = startOnControls;
    m_waitingKey = -1;
    m_t = 0.f;
    rebuild();
}

static std::string pct(float v) { return toStr(static_cast<int>(v * 100.f + 0.5f)) + "%"; }

void OptionsMenu::rebuild() {
    const int keep = m_list.selected();
    std::vector<MenuItem> it;
    if (!m_controls) {
        const Resolution r = resolutionAt(m_s->resolution);
        it = {{"Master Volume  < " + pct(m_s->master) + " >", true},
              {"Music Volume  < " + pct(m_s->music) + " >", true},
              {"Effects Volume  < " + pct(m_s->sfx) + " >", true},
              {std::string("Display  < ") + (m_s->fullscreen ? "Fullscreen" : "Windowed") + " >", true},
              {"Resolution  < " + toStr(r.w) + "x" + toStr(r.h) + " >", !m_s->fullscreen},
              {std::string("VSync  < ") + (m_s->vsync ? "On" : "Off") + " >", true},
              {std::string("Show FPS  < ") + (m_s->showFps ? "On" : "Off") + " >", true},
              {"Keyboard Controls", true},
              {"Reset Settings", true},
              {"Back", true}};
    } else {
        for (int i = 0; i < kRebindableCount; ++i) {
            const std::string k = (m_waitingKey == i) ? "Press a key..." : keyName(m_s->keys[static_cast<size_t>(i)]);
            it.push_back({std::string(actionName(static_cast<Action>(i))) + ":  " + k, true});
        }
        it.push_back({"Reset Keys", true});
        it.push_back({"Back", true});
    }
    m_list.setItems(it);
    m_list.setSelected(std::min(keep, static_cast<int>(it.size()) - 1));
}

OptionsResult OptionsMenu::update(float dt, Input& in, Audio& audio) {
    OptionsResult res;
    m_t += dt;
    if (!m_s) { res.back = true; return res; }
    if (m_waitingKey >= 0) {
        const int k = in.takeLastKey();
        if (k == sf::Keyboard::Escape) { m_waitingKey = -1; rebuild(); audio.play("menu_back"); }
        else if (k >= 0) {
            m_s->keys[static_cast<size_t>(m_waitingKey)] = k;
            m_waitingKey = -1;
            res.keysChanged = true;
            audio.play("menu_select");
            rebuild();
        }
        return res;
    }
    const sf::Vector2f origin(640.f, 170.f);
    const float spacing = m_controls ? 54.f : 50.f;
    const int sel = m_list.selected();
    int dir = 0;
    if (in.pressed(Action::Left)) dir = -1;
    if (in.pressed(Action::Right)) dir = 1;
    if (!m_controls && dir != 0) {
        switch (sel) {
        case 0: m_s->master = clampf(m_s->master + 0.1f * static_cast<float>(dir), 0.f, 1.f); res.audioChanged = true; break;
        case 1: m_s->music = clampf(m_s->music + 0.1f * static_cast<float>(dir), 0.f, 1.f); res.audioChanged = true; break;
        case 2: m_s->sfx = clampf(m_s->sfx + 0.1f * static_cast<float>(dir), 0.f, 1.f); res.audioChanged = true; break;
        case 3: m_s->fullscreen = !m_s->fullscreen; res.videoChanged = true; break;
        case 4: m_s->resolution = (m_s->resolution + dir + kResolutionCount) % kResolutionCount; res.videoChanged = true; break;
        case 5: m_s->vsync = !m_s->vsync; res.videoChanged = true; break;
        case 6: m_s->showFps = !m_s->showFps; break;
        default: dir = 0; break;
        }
        if (dir != 0) { audio.play("menu_move"); rebuild(); }
    }
    const int r = m_list.update(dt, in, audio, origin, spacing, 560.f);
    if (r >= 0) {
        if (!m_controls) {
            switch (r) {
            case 3: m_s->fullscreen = !m_s->fullscreen; res.videoChanged = true; break;
            case 4: m_s->resolution = (m_s->resolution + 1) % kResolutionCount; res.videoChanged = true; break;
            case 5: m_s->vsync = !m_s->vsync; res.videoChanged = true; break;
            case 6: m_s->showFps = !m_s->showFps; break;
            case 7: m_controls = true; m_list.setSelected(0); break;
            case 8: {
                const auto keys = m_s->keys;
                *m_s = Settings();
                m_s->keys = keys;
                res.audioChanged = res.videoChanged = true;
                break;
            }
            case 9: res.back = true; return res;
            default: break;
            }
            rebuild();
            if (r == 7) m_list.setSelected(0);
        } else {
            if (r < kRebindableCount) { m_waitingKey = r; in.takeLastKey(); }
            else if (r == kRebindableCount) { m_s->keys = defaultBindings(); res.keysChanged = true; }
            else { m_controls = false; rebuild(); m_list.setSelected(7); return res; }
            rebuild();
        }
    }
    if (in.pressed(Action::Back) && m_waitingKey < 0) {
        audio.play("menu_back");
        if (m_controls) { m_controls = false; rebuild(); m_list.setSelected(7); }
        else res.back = true;
    }
    return res;
}

void OptionsMenu::draw(sf::RenderTarget& t, const AssetManager& a, float time) const {
    ui::panel(t, a.texture("panel"), sf::FloatRect(300.f, 40.f, 680.f, 640.f), 0.95f);
    ui::text(t, a.font("display"), m_controls ? "KEYBOARD CONTROLS" : "OPTIONS", 40, sf::Vector2f(640.f, 62.f), kGold, ui::Align::Center, 3.f);
    m_list.draw(t, a, sf::Vector2f(640.f, 170.f), m_controls ? 54.f : 50.f, time, 560.f, 1.f);
    const char* hint = m_controls ? "Enter: rebind  |  Esc: back  (arrows/WASD always work in menus)" : "Left/Right to adjust  |  Esc: back";
    ui::text(t, a.font("body"), hint, 17, sf::Vector2f(640.f, 652.f), kDim, ui::Align::Center);
}

// ---------------------------------------------------------------- Extras
void ExtrasMenu::open(const SaveData& save) {
    m_save = &save;
    m_tab = 0;
    m_sel = 0;
    m_t = 0.f;
}

bool ExtrasMenu::update(float dt, const Input& in, Audio& audio) {
    m_t += dt;
    const int tabs = 4;
    if (in.pressed(Action::Back)) { audio.play("menu_back"); return true; }
    // tab switching with Up/Down at the tab row is awkward; use Left/Right for tabs except in the gallery grid
    if (m_tab == 0) {
        const int cols = 8, count = 30 + 5 + 5 + 5;
        if (in.pressed(Action::Right)) { m_sel = (m_sel + 1) % count; audio.play("menu_move", 0.5f); }
        if (in.pressed(Action::Left)) { m_sel = (m_sel - 1 + count) % count; audio.play("menu_move", 0.5f); }
        if (in.pressed(Action::Down)) { m_sel = std::min(count - 1, m_sel + cols); audio.play("menu_move", 0.5f); }
        if (in.pressed(Action::Up)) { m_sel = std::max(0, m_sel - cols); audio.play("menu_move", 0.5f); }
    }
    if (in.pressed(Action::Confirm) || in.pressed(Action::Jump) || (m_tab != 0 && (in.pressed(Action::Right) || in.pressed(Action::Left)))) {
        const int d = in.pressed(Action::Left) ? -1 : 1;
        m_tab = (m_tab + d + tabs) % tabs;
        audio.play("menu_select");
    }
    for (int i = 0; i < tabs; ++i) {
        const sf::FloatRect r(150.f + static_cast<float>(i) * 250.f, 96.f, 230.f, 44.f);
        if (in.mouseClicked() && r.contains(in.mouse())) { m_tab = i; audio.play("menu_select"); }
    }
    return false;
}

void ExtrasMenu::draw(sf::RenderTarget& t, const AssetManager& a, float time) const {
    if (!m_save) return;
    const SaveData& s = *m_save;
    ui::panel(t, a.texture("panel"), sf::FloatRect(80.f, 30.f, 1120.f, 660.f), 0.96f);
    ui::text(t, a.font("display"), "EXTRAS", 36, sf::Vector2f(640.f, 44.f), kGold, ui::Align::Center, 3.f);
    const char* tabs[4] = {"Gallery", "Lumo", "Achievements", "Statistics"};
    for (int i = 0; i < 4; ++i) {
        const sf::FloatRect r(150.f + static_cast<float>(i) * 250.f, 96.f, 230.f, 44.f);
        sf::RectangleShape bg(sf::Vector2f(r.width, r.height));
        bg.setPosition(r.left, r.top);
        bg.setFillColor(i == m_tab ? sf::Color(255, 120, 210, 200) : sf::Color(60, 40, 110, 200));
        bg.setOutlineThickness(2.f);
        bg.setOutlineColor(sf::Color(255, 220, 140));
        t.draw(bg);
        ui::text(t, a.font("ui"), tabs[i], 22, sf::Vector2f(r.left + r.width * 0.5f, r.top + 8.f), i == m_tab ? sf::Color::White : kDim, ui::Align::Center);
    }
    const sf::Font& body = a.font("body");
    const sf::Font& uiF = a.font("ui");
    if (m_tab == 0) {
        // 30 enemies, 5 bosses, 5 relics, 5 concept-art backgrounds
        const int cols = 8;
        for (int i = 0; i < 45; ++i) {
            const float x = 150.f + static_cast<float>(i % cols) * 122.f, y = 170.f + static_cast<float>(i / cols) * 74.f;
            sf::RectangleShape cell(sf::Vector2f(110.f, 66.f));
            cell.setPosition(x, y);
            cell.setFillColor(i == m_sel ? sf::Color(255, 120, 210, 120) : sf::Color(20, 10, 40, 160));
            cell.setOutlineThickness(i == m_sel ? 3.f : 1.f);
            cell.setOutlineColor(i == m_sel ? kGold : sf::Color(140, 100, 200));
            t.draw(cell);
            bool unlocked = false;
            if (i < 30) {
                unlocked = (s.enemiesSeen >> i) & 1u;
                const int w = i / 6 + 1, ty = i % 6;
                const char* keys[6] = {"walker", "flyer", "armored", "exploder", "charger", "shooter"};
                sf::Sprite sp(a.texture("w" + toStr(w) + "_" + keys[ty]), sf::IntRect((static_cast<int>(time * 8.f) % 6) * 64, 64, 64, 64));
                sp.setOrigin(32.f, 32.f);
                sp.setPosition(x + 55.f, y + 33.f);
                if (!unlocked) sp.setColor(sf::Color(0, 0, 0, 160));
                t.draw(sp);
            } else if (i < 35) {
                const int w = i - 30 + 1;
                unlocked = s.bossDefeated[static_cast<size_t>(w - 1)];
                sf::Sprite sp(a.texture("boss" + toStr(w)), sf::IntRect(0, 0, 192, 192));
                sp.setOrigin(96.f, 96.f);
                sp.setScale(0.33f, 0.33f);
                sp.setPosition(x + 55.f, y + 33.f);
                if (!unlocked) sp.setColor(sf::Color(0, 0, 0, 160));
                t.draw(sp);
            } else if (i < 40) {
                const int w = i - 35 + 1;
                int got = 0;
                for (int st = 1; st <= kStagesPerWorld; ++st) got += s.stage(w, st).bestRelics;
                unlocked = got > 0;
                sf::Sprite sp(a.texture("relics"), sf::IntRect((w - 1) * 48, 0, 48, 48));
                sp.setOrigin(24.f, 24.f);
                sp.setPosition(x + 55.f, y + 33.f);
                if (!unlocked) sp.setColor(sf::Color(0, 0, 0, 160));
                t.draw(sp);
            } else {
                const int w = i - 40 + 1;
                unlocked = s.stage(w, 1).unlocked;
                const sf::Texture& bg = a.texture("bg" + toStr(w) + "_2");
                sf::Sprite sp(bg);
                sp.setScale(104.f / static_cast<float>(bg.getSize().x), 60.f / static_cast<float>(bg.getSize().y));
                sp.setPosition(x + 3.f, y + 3.f);
                if (!unlocked) sp.setColor(sf::Color(0, 0, 0, 200));
                t.draw(sp);
            }
            if (!unlocked) ui::icon(t, a.texture("icons"), 6, sf::Vector2f(x + 92.f, y + 16.f), 24.f);
        }
        // caption
        std::string cap;
        const int i = m_sel;
        if (i < 30) cap = ((s.enemiesSeen >> i) & 1u) ? std::string(enemyDisplayName(i / 6 + 1, i % 6)) + "  (World " + toStr(i / 6 + 1) + ")" : "??? - defeat this creature to unlock";
        else if (i < 35) cap = s.bossDefeated[static_cast<size_t>(i - 30)] ? std::string(bossName(i - 29)) + "  - World " + toStr(i - 29) + " guardian" : "??? - defeat this boss to unlock";
        else if (i < 40) {
            const char* rel[5] = {"Moon Petal", "Golden Note", "Neon Chip", "Frost Crystal", "Ember Heart"};
            cap = std::string("Relic: ") + rel[i - 35] + " - a fragment of the Shattered Dream";
        } else {
            const char* wn[5] = {"Dream Forest", "Band Land", "Picture City", "Frozen Kingdom", "Volcano of Shadows"};
            cap = std::string("Concept art: ") + wn[i - 40];
        }
        ui::text(t, uiF, cap, 22, sf::Vector2f(640.f, 630.f), kText, ui::Align::Center);
    } else if (m_tab == 1) {
        sf::Sprite p(a.texture("lumo_portrait"));
        p.setPosition(150.f, 180.f);
        p.setScale(1.3f, 1.3f);
        t.draw(p);
        ui::text(t, a.font("display"), "LUMO, THE WISP-KEEPER", 32, sf::Vector2f(560.f, 180.f), kGold);
        const char* lines[] = {
            "A limbless dreamling whose gloves and shoes float wherever",
            "his heart leads. His hood ends in a fallen star that hums",
            "when Lumlings are near.",
            "",
            "When Umbra, the Ember King, shattered the Great Dream into",
            "five shards, the Lumlings were caged and the worlds fell silent.",
            "Lumo set out with nothing but a magic fist and a stubborn grin.",
            "",
            "Abilities: Magic Fist (hold to charge), Helicopter Hood (hold",
            "Jump while falling), Swimming, and - once found - Double Jump."};
        float y = 240.f;
        for (const char* l : lines) { ui::text(t, body, l, 20, sf::Vector2f(560.f, y), kText); y += 30.f; }
    } else if (m_tab == 2) {
        for (int i = 0; i < AchCount; ++i) {
            const float x = 140.f + static_cast<float>(i % 2) * 520.f, y = 168.f + static_cast<float>(i / 2) * 78.f;
            const bool got = s.hasAchievement(i);
            ui::icon(t, a.texture("icons"), got ? 11 : 6, sf::Vector2f(x + 26.f, y + 28.f), 44.f, got ? 1.f : 0.5f);
            ui::text(t, uiF, achievementName(i), 22, sf::Vector2f(x + 60.f, y + 4.f), got ? kGold : kDim);
            ui::text(t, body, achievementDesc(i), 17, sf::Vector2f(x + 60.f, y + 34.f), got ? kText : sf::Color(150, 140, 170));
        }
    } else {
        const Stats& st = s.stats;
        int stages = 0, stars = 0;
        for (int w = 1; w <= kWorlds; ++w)
            for (int k = 1; k <= kStagesPerWorld; ++k) { stages += s.stage(w, k).completed ? 1 : 0; stars += s.stage(w, k).stars; }
        const std::string rows[][2] = {
            {"Stages completed", toStr(stages) + " / 35"}, {"Stars earned", toStr(stars) + " / 105"},
            {"Overall completion", toStr(static_cast<int>(s.totalCompletion() * 100.f)) + "%"}, {"Tings collected", toStr(st.totalTings)},
            {"Lumlings freed", toStr(st.lumlingsFreed)}, {"Dream Gems", toStr(st.gemsCollected)}, {"Relics", toStr(st.relicsCollected)},
            {"Secrets discovered", toStr(st.secretsFound)}, {"Enemies defeated", toStr(st.enemiesDefeated)},
            {"Bosses defeated", toStr(st.bossesDefeated)}, {"Jumps", toStr(st.jumps)}, {"Deaths", toStr(st.deaths)},
            {"Play time", formatTime(st.playTime)}};
        float y = 166.f;
        for (const auto& r : rows) {
            ui::text(t, uiF, r[0], 22, sf::Vector2f(340.f, y), kText);
            ui::text(t, uiF, r[1], 22, sf::Vector2f(940.f, y), kGold, ui::Align::Right);
            y += 36.f;
        }
    }
    ui::text(t, body, m_tab == 0 ? "Arrows: browse  |  Enter: next tab  |  Esc: back" : "Left/Right or Enter: switch tab  |  Esc: back", 17,
             sf::Vector2f(640.f, 662.f), kDim, ui::Align::Center);
}

// ---------------------------------------------------------------- Credits
bool CreditsScreen::update(float dt, const Input& in) {
    m_scroll += dt * (in.held(Action::Down) || in.held(Action::Jump) ? 140.f : 45.f);
    if (in.pressed(Action::Back) || in.pressed(Action::Confirm) || in.mouseClicked()) return true;
    return m_scroll > 1500.f;
}

void CreditsScreen::draw(sf::RenderTarget& t, const AssetManager& a, float time) const {
    (void)time;
    static const char* lines[] = {
        "#LUMO AND THE SHATTERED DREAM", "", "@Game Design, Programming & Level Design", "ClickUp Brain dev team", "",
        "@Engine", "C++17 + SFML 2.6.1 (zlib/png license)", "", "@Character, Enemy & Boss Art", "Original procedural art (CC0)",
        "generated by tools/gen_hero.py and tools/gen_creatures.py", "", "@Environments, Tiles, World Map & UI",
        "Original procedural art (CC0) - tools/gen_world.py, gen_objects.py", "", "@Music & Sound Effects",
        "Original synthesized score and SFX (CC0) - tools/gen_audio.py", "", "@Fonts", "Noto Sans & Noto Sans Display",
        "Copyright Google LLC - SIL Open Font License 1.1", "", "@Inspired by", "The joy of classic 2D platformers and adventure maps", "",
        "@Special Thanks", "Every Lumling waiting in a cage", "", "See ASSET_CREDITS.md for full attribution", "", "", "#Thank you for playing!"};
    float y = 720.f - m_scroll;
    for (const char* l : lines) {
        std::string s(l);
        if (!s.empty() && s[0] == '#') ui::text(t, a.font("display"), s.substr(1), 40, sf::Vector2f(640.f, y), kGold, ui::Align::Center, 3.f);
        else if (!s.empty() && s[0] == '@') ui::text(t, a.font("ui"), s.substr(1), 26, sf::Vector2f(640.f, y), sf::Color(255, 160, 220), ui::Align::Center);
        else ui::text(t, a.font("body"), s, 22, sf::Vector2f(640.f, y), kText, ui::Align::Center);
        y += (!s.empty() && s[0] == '#') ? 64.f : 36.f;
    }
}

// ---------------------------------------------------------------- Pause
void PauseMenu::open() {
    m_controls = false;
    m_t = 0.f;
    m_list.setItems({{"Resume", true}, {"Restart Level", true}, {"Options", true}, {"Controls", true},
                     {"Return to World Map", true}, {"Quit to Main Menu", true}});
}

MenuAction PauseMenu::update(float dt, const Input& in, Audio& audio) {
    m_t += dt;
    if (m_controls) {
        if (in.pressed(Action::Back) || in.pressed(Action::Confirm) || in.mouseClicked()) { m_controls = false; audio.play("menu_back"); }
        return MenuAction::None;
    }
    if (in.pressed(Action::Back) || in.pressed(Action::Pause)) { audio.play("menu_back"); return MenuAction::Resume; }
    switch (m_list.update(dt, in, audio, sf::Vector2f(640.f, 230.f), 64.f, 380.f)) {
    case 0: return MenuAction::Resume;
    case 1: return MenuAction::Restart;
    case 2: return MenuAction::Options;
    case 3: m_controls = true; return MenuAction::None;
    case 4: return MenuAction::ToMap;
    case 5: return MenuAction::ToMainMenu;
    default: return MenuAction::None;
    }
}

void PauseMenu::draw(sf::RenderTarget& t, const AssetManager& a, const Input& in, float time) const {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(kViewW), static_cast<float>(kViewH)));
    dim.setFillColor(sf::Color(10, 4, 24, 150));
    t.draw(dim);
    ui::panel(t, a.texture("panel"), sf::FloatRect(390.f, 90.f, 500.f, 560.f), 0.96f);
    ui::text(t, a.font("display"), m_controls ? "CONTROLS" : "PAUSED", 44, sf::Vector2f(640.f, 112.f), kGold, ui::Align::Center, 3.f);
    if (!m_controls) {
        m_list.draw(t, a, sf::Vector2f(640.f, 230.f), 64.f, time, 380.f, 1.f);
        return;
    }
    const auto& b = in.bindings();
    float y = 200.f;
    for (int i = 0; i < kRebindableCount; ++i) {
        ui::text(t, a.font("ui"), actionName(static_cast<Action>(i)), 22, sf::Vector2f(430.f, y), kText);
        ui::text(t, a.font("ui"), keyName(b[static_cast<size_t>(i)]), 22, sf::Vector2f(850.f, y), kGold, ui::Align::Right);
        y += 44.f;
    }
    ui::text(t, a.font("body"), "Also: WASD move, Z/K jump, J attack, Esc/P pause", 18, sf::Vector2f(640.f, y + 10.f), kDim, ui::Align::Center);
    ui::text(t, a.font("body"), "Hold Attack to charge  |  Hold Jump while falling to glide", 18, sf::Vector2f(640.f, y + 40.f), kDim, ui::Align::Center);
}

// ---------------------------------------------------------------- Confirm
void ConfirmDialog::open(const std::string& question, const std::string& detail) {
    m_q = question;
    m_d = detail;
    m_t = 0.f;
    m_list.setItems({{"Yes", true}, {"No", true}});
    m_list.setSelected(1);
}

MenuAction ConfirmDialog::update(float dt, const Input& in, Audio& audio) {
    m_t += dt;
    if (in.pressed(Action::Back)) { audio.play("menu_back"); return MenuAction::No; }
    const int r = m_list.update(dt, in, audio, sf::Vector2f(640.f, 400.f), 70.f, 260.f);
    if (r == 0) return MenuAction::Yes;
    if (r == 1) return MenuAction::No;
    return MenuAction::None;
}

void ConfirmDialog::draw(sf::RenderTarget& t, const AssetManager& a, float time) const {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(kViewW), static_cast<float>(kViewH)));
    dim.setFillColor(sf::Color(10, 4, 24, 170));
    t.draw(dim);
    const float k = easeOutBack(clampf(m_t / 0.3f, 0.f, 1.f));
    ui::panel(t, a.texture("panel"), sf::FloatRect(340.f, 200.f + (1.f - k) * 40.f, 600.f, 320.f), k);
    ui::text(t, a.font("display"), m_q, 32, sf::Vector2f(640.f, 236.f), kGold, ui::Align::Center, 3.f, sf::Color(30, 16, 50), k);
    ui::text(t, a.font("body"), m_d, 20, sf::Vector2f(640.f, 296.f), kText, ui::Align::Center, 2.f, sf::Color(30, 16, 50), k);
    m_list.draw(t, a, sf::Vector2f(640.f, 400.f), 70.f, time, 260.f, m_t * 2.f);
}

// ---------------------------------------------------------------- Level complete
void LevelCompleteScreen::open(const StageResults& r) {
    m_r = r;
    m_t = 0.f;
    m_starsShown = 0;
    if (r.finalBoss) m_list.setItems({{"Continue", true}});
    else m_list.setItems({{"Continue", true}, {"Return to World Map", true}});
}

MenuAction LevelCompleteScreen::update(float dt, const Input& in, Audio& audio) {
    m_t += dt;
    const int stars = std::min(m_r.stars, static_cast<int>((m_t - 1.6f) / 0.45f) + 1);
    if (m_t > 1.6f && stars > m_starsShown) {
        m_starsShown = stars;
        audio.play("gem", 1.f, 0.f, 0.9f + 0.15f * static_cast<float>(stars));
    }
    if (m_t < 1.0f) {
        if (in.pressed(Action::Confirm) || in.pressed(Action::Jump)) m_t = 3.f;  // skip the tally
        return MenuAction::None;
    }
    const int r = m_list.update(dt, in, audio, sf::Vector2f(640.f, 600.f), 64.f, 380.f);
    if (r == 0) return MenuAction::Next;
    if (r == 1) return MenuAction::ToMap;
    return MenuAction::None;
}

void LevelCompleteScreen::draw(sf::RenderTarget& t, const AssetManager& a, float time) const {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(kViewW), static_cast<float>(kViewH)));
    dim.setFillColor(sf::Color(10, 4, 24, 175));
    t.draw(dim);
    const float k = easeOutBack(clampf(m_t / 0.5f, 0.f, 1.f));
    ui::panel(t, a.texture("panel"), sf::FloatRect(240.f, 24.f + (1.f - k) * 60.f, 800.f, 520.f), k);
    ui::text(t, a.font("display"), m_r.boss ? "BOSS DEFEATED!" : "STAGE CLEAR!", 46, sf::Vector2f(640.f, 40.f), kGold, ui::Align::Center, 3.f);
    ui::text(t, a.font("ui"), toStr(m_r.world) + "-" + toStr(m_r.stage) + "  " + m_r.name, 22, sf::Vector2f(640.f, 102.f), kText, ui::Align::Center);
    // stars
    for (int i = 0; i < 3; ++i) {
        const bool lit = i < m_starsShown;
        const float pop = lit ? easeOutBack(clampf((m_t - 1.6f - 0.45f * static_cast<float>(i)) / 0.35f, 0.f, 1.f)) : 1.f;
        ui::icon(t, a.texture("icons"), lit ? 4 : 5, sf::Vector2f(560.f + static_cast<float>(i) * 80.f, 166.f), 64.f * (lit ? pop : 1.f),
                 1.f, lit ? std::sin(time * 3.f + static_cast<float>(i)) * 6.f : 0.f);
    }
    struct Row { std::string label, value; };
    const Row rows[] = {
        {"Tings", toStr(m_r.tings) + " / " + toStr(m_r.tingsTotal) + (m_r.tings < m_r.tingsTotal ? "   (missed " + toStr(m_r.tingsTotal - m_r.tings) + ")" : "")},
        {"Lumlings freed", toStr(m_r.cages) + " / " + toStr(m_r.cagesTotal)},
        {"Dream Gems", toStr(m_r.gems) + " / " + toStr(m_r.gemsTotal)},
        {"Relics / Secrets", toStr(m_r.relics) + "/" + toStr(m_r.relicsTotal) + "   " + toStr(m_r.secrets) + "/" + toStr(m_r.secretsTotal)},
        {"Enemies defeated", toStr(m_r.enemies)},
        {"Time  (par " + formatTime(m_r.par) + ")", formatTime(m_r.time)},
        {"Deaths", toStr(m_r.deaths)},
        {"Completion", toStr(static_cast<int>(m_r.completion * 100.f + 0.5f)) + "%   Rank " + m_r.rank}};
    float y = 212.f;
    int idx = 0;
    for (const Row& r : rows) {
        const float a2 = clampf((m_t - 0.4f - 0.12f * static_cast<float>(idx)) / 0.25f, 0.f, 1.f);
        ui::text(t, a.font("ui"), r.label, 20, sf::Vector2f(300.f, y), kText, ui::Align::Left, 2.f, sf::Color(30, 16, 50), a2);
        ui::text(t, a.font("ui"), r.value, 20, sf::Vector2f(980.f, y), kGold, ui::Align::Right, 2.f, sf::Color(30, 16, 50), a2);
        y += 32.f;
        ++idx;
    }
    const int shownScore = static_cast<int>(static_cast<float>(m_r.score) * clampf((m_t - 0.5f) / 1.0f, 0.f, 1.f));
    ui::text(t, a.font("display"), "SCORE  " + padNumber(shownScore, 7) + (m_r.newBest && m_t > 1.6f ? "   NEW BEST!" : ""), 30,
             sf::Vector2f(640.f, 474.f), sf::Color(255, 250, 220), ui::Align::Center, 3.f);
    if (!m_r.rewards.empty()) {
        std::string rw = "Rewards: ";
        for (size_t i = 0; i < m_r.rewards.size(); ++i) rw += (i ? ", " : "") + m_r.rewards[i];
        ui::text(t, a.font("body"), rw, 18, sf::Vector2f(640.f, 514.f), sf::Color(160, 255, 200), ui::Align::Center);
    }
    if (m_t > 1.0f) m_list.draw(t, a, sf::Vector2f(640.f, 600.f), 64.f, time, 380.f, (m_t - 1.0f) * 2.f);
}

// ---------------------------------------------------------------- Game over
void GameOverScreen::open(int score, const std::string& stage) {
    m_score = score;
    m_stage = stage;
    m_t = 0.f;
    m_list.setItems({{"Restart", true}, {"Return to World Map", true}});
}

MenuAction GameOverScreen::update(float dt, const Input& in, Audio& audio) {
    m_t += dt;
    if (m_t < 1.2f) return MenuAction::None;
    const int r = m_list.update(dt, in, audio, sf::Vector2f(640.f, 470.f), 70.f, 380.f);
    if (r == 0) return MenuAction::RetryLevel;
    if (r == 1) return MenuAction::ToMap;
    return MenuAction::None;
}

void GameOverScreen::draw(sf::RenderTarget& t, const AssetManager& a, float time) const {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(kViewW), static_cast<float>(kViewH)));
    dim.setFillColor(sf::Color(20, 0, 20, static_cast<sf::Uint8>(clampf(m_t, 0.f, 1.f) * 210.f)));
    t.draw(dim);
    const float k = easeOutBack(clampf((m_t - 0.3f) / 0.6f, 0.f, 1.f));
    ui::text(t, a.font("display"), "GAME OVER", static_cast<unsigned>(80.f * std::max(0.1f, k)), sf::Vector2f(640.f, 170.f), sf::Color(255, 110, 140),
             ui::Align::Center, 4.f);
    ui::icon(t, a.texture("icons"), 13, sf::Vector2f(640.f, 320.f + std::sin(time * 2.f) * 6.f), 72.f, clampf(m_t, 0.f, 1.f));
    ui::text(t, a.font("ui"), m_stage, 24, sf::Vector2f(640.f, 270.f), kText, ui::Align::Center, 2.f, sf::Color(30, 16, 50), k);
    ui::text(t, a.font("ui"), "Final score  " + padNumber(m_score, 7), 28, sf::Vector2f(640.f, 372.f), kGold, ui::Align::Center, 2.f, sf::Color(30, 16, 50), k);
    if (m_t > 1.2f) m_list.draw(t, a, sf::Vector2f(640.f, 470.f), 70.f, time, 380.f, (m_t - 1.2f) * 2.f);
}

// ---------------------------------------------------------------- Victory
void VictoryScreen::setup(const AssetManager& a) {
    m_hero.setup(&a.texture("lumo"), &a.anim("lumo"));
    m_hero.play("run");
}

void VictoryScreen::open(const SaveData& save) {
    m_t = 0.f;
    m_score = 0;
    m_worlds = 0;
    for (int w = 1; w <= kWorlds; ++w) {
        if (save.bossDefeated[static_cast<size_t>(w - 1)]) ++m_worlds;
        for (int s = 1; s <= kStagesPerWorld; ++s) m_score += save.stage(w, s).bestScore;
    }
    m_tings = static_cast<int>(save.stats.totalTings);
    m_lumlings = save.stats.lumlingsFreed;
    m_gems = save.stats.gemsCollected;
    m_relics = save.stats.relicsCollected;
    m_completion = save.totalCompletion();
    m_list.setItems({{"Play Again", true}, {"Main Menu", true}});
    m_hero.play("run", true);
}

MenuAction VictoryScreen::update(float dt, const Input& in, Audio& audio, ParticleSystem& fx) {
    m_t += dt;
    m_hero.update(dt);
    if (m_t > 4.f && m_hero.current() == "run") m_hero.play("idle");
    if (static_cast<int>(m_t * 3.f) != static_cast<int>((m_t - dt) * 3.f)) {
        fx.sparkle(sf::Vector2f(globalRng().uniform(100.f, 1180.f), globalRng().uniform(60.f, 300.f)),
                   globalRng().chance(0.5f) ? sf::Color(255, 200, 120) : sf::Color(200, 160, 255), 14);
    }
    fx.ambient(AmbientKind::MenuDust, sf::FloatRect(0.f, 0.f, static_cast<float>(kViewW), static_cast<float>(kViewH)), dt);
    if (m_t < 5.5f) {
        if (in.pressed(Action::Confirm)) m_t = 5.5f;
        return MenuAction::None;
    }
    const int r = m_list.update(dt, in, audio, sf::Vector2f(640.f, 600.f), 66.f, 340.f);
    if (r == 0) return MenuAction::PlayAgain;
    if (r == 1) return MenuAction::ToMainMenu;
    return MenuAction::None;
}

void VictoryScreen::draw(sf::RenderTarget& t, const AssetManager& a, const ParticleSystem& fx, float time) const {
    drawMenuBackdrop(t, a, time * 0.5f, 1, 0.1f);
    // the Great Dream is restored: lumlings spiral around Lumo
    const float heroX = std::min(640.f, -100.f + m_t * 190.f);
    m_hero.draw(t, sf::Vector2f(heroX, 470.f), false, 2.2f);
    const sf::Texture& lum = a.texture("lumling");
    for (int i = 0; i < 14; ++i) {
        const float ang = time * (0.6f + 0.05f * static_cast<float>(i)) + static_cast<float>(i) * 0.45f;
        const float rad = 150.f + 40.f * std::sin(time + static_cast<float>(i));
        sf::Sprite l(lum, sf::IntRect(((static_cast<int>(time * 12.f) + i) % 6) * 40, 0, 40, 40));
        l.setOrigin(20.f, 20.f);
        l.setPosition(heroX + std::cos(ang) * rad, 330.f + std::sin(ang) * rad * 0.45f);
        l.setColor(withAlpha(sf::Color::White, clampf(m_t - 1.f - static_cast<float>(i) * 0.15f, 0.f, 1.f)));
        t.draw(l);
    }
    fx.draw(t);
    const float k = clampf((m_t - 0.5f) / 1.0f, 0.f, 1.f);
    ui::text(t, a.font("display"), "THE DREAM IS RESTORED!", 52, sf::Vector2f(640.f, 40.f), kGold, ui::Align::Center, 4.f, sf::Color(30, 16, 50), k);
    ui::text(t, a.font("body"), "Umbra's shadow fades, and every Lumling sings again.", 22, sf::Vector2f(640.f, 112.f), kText, ui::Align::Center, 2.f,
             sf::Color(30, 16, 50), k);
    const float k2 = clampf((m_t - 2.5f) / 1.0f, 0.f, 1.f);
    if (k2 > 0.f) {
        ui::panel(t, a.texture("panel"), sf::FloatRect(360.f, 150.f, 560.f, 64.f), k2 * 0.9f, 20.f);
        ui::text(t, a.font("ui"), "Total score  " + padNumber(m_score, 8), 26, sf::Vector2f(640.f, 166.f), kGold, ui::Align::Center, 2.f, sf::Color(30, 16, 50), k2);
        const std::string line = "Worlds " + toStr(m_worlds) + "/5    Tings " + toStr(m_tings) + "    Lumlings " + toStr(m_lumlings) + "    Gems " +
                                 toStr(m_gems) + "    Relics " + toStr(m_relics) + "    " + toStr(static_cast<int>(m_completion * 100.f)) + "% complete";
        ui::text(t, a.font("ui"), line, 20, sf::Vector2f(640.f, 520.f), kText, ui::Align::Center, 2.f, sf::Color(30, 16, 50), k2);
    }
    if (m_t > 5.5f) m_list.draw(t, a, sf::Vector2f(640.f, 600.f), 66.f, time, 340.f, (m_t - 5.5f) * 2.f);
}

} // namespace rayman
