#include "AssetManager.h"
#include "Utils.h"
#include <fstream>

namespace rayman {

namespace fs = std::filesystem;

AssetManager::AssetManager() {
    // Fallback texture: magenta/black checker so a missing sprite is obvious but never crashes.
    sf::Image img;
    img.create(32, 32, sf::Color::Magenta);
    for (unsigned y = 0; y < 32; ++y)
        for (unsigned x = 0; x < 32; ++x)
            if (((x / 8) + (y / 8)) % 2 == 0) img.setPixel(x, y, sf::Color(20, 0, 20));
    m_fallbackTex = std::make_unique<sf::Texture>();
    m_fallbackTex->loadFromImage(img);
    m_fallbackFont = std::make_unique<sf::Font>();
    m_fallbackAnim = std::make_unique<AnimationSet>();
    AnimClip c;
    m_fallbackAnim->addClip("idle", c);
    m_fallbackAnim->setFrameSize(sf::Vector2i(32, 32));
}

bool AssetManager::locateAssetRoot(const fs::path& exeDir) {
    std::error_code ec;
    const fs::path candidates[] = {exeDir / "assets", fs::current_path(ec) / "assets", exeDir.parent_path() / "assets",
                                   exeDir.parent_path().parent_path() / "assets"};
    for (const auto& c : candidates) {
        if (fs::exists(c / "fonts", ec)) {
            m_root = c;
            return true;
        }
    }
    m_root = exeDir / "assets";
    return false;
}

void AssetManager::queueManifest() {
    m_queue.clear();
    m_next = 0;
    add(Kind::Font, "display", "fonts/NotoSansDisplay-Bold.ttf", true);
    add(Kind::Font, "ui", "fonts/NotoSans-Bold.ttf", true);
    add(Kind::Font, "body", "fonts/NotoSans-Regular.ttf", true);
    add(Kind::Texture, "logo", "ui/logo.png");
    add(Kind::Texture, "icons", "ui/icons.png");
    add(Kind::Texture, "panel", "ui/panel.png");
    add(Kind::Texture, "button", "ui/button.png");
    add(Kind::Texture, "slider_bar", "ui/slider_bar.png");
    add(Kind::Texture, "slider_knob", "ui/slider_knob.png");
    add(Kind::Texture, "vignette", "ui/vignette.png");
    add(Kind::Image, "icon", "ui/icon.png");
    add(Kind::Texture, "lumo", "characters/lumo.png", true);
    add(Kind::Anim, "lumo", "characters/lumo.anim", true);
    add(Kind::Texture, "lumo_portrait", "characters/lumo_portrait.png");
    add(Kind::Anim, "enemy", "enemies/enemy.anim", true);
    add(Kind::Anim, "boss", "bosses/boss.anim", true);
    const char* types[] = {"walker", "flyer", "armored", "exploder", "charger", "shooter"};
    for (int w = 1; w <= kWorlds; ++w) {
        for (const char* t : types) add(Kind::Texture, "w" + toStr(w) + "_" + t, "enemies/w" + toStr(w) + "_" + t + ".png");
        add(Kind::Texture, "boss" + toStr(w), "bosses/boss" + toStr(w) + ".png");
        add(Kind::Texture, "tiles" + toStr(w), "tilesets/w" + toStr(w) + "_tiles.png", true);
        for (int l = 0; l < 4; ++l) add(Kind::Texture, "bg" + toStr(w) + "_" + toStr(l), "backgrounds/w" + toStr(w) + "_layer" + toStr(l) + ".png");
        add(Kind::Texture, "deco" + toStr(w), "objects/deco_w" + toStr(w) + ".png");
        add(Kind::MusicCheck, "world" + toStr(w), "music/world" + toStr(w) + ".ogg");
        for (int s = 1; s <= kStagesPerWorld; ++s)
            add(Kind::TextCheck, "level", "levels/w" + toStr(w) + "_s" + toStr(s) + ".txt", true);
    }
    add(Kind::Texture, "fog", "backgrounds/fog.png");
    const char* objs[] = {"checkpoint", "exit_gate", "cage", "spikeball", "chain", "fist", "projectiles", "particle_soft",
                          "particle_star", "particle_dust", "telegraph", "shockwave", "wind", "laser"};
    for (const char* o : objs) add(Kind::Texture, o, std::string("objects/") + o + ".png");
    const char* cols[] = {"ting", "gem", "key", "relics", "lumling", "heart", "upgrade"};
    for (const char* c : cols) add(Kind::Texture, c, std::string("collectibles/") + c + ".png");
    add(Kind::Texture, "map", "worldmap/map.png");
    add(Kind::Image, "map", "worldmap/map.png");
    add(Kind::TextCheck, "nodes", "worldmap/nodes.txt", true);
    const char* wm[] = {"nodes", "flag", "cloud0", "cloud1", "cloud2", "tree0", "tree1", "tree2", "waterfall"};
    for (const char* m : wm) add(Kind::Texture, std::string("map_") + m, std::string("worldmap/") + m + ".png");
    const char* sfx[] = {"jump", "double_jump", "land", "attack", "enemy_hit", "enemy_die", "ting", "gem", "checkpoint", "death",
                         "level_complete", "menu_move", "menu_select", "menu_back", "hurt", "spring", "break", "boss_hit",
                         "boss_roar", "explosion", "key", "door", "oneup", "cage_break", "glide", "shoot", "splash", "upgrade",
                         "laser", "shockwave", "achievement"};
    for (const char* s : sfx) add(Kind::Sound, s, std::string("sounds/") + s + ".wav");
    const char* music[] = {"menu", "worldmap", "boss", "boss_final", "victory", "gameover"};
    for (const char* m : music) add(Kind::MusicCheck, m, std::string("music/") + m + ".ogg");
    const char* amb[] = {"amb_forest", "amb_band", "amb_city", "amb_snow", "amb_lava"};
    for (const char* a : amb) add(Kind::MusicCheck, a, std::string("sounds/") + a + ".ogg");
}

void AssetManager::markMissing(const Item& it) {
    m_missing.push_back(it.rel);
    logLine("[assets] missing or unreadable: " + it.rel);
    if (it.critical) m_criticalMissing = true;
}

bool AssetManager::loadNext() {
    if (m_next >= m_queue.size()) return false;
    const Item& it = m_queue[m_next++];
    const std::string full = path(it.rel);
    std::error_code ec;
    if (!fs::exists(full, ec)) {
        markMissing(it);
        return true;
    }
    switch (it.kind) {
    case Kind::Texture: {
        auto t = std::make_unique<sf::Texture>();
        if (t->loadFromFile(full)) {
            t->setSmooth(true);
            m_textures[it.key] = std::move(t);
        } else markMissing(it);
        break;
    }
    case Kind::Font: {
        auto f = std::make_unique<sf::Font>();
        if (f->loadFromFile(full)) {
            m_fonts[it.key] = std::move(f);
            m_fontOk = true;
        } else markMissing(it);
        break;
    }
    case Kind::Sound: {
        auto s = std::make_unique<sf::SoundBuffer>();
        if (s->loadFromFile(full)) m_sounds[it.key] = std::move(s);
        else markMissing(it);
        break;
    }
    case Kind::Anim: {
        auto a = std::make_unique<AnimationSet>();
        if (a->loadFromFile(full)) m_anims[it.key] = std::move(a);
        else markMissing(it);
        break;
    }
    case Kind::Image: {
        auto im = std::make_unique<sf::Image>();
        if (im->loadFromFile(full)) m_images[it.key] = std::move(im);
        else markMissing(it);
        break;
    }
    case Kind::MusicCheck:
    case Kind::TextCheck:
        break;  // existence verified above; streamed / parsed on demand
    }
    return true;
}

float AssetManager::progress() const {
    if (m_queue.empty()) return 1.f;
    return static_cast<float>(m_next) / static_cast<float>(m_queue.size());
}

std::string AssetManager::currentLabel() const {
    if (m_next >= m_queue.size()) return "Ready";
    return m_queue[m_next].rel;
}

const sf::Texture& AssetManager::texture(const std::string& key) const {
    const auto it = m_textures.find(key);
    return it == m_textures.end() ? *m_fallbackTex : *it->second;
}

const sf::Font& AssetManager::font(const std::string& key) const {
    auto it = m_fonts.find(key);
    if (it != m_fonts.end()) return *it->second;
    it = m_fonts.find("ui");
    if (it != m_fonts.end()) return *it->second;
    if (!m_fonts.empty()) return *m_fonts.begin()->second;
    return *m_fallbackFont;
}

const sf::SoundBuffer* AssetManager::sound(const std::string& key) const {
    const auto it = m_sounds.find(key);
    return it == m_sounds.end() ? nullptr : it->second.get();
}

const AnimationSet& AssetManager::anim(const std::string& key) const {
    const auto it = m_anims.find(key);
    return it == m_anims.end() ? *m_fallbackAnim : *it->second;
}

const sf::Image* AssetManager::image(const std::string& key) const {
    const auto it = m_images.find(key);
    return it == m_images.end() ? nullptr : it->second.get();
}

std::string AssetManager::musicPath(const std::string& name) const {
    std::error_code ec;
    const fs::path a = m_root / "music" / (name + ".ogg");
    if (fs::exists(a, ec)) return a.string();
    const fs::path b = m_root / "sounds" / (name + ".ogg");
    if (fs::exists(b, ec)) return b.string();
    return std::string();
}

std::string AssetManager::textFile(const std::string& rel) const {
    std::ifstream f(path(rel), std::ios::binary);
    if (!f) return std::string();
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace rayman
