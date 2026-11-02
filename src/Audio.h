#pragma once
// Music with cross-fading, looping ambience, and a fixed voice pool for sound effects.
// All failures (missing files, no audio device) degrade to silence; nothing throws.
#include <SFML/Audio.hpp>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>

namespace rayman {

class AssetManager;

class Audio {
public:
    explicit Audio(const AssetManager& assets);
    void setVolumes(float master, float music, float sfx);
    void playMusic(const std::string& name, float fadeSeconds = 1.0f);
    void stopMusic(float fadeSeconds = 0.8f);
    void playAmbience(const std::string& name);
    void play(const std::string& sfx, float volume = 1.f, float pitchVariance = 0.f, float pitch = 1.f);
    void setDucked(bool ducked) { m_ducked = ducked; }
    void update(float dt);
    const std::string& currentMusic() const { return m_track[m_active]; }
private:
    float musicVolumeScalar() const;
    const AssetManager& m_assets;
    std::array<std::unique_ptr<sf::Music>, 2> m_music;
    std::array<std::string, 2> m_track;
    std::array<float, 2> m_gain{{0.f, 0.f}};
    int m_active = 0;
    float m_fadeSpeed = 1.f;
    std::unique_ptr<sf::Music> m_ambience;
    std::string m_ambName;
    static constexpr int kVoices = 24;
    std::array<sf::Sound, kVoices> m_voices;
    std::unordered_map<std::string, float> m_cooldown;
    float m_master = 0.8f, m_musicVol = 0.7f, m_sfxVol = 0.8f;
    bool m_ducked = false;
    float m_duck = 1.f;
};

} // namespace rayman
