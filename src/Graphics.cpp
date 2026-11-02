#include "Graphics.h"
#include "AssetManager.h"

namespace rayman {

// ---------------------------------------------------------------- Camera
void Camera::reset(sf::Vector2f focus, const sf::FloatRect& worldBounds) {
    m_bounds = worldBounds;
    m_center = clampCenter(focus);
    m_lookAhead = 0.f;
    m_groundY = focus.y;
    m_focus.reset();
}

sf::Vector2f Camera::clampCenter(sf::Vector2f c) const {
    const float hw = kViewW * 0.5f, hh = kViewH * 0.5f;
    if (m_bounds.width <= kViewW) c.x = m_bounds.left + m_bounds.width * 0.5f;
    else c.x = clampf(c.x, m_bounds.left + hw, m_bounds.left + m_bounds.width - hw);
    if (m_bounds.height <= kViewH) c.y = m_bounds.top + m_bounds.height - hh;
    else c.y = clampf(c.y, m_bounds.top + hh, m_bounds.top + m_bounds.height - hh);
    return c;
}

void Camera::update(float dt, sf::Vector2f target, sf::Vector2f velocity, float facing, bool grounded) {
    sf::Vector2f desired;
    if (m_focus) {
        desired = sf::Vector2f(m_focus->left + m_focus->width * 0.5f, m_focus->top + m_focus->height * 0.5f);
        m_center = lerpv(m_center, desired, 1.f - std::exp(-3.f * dt));
    } else {
        const float aheadTarget = facing * 140.f + clampf(velocity.x * 0.25f, -80.f, 80.f);
        m_lookAhead = lerpf(m_lookAhead, aheadTarget, 1.f - std::exp(-2.5f * dt));
        // vertical: follow ground height when grounded, only follow airborne motion beyond a dead zone
        if (grounded) m_groundY = target.y;
        float ty = m_groundY - 60.f;
        if (target.y < m_groundY - 220.f) ty = target.y + 160.f;
        if (target.y > m_groundY + 40.f || velocity.y > 700.f) { ty = target.y - 40.f; m_groundY = target.y; }
        desired = sf::Vector2f(target.x + m_lookAhead, ty);
        m_center.x = lerpf(m_center.x, desired.x, 1.f - std::exp(-6.f * dt));
        m_center.y = lerpf(m_center.y, desired.y, 1.f - std::exp(-4.f * dt));
    }
    m_center = clampCenter(m_center);
    if (m_shakeTime > 0.f) {
        m_shakeTime -= dt;
        const float k = clampf(m_shakeTime / std::max(0.001f, m_shakeDur), 0.f, 1.f) * m_shakeStrength;
        m_shakeOffset = sf::Vector2f(globalRng().uniform(-k, k), globalRng().uniform(-k, k));
    } else {
        m_shakeOffset = sf::Vector2f(0.f, 0.f);
    }
}

void Camera::shake(float strength, float duration) {
    if (strength >= m_shakeStrength * clampf(m_shakeTime / std::max(0.001f, m_shakeDur), 0.f, 1.f)) {
        m_shakeStrength = strength;
        m_shakeDur = duration;
        m_shakeTime = duration;
    }
}

sf::View Camera::view() const {
    sf::View v(sf::FloatRect(0.f, 0.f, static_cast<float>(kViewW), static_cast<float>(kViewH)));
    v.setCenter(std::round(m_center.x + m_shakeOffset.x), std::round(m_center.y + m_shakeOffset.y));
    return v;
}

sf::FloatRect Camera::visibleRect() const {
    return sf::FloatRect(m_center.x - kViewW * 0.5f, m_center.y - kViewH * 0.5f, static_cast<float>(kViewW), static_cast<float>(kViewH));
}

// ---------------------------------------------------------------- Fader
void Fader::update(float dt) {
    if (m_dir == 0) return;
    m_alpha += static_cast<float>(m_dir) * m_speed * dt;
    if (m_alpha >= 1.f) { m_alpha = 1.f; m_dir = 0; }
    if (m_alpha <= 0.f) { m_alpha = 0.f; m_dir = 0; }
}

void Fader::draw(sf::RenderTarget& t) const {
    if (m_alpha <= 0.f) return;
    const sf::View old = t.getView();
    t.setView(t.getDefaultView());
    // iris-style transition: dark overlay plus a soft circle closing in
    sf::RectangleShape r(sf::Vector2f(static_cast<float>(kViewW), static_cast<float>(kViewH)));
    r.setFillColor(sf::Color(12, 6, 24, static_cast<sf::Uint8>(easeInOut(m_alpha) * 255.f)));
    t.draw(r);
    t.setView(old);
}

// ---------------------------------------------------------------- PostFX
void PostFX::init(const AssetManager& assets) {
    m_ok = false;
    if (!sf::Shader::isAvailable()) {
        logLine("[gfx] shaders unavailable; using sprite vignette fallback");
        return;
    }
    if (m_shader.loadFromFile(assets.path("shaders/post.frag"), sf::Shader::Fragment)) {
        m_ok = true;
    } else {
        logLine("[gfx] post.frag failed to compile; using fallback");
    }
}

void PostFX::present(sf::RenderTarget& window, const sf::RenderTexture& scene, const sf::FloatRect& viewport, float time,
                     const sf::Texture* vignetteFallback) {
    sf::Sprite s(scene.getTexture());
    s.setPosition(viewport.left, viewport.top);
    s.setScale(viewport.width / static_cast<float>(kViewW), viewport.height / static_cast<float>(kViewH));
    if (m_ok) {
        m_shader.setUniform("texture", sf::Shader::CurrentTexture);
        m_shader.setUniform("time", time);
        m_shader.setUniform("vignette", 0.25f);
        m_shader.setUniform("tint", sf::Glsl::Vec3(m_tint.x, m_tint.y, m_tint.z));
        m_shader.setUniform("flash", clampf(m_flash, 0.f, 1.f));
        sf::RenderStates st;
        st.shader = &m_shader;
        window.draw(s, st);
    } else {
        window.draw(s);
        if (vignetteFallback) {
            sf::Sprite v(*vignetteFallback);
            v.setPosition(viewport.left, viewport.top);
            v.setScale(viewport.width / static_cast<float>(vignetteFallback->getSize().x),
                       viewport.height / static_cast<float>(vignetteFallback->getSize().y));
            window.draw(v);
        }
        if (m_flash > 0.f) {
            sf::RectangleShape r(sf::Vector2f(viewport.width, viewport.height));
            r.setPosition(viewport.left, viewport.top);
            r.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(clampf(m_flash, 0.f, 1.f) * 255.f)));
            window.draw(r);
        }
    }
}

