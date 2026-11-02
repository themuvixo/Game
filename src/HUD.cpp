#include "HUD.h"
#include "AssetManager.h"
#include "Boss.h"
#include "Graphics.h"
#include "Level.h"
#include "Player.h"

namespace rayman {

void HUD::update(float dt) {
    m_time += dt;
    m_checkpointFlash = std::max(0.f, m_checkpointFlash - dt);
    m_tingPulse = std::max(0.f, m_tingPulse - dt);
    if (!m_toasts.empty()) {
        m_toasts.front().t += dt;
        if (m_toasts.front().t > 3.6f) m_toasts.pop_front();
    }
}

void HUD::toast(const std::string& title, const std::string& body) {
    Toast t;
    t.title = title;
    t.body = body;
    m_toasts.push_back(t);
}

void HUD::draw(sf::RenderTarget& t, const AssetManager& a, const Level& level, const Player& player, int lives, float fps, bool showFps) const {
    const sf::Font& ui = a.font("ui");
    const sf::Texture& icons = a.texture("icons");
    const RunStats& s = level.stats();
    // top-left: health + lives
    ui::panel(t, a.texture("panel"), sf::FloatRect(16.f, 14.f, 236.f, 66.f), 0.85f, 20.f);
    for (int i = 0; i < Player::kMaxHealth; ++i) {
        const bool full = i < player.health();
        const float pulse = (full && player.health() == 1) ? 1.f + 0.12f * std::sin(m_time * 10.f) : 1.f;
        ui::icon(t, icons, full ? 0 : 1, sf::Vector2f(46.f + static_cast<float>(i) * 44.f, 47.f), 40.f * pulse);
    }
    ui::icon(t, icons, 3, sf::Vector2f(186.f, 46.f), 38.f);
    ui::text(t, ui, "x" + toStr(lives), 24, sf::Vector2f(206.f, 30.f), sf::Color(255, 245, 220));

    // top-center: stage name
    ui::text(t, a.font("display"), toStr(level.info().world) + "-" + toStr(level.info().stage) + "  " + level.info().name, 24,
             sf::Vector2f(kViewW * 0.5f, 18.f), sf::Color(255, 240, 210), ui::Align::Center, 2.f);
    ui::text(t, a.font("body"), formatTime(s.time), 20, sf::Vector2f(kViewW * 0.5f, 50.f), sf::Color(220, 210, 255), ui::Align::Center);

    // top-right: score + collectibles
    ui::panel(t, a.texture("panel"), sf::FloatRect(kViewW - 300.f, 14.f, 284.f, 108.f), 0.85f, 20.f);
    ui::text(t, ui, padNumber(s.score, 7), 28, sf::Vector2f(kViewW - 32.f, 22.f), sf::Color(255, 236, 140), ui::Align::Right);
    const float tp = 1.f + m_tingPulse * 1.2f;
    ui::icon(t, icons, 2, sf::Vector2f(kViewW - 270.f, 86.f), 32.f * tp);
    ui::text(t, ui, toStr(s.tings) + "/" + toStr(s.tingsTotal), 20, sf::Vector2f(kViewW - 250.f, 73.f), sf::Color::White);
    ui::icon(t, icons, 9, sf::Vector2f(kViewW - 168.f, 86.f), 30.f);
    ui::text(t, ui, toStr(s.cages) + "/" + toStr(s.cagesTotal), 20, sf::Vector2f(kViewW - 150.f, 73.f), sf::Color::White);
    ui::icon(t, icons, 8, sf::Vector2f(kViewW - 84.f, 86.f), 28.f);
    ui::text(t, ui, toStr(s.gems), 20, sf::Vector2f(kViewW - 66.f, 73.f), sf::Color::White);
    if (s.keyFound) ui::icon(t, icons, 10, sf::Vector2f(kViewW - 30.f, 140.f), 34.f);

    // checkpoint indicator
    if (level.activeCheckpoint() >= 0) {
        const float k = m_checkpointFlash > 0.f ? 0.6f + 0.4f * std::sin(m_time * 12.f) : 0.7f;
        ui::icon(t, icons, 14, sf::Vector2f(36.f, 106.f), 32.f, k);
        if (m_checkpointFlash > 0.f) ui::text(t, ui, "Checkpoint saved", 18, sf::Vector2f(58.f, 94.f), sf::Color(255, 190, 230), ui::Align::Left, 2.f, sf::Color(30, 16, 50), clampf(m_checkpointFlash, 0.f, 1.f));
    }

    // boss bar
    Level& lv = const_cast<Level&>(level);
    if (Boss* b = lv.boss()) {
        if (b->active() || b->dying()) {
            const sf::FloatRect bar(kViewW * 0.5f - 320.f, kViewH - 60.f, 640.f, 26.f);
            ui::text(t, a.font("display"), b->name(), 24, sf::Vector2f(kViewW * 0.5f, kViewH - 96.f), sf::Color(255, 200, 200), ui::Align::Center, 2.f);
            const sf::Color c = b->vulnerable() ? sf::Color(255, 230, 120) : (b->phase() >= 3 ? sf::Color(255, 70, 90) : sf::Color(230, 80, 160));
            ui::bar(t, bar, b->healthFrac(), c);
            for (int i = 1; i < 3; ++i) {
                sf::RectangleShape tick(sf::Vector2f(2.f, 26.f));
                tick.setPosition(bar.left + bar.width * static_cast<float>(i) / 3.f, bar.top);
                tick.setFillColor(sf::Color(255, 255, 255, 120));
                t.draw(tick);
            }
            if (b->vulnerable())
                ui::text(t, ui, "STRIKE NOW!", 22, sf::Vector2f(kViewW * 0.5f, kViewH - 132.f), sf::Color(255, 240, 150), ui::Align::Center, 2.f,
                         sf::Color(30, 16, 50), 0.6f + 0.4f * std::sin(m_time * 12.f));
        }
    }
    if (player.charging()) {
        ui::text(t, ui, "CHARGED!", 18, sf::Vector2f(kViewW * 0.5f, 78.f), sf::Color(255, 230, 140), ui::Align::Center);
    }
    if (showFps) ui::text(t, a.font("body"), "FPS " + toStr(static_cast<int>(fps + 0.5f)), 18, sf::Vector2f(20.f, kViewH - 34.f), sf::Color(180, 255, 180));
    drawToasts(t, a);
}

void HUD::drawToasts(sf::RenderTarget& t, const AssetManager& a) const {
    if (m_toasts.empty()) return;
    const Toast& to = m_toasts.front();
    const float in = easeOutBack(clampf(to.t / 0.4f, 0.f, 1.f));
    const float out = clampf((3.6f - to.t) / 0.4f, 0.f, 1.f);
    const float y = -100.f + 230.f * in * out + (1.f - out) * -20.f;
    const sf::FloatRect r(kViewW * 0.5f - 230.f, y, 460.f, 84.f);
    ui::panel(t, a.texture("panel"), r, 0.95f, 22.f);
    ui::icon(t, a.texture("icons"), 11, sf::Vector2f(r.left + 44.f, r.top + 42.f), 44.f);
    ui::text(t, a.font("ui"), to.title, 22, sf::Vector2f(r.left + 80.f, r.top + 12.f), sf::Color(255, 230, 140));
    ui::text(t, a.font("body"), to.body, 17, sf::Vector2f(r.left + 80.f, r.top + 44.f), sf::Color(235, 225, 255));
}

} // namespace rayman
