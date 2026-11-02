#include "Animation.h"
#include "Utils.h"
#include <fstream>
#include <sstream>

namespace rayman {

bool AnimationSet::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string name;
        ss >> name;
        if (name == "frame") {
            ss >> m_frame.x >> m_frame.y;
            continue;
        }
        AnimClip c;
        int loop = 1;
        ss >> c.row >> c.frames >> c.frameDuration >> loop;
        if (!ss.fail()) {
            c.loop = loop != 0;
            if (c.frames < 1) c.frames = 1;
            if (c.frameDuration <= 0.f) c.frameDuration = 0.1f;
            m_clips[name] = c;
        }
    }
    return !m_clips.empty();
}

const AnimClip* AnimationSet::find(const std::string& name) const {
    const auto it = m_clips.find(name);
    return it == m_clips.end() ? nullptr : &it->second;
}

sf::IntRect Animator::rectFor(const AnimClip& c, int frame) const {
    const sf::Vector2i fs = m_set ? m_set->frameSize() : sf::Vector2i(64, 64);
    return sf::IntRect(frame * fs.x, c.row * fs.y, fs.x, fs.y);
}

void Animator::play(const std::string& name, bool restart) {
    if (!m_set) return;
    if (name == m_name && !restart) return;
    const AnimClip* c = m_set->find(name);
    if (!c) return;
    if (m_clip) {
        m_prevRect = rectFor(*m_clip, m_frame);
        m_blend = kBlendTime;
    }
    m_clip = c;
    m_name = name;
    m_time = 0.f;
    m_frame = 0;
    m_finished = false;
}

void Animator::update(float dt) {
    if (!m_clip) return;
    if (m_blend > 0.f) m_blend = std::max(0.f, m_blend - dt);
    m_time += dt * m_speed;
    const int f = static_cast<int>(m_time / m_clip->frameDuration);
    if (m_clip->loop) {
        m_frame = f % m_clip->frames;
    } else {
        m_frame = std::min(f, m_clip->frames - 1);
        m_finished = f >= m_clip->frames;
    }
}

void Animator::draw(sf::RenderTarget& target, sf::Vector2f pos, bool flipX, float scale, sf::Color color,
                    float rotationDeg, sf::Vector2f originFrac) const {
    if (!m_tex || !m_clip) return;
    const sf::IntRect r = rectFor(*m_clip, m_frame);
    sf::Sprite s(*m_tex, r);
    s.setOrigin(r.width * originFrac.x, r.height * originFrac.y);
    s.setPosition(pos);
    s.setScale(flipX ? -scale : scale, scale);
    s.setRotation(rotationDeg);
    if (m_blend > 0.f) {
        const float t = m_blend / kBlendTime;
        sf::Sprite prev(*m_tex, m_prevRect);
        prev.setOrigin(s.getOrigin());
        prev.setPosition(pos);
        prev.setScale(s.getScale());
        prev.setRotation(rotationDeg);
        sf::Color pc = color;
        pc.a = static_cast<sf::Uint8>(color.a * t * 0.8f);
        prev.setColor(pc);
        target.draw(prev);
    }
    s.setColor(color);
    target.draw(s);
}

} // namespace rayman
