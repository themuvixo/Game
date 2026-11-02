#pragma once
// Sprite-sheet animation clips (data-driven .anim files), state-based animator with crossfade blending.
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>

namespace rayman {

struct AnimClip {
    int row = 0;
    int frames = 1;
    float frameDuration = 0.1f;
    bool loop = true;
};

class AnimationSet {
public:
    bool loadFromFile(const std::string& path);
    const AnimClip* find(const std::string& name) const;
    sf::Vector2i frameSize() const { return m_frame; }
    bool valid() const { return !m_clips.empty(); }
    void addClip(const std::string& name, const AnimClip& c) { m_clips[name] = c; }
    void setFrameSize(sf::Vector2i s) { m_frame = s; }
private:
    std::unordered_map<std::string, AnimClip> m_clips;
    sf::Vector2i m_frame{64, 64};
};

class Animator {
public:
    void setup(const sf::Texture* tex, const AnimationSet* set) { m_tex = tex; m_set = set; }
    // Starts a clip. Changing clip records the previous frame for a short blend.
    void play(const std::string& name, bool restart = false);
    void update(float dt);
    bool finished() const { return m_finished; }
    const std::string& current() const { return m_name; }
    int frame() const { return m_frame; }
    float clipTime() const { return m_time; }
    void setSpeed(float s) { m_speed = s; }
    // Draws centered horizontally with feet at `pos` (origin bottom-center of the frame).
    void draw(sf::RenderTarget& target, sf::Vector2f pos, bool flipX, float scale = 1.f,
              sf::Color color = sf::Color::White, float rotationDeg = 0.f, sf::Vector2f originFrac = sf::Vector2f(0.5f, 1.f)) const;
    sf::IntRect rectFor(const AnimClip& c, int frame) const;
private:
    const sf::Texture* m_tex = nullptr;
    const AnimationSet* m_set = nullptr;
    const AnimClip* m_clip = nullptr;
    std::string m_name;
    float m_time = 0.f;
    int m_frame = 0;
    bool m_finished = false;
    float m_speed = 1.f;
    // blending
    sf::IntRect m_prevRect;
    float m_blend = 0.f;
    static constexpr float kBlendTime = 0.07f;
};

} // namespace rayman
