#pragma once
// Central resource cache. Assets are queued from a manifest and loaded incrementally so the
// loading screen can show real progress. Missing files are recorded and replaced by fallbacks.
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Animation.h"

namespace rayman {

class AssetManager {
public:
    AssetManager();
    bool locateAssetRoot(const std::filesystem::path& exeDir);
    const std::filesystem::path& root() const { return m_root; }
    std::string path(const std::string& rel) const { return (m_root / rel).string(); }

    void queueManifest();
    // Loads one queued item; returns false when the queue is empty.
    bool loadNext();
    float progress() const;
    std::string currentLabel() const;
    bool done() const { return m_next >= m_queue.size(); }

    const sf::Texture& texture(const std::string& key) const;
    bool hasTexture(const std::string& key) const { return m_textures.count(key) != 0; }
    const sf::Font& font(const std::string& key) const;
    bool fontOk() const { return m_fontOk; }
    const sf::SoundBuffer* sound(const std::string& key) const;
    const AnimationSet& anim(const std::string& key) const;
    const sf::Image* image(const std::string& key) const;
    std::string musicPath(const std::string& name) const;
    std::string textFile(const std::string& rel) const;

    const std::vector<std::string>& missing() const { return m_missing; }
    bool criticalMissing() const { return m_criticalMissing; }

private:
    enum class Kind { Texture, Font, Sound, Anim, Image, MusicCheck, TextCheck };
    struct Item { Kind kind; std::string key; std::string rel; bool critical; };
    void add(Kind k, const std::string& key, const std::string& rel, bool critical = false) { m_queue.push_back(Item{k, key, rel, critical}); }
    void markMissing(const Item& it);

    std::filesystem::path m_root;
    std::vector<Item> m_queue;
    size_t m_next = 0;
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_textures;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> m_fonts;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> m_sounds;
    std::unordered_map<std::string, std::unique_ptr<AnimationSet>> m_anims;
    std::unordered_map<std::string, std::unique_ptr<sf::Image>> m_images;
    std::unique_ptr<sf::Texture> m_fallbackTex;
    std::unique_ptr<sf::Font> m_fallbackFont;
    std::unique_ptr<AnimationSet> m_fallbackAnim;
    std::vector<std::string> m_missing;
    bool m_criticalMissing = false;
    bool m_fontOk = false;
};

} // namespace rayman