// ---------------------------------------------------------------- UI helpers
namespace ui {

sf::FloatRect text(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size, sf::Vector2f pos, sf::Color fill,
                   Align align, float outline, sf::Color outlineColor, float alpha) {
    sf::Text tx(s, f, size);
    const sf::FloatRect b = tx.getLocalBounds();
    float ox = 0.f;
    if (align == Align::Center) ox = b.left + b.width * 0.5f;
    else if (align == Align::Right) ox = b.left + b.width;
    tx.setOrigin(std::round(ox), 0.f);
    tx.setPosition(std::round(pos.x), std::round(pos.y));
    sf::Text shadow = tx;
    shadow.setFillColor(withAlpha(sf::Color(10, 4, 20), 0.55f * alpha));
    shadow.move(2.f, 3.f);
    shadow.setOutlineThickness(outline);
    shadow.setOutlineColor(withAlpha(sf::Color(10, 4, 20), 0.35f * alpha));
    t.draw(shadow);
    tx.setFillColor(withAlpha(fill, (fill.a / 255.f) * alpha));
    tx.setOutlineThickness(outline);
    tx.setOutlineColor(withAlpha(outlineColor, alpha));
    t.draw(tx);
    return tx.getGlobalBounds();
}

void panel(sf::RenderTarget& t, const sf::Texture& tex, const sf::FloatRect& r, float alpha, float border) {
    const sf::Vector2u ts = tex.getSize();
    const float tb = static_cast<float>(ts.x) / 4.f;  // source border
    const float xs[4] = {r.left, r.left + border, r.left + r.width - border, r.left + r.width};
    const float ys[4] = {r.top, r.top + border, r.top + r.height - border, r.top + r.height};
    const float us[4] = {0.f, tb, static_cast<float>(ts.x) - tb, static_cast<float>(ts.x)};
    const float vs[4] = {0.f, tb, static_cast<float>(ts.y) - tb, static_cast<float>(ts.y)};
    sf::VertexArray va(sf::Triangles);
    const sf::Color c = withAlpha(sf::Color::White, alpha);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            const sf::Vertex a(sf::Vector2f(xs[x], ys[y]), c, sf::Vector2f(us[x], vs[y]));
            const sf::Vertex b(sf::Vector2f(xs[x + 1], ys[y]), c, sf::Vector2f(us[x + 1], vs[y]));
            const sf::Vertex cc(sf::Vector2f(xs[x + 1], ys[y + 1]), c, sf::Vector2f(us[x + 1], vs[y + 1]));
            const sf::Vertex d(sf::Vector2f(xs[x], ys[y + 1]), c, sf::Vector2f(us[x], vs[y + 1]));
            va.append(a); va.append(b); va.append(cc);
            va.append(a); va.append(cc); va.append(d);
        }
    }
    sf::RenderStates st;
    st.texture = &tex;
    t.draw(va, st);
}

