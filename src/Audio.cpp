#include "Audio.h"
#include "AssetManager.h"
#include "Utils.h"

namespace rayman {

Audio::Audio(const AssetManager& assets) : m_assets(assets) {
    m_music[0] = std::make_unique<sf::Music>();
    m_music[1] = std::make_unique<sf::Music>();
    m_ambience = std::make_unique<sf::Music>();
}

void Audio::setVolumes(float master, float music, float sfx) {
    m_master = clampf(master, 0.f, 1.f);
    m_musicVol = clampf(music, 0.f, 1.f);
    m_sfxVol = clampf(sfx, 0.f, 1.f);
}

float Audio::musicVolumeScalar() const { return m_master * m_musicVol * m_duck * 100.f; }

void Audio::playMusic(const std::string& name, float fadeSeconds) {
    if (name == m_track[m_active] && m_music[m_active]->getStatus() == sf::SoundSource::Playing) return;
    const std::string file = m_assets.musicPath(name);
    const int next = 1 - m_active;
    m_music[next]->stop();
    m_track[next].clear();
    if (!file.empty() && m_music[next]->openFromFile(file)) {
        m_music[next]->setLoop(true);
        m_music[next]->setVolume(0.f);
        m_music[next]->play();
        m_track[next] = name;
    } else {
        logLine("[audio] music unavailable: " + name);
    }
    m_gain[next] = 0.f;
    m_active = next;
    m_fadeSpeed = fadeSeconds > 0.01f ? 1.f / fadeSeconds : 100.f;
}

void Audio::stopMusic(float fadeSeconds) {
    const int next = 1 - m_active;
    m_music[next]->stop();
    m_track[next].clear();
    m_gain[next] = 0.f;
    m_active = next;
    m_fadeSpeed = fadeSeconds > 0.01f ? 1.f / fadeSeconds : 100.f;
}

void Audio::playAmbience(const std::string& name) {
    if (name == m_ambName) return;
    m_ambience->stop();
    m_ambName = name;
    if (name.empty()) return;
    const std::string file = m_assets.musicPath(name);
    if (!file.empty() && m_ambience->openFromFile(file)) {
        m_ambience->setLoop(true);
        m_ambience->setVolume(m_master * m_sfxVol * 35.f);
        m_ambience->play();
    }
}

void Audio::play(const std::string& sfx, float volume, float pitchVariance, float pitch) {
    const sf::SoundBuffer* buf = m_assets.sound(sfx);
    if (!buf) return;
    auto cd = m_cooldown.find(sfx);
    if (cd != m_cooldown.end() && cd->second > 0.f) return;  // avoid stacking identical sounds
    m_cooldown[sfx] = 0.045f;
    sf::Sound* slot = nullptr;
    for (auto& v : m_voices) {
        if (v.getStatus() != sf::SoundSource::Playing) { slot = &v; break; }
    }
    if (!slot) return;  // all voices busy: drop rather than cut off important sounds
    slot->setBuffer(*buf);
    slot->setVolume(clampf(volume, 0.f, 2.f) * m_master * m_sfxVol * 100.f * 0.5f);
    float p = pitch;
    if (pitchVariance > 0.f) p += globalRng().uniform(-pitchVariance, pitchVariance);
    slot->setPitch(clampf(p, 0.3f, 3.f));
    slot->play();
}

void Audio::update(float dt) {
    for (auto& kv : m_cooldown) kv.second = std::max(0.f, kv.second - dt);
    m_duck = approach(m_duck, m_ducked ? 0.45f : 1.f, dt * 2.f);
    for (int i = 0; i < 2; ++i) {
        const float target = (i == m_active && !m_track[i].empty()) ? 1.f : 0.f;
        m_gain[i] = approach(m_gain[i], target, dt * m_fadeSpeed);
        if (m_music[i]->getStatus() == sf::SoundSource::Playing) {
            m_music[i]->setVolume(m_gain[i] * musicVolumeScalar());
            if (i != m_active && m_gain[i] <= 0.f) m_music[i]->stop();
        }
    }
    if (m_ambience->getStatus() == sf::SoundSource::Playing) m_ambience->setVolume(m_master * m_sfxVol * 35.f * m_duck);
}

} // namespace rayman
