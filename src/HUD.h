#pragma once
// In-game heads-up display: health, lives, score, collectibles, stage, checkpoint state, boss bar, toasts.
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>

namespace rayman {

class AssetManager;
class Level;
class Player;

class HUD {
public:
    void update(float dt);
    void draw(sf::RenderTarget& t, const AssetManager& a, const Level& level, const Player& player, int lives, float fps, bool showFps) const;
    void toast(const std::string& title, const std::string& body);
    void flashCheckpoint() { m_checkpointFlash = 2.5f; }
    void bumpTings() { m_tingPulse = 0.25f; }
    void drawToasts(sf::RenderTarget& t, const AssetManager& a) const;
private:
    struct Toast { std::string title, body; float t = 0.f; };
    std::deque<Toast> m_toasts;
    float m_checkpointFlash = 0.f;
    float m_tingPulse = 0.f;
    float m_time = 0.f;
};

} // namespace rayman