void icon(sf::RenderTarget& t, const sf::Texture& icons, int index, sf::Vector2f pos, float size, float alpha, float rotation) {
    sf::Sprite s(icons, sf::IntRect((index % 8) * 48, (index / 8) * 48, 48, 48));
    s.setOrigin(24.f, 24.f);
    s.setPosition(pos);
    s.setScale(size / 48.f, size / 48.f);
    s.setRotation(rotation);
    s.setColor(withAlpha(sf::Color::White, alpha));
    t.draw(s);
}

void bar(sf::RenderTarget& t, const sf::FloatRect& r, float fill, sf::Color c, sf::Color back) {
    sf::RectangleShape bg(sf::Vector2f(r.width, r.height));
    bg.setPosition(r.left, r.top);
    bg.setFillColor(back);
    bg.setOutlineThickness(2.f);
    bg.setOutlineColor(sf::Color(255, 220, 140));
    t.draw(bg);
    sf::RectangleShape fg(sf::Vector2f(std::max(0.f, (r.width - 4.f) * clampf(fill, 0.f, 1.f)), r.height - 4.f));
    fg.setPosition(r.left + 2.f, r.top + 2.f);
    fg.setFillColor(c);
    t.draw(fg);
    sf::RectangleShape hl(sf::Vector2f(fg.getSize().x, std::max(1.f, (r.height - 4.f) * 0.35f)));
    hl.setPosition(r.left + 2.f, r.top + 2.f);
    hl.setFillColor(sf::Color(255, 255, 255, 60));
    t.draw(hl);
}

void button(sf::RenderTarget& t, const AssetManager& a, const std::string& label, sf::Vector2f c, bool selected, bool enabled,
            float pulse, float width) {
    const sf::Texture& tex = a.texture("button");
    const float scale = selected ? 1.f + 0.04f * std::sin(pulse * 6.f) + 0.04f : 1.f;
    sf::Sprite s(tex, sf::IntRect(0, selected ? 64 : 0, 320, 64));
    s.setOrigin(160.f, 32.f);
    s.setPosition(c);
    s.setScale(width / 320.f * scale, scale);
    s.setColor(enabled ? sf::Color::White : sf::Color(140, 130, 150, 200));
    t.draw(s);
    const sf::Color fill = enabled ? (selected ? sf::Color(255, 250, 220) : sf::Color(235, 225, 255)) : sf::Color(150, 140, 160);
    text(t, a.font("ui"), label, 26, sf::Vector2f(c.x, c.y - 19.f), fill, Align::Center, 2.f);
    if (selected) {
        const float bob = std::sin(pulse * 8.f) * 6.f;
        icon(t, a.texture("icons"), 15, sf::Vector2f(c.x - width * 0.5f - 26.f + bob, c.y), 34.f);
    }
}

bool hit(const sf::FloatRect& r, sf::Vector2f p) { return r.contains(p); }

} // namespace ui

void drawParallax(sf::RenderTarget& t, const sf::Texture& tex, float cameraX, float cameraY, float factorX, float factorY,
                  float refCameraY, float autoScroll, sf::Color color) {
    const float w = static_cast<float>(tex.getSize().x);
    if (w <= 0.f) return;
    const float viewLeft = cameraX - kViewW * 0.5f;
    const float viewTop = cameraY - kViewH * 0.5f;
    float offset = std::fmod(viewLeft * factorX + autoScroll, w);
    if (offset < 0.f) offset += w;
    const float screenY = (refCameraY - cameraY) * factorY;  // layer drifts down as the camera climbs
    sf::Sprite s(tex);
    s.setColor(color);
    for (float x = -offset; x < static_cast<float>(kViewW); x += w) {
        s.setPosition(std::round(viewLeft + x), std::round(viewTop + screenY));
        t.draw(s);
    }
}

} // namespace rayman
