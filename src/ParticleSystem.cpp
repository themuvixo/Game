#include "ParticleSystem.h"

namespace rayman {

static constexpr size_t kMaxParticles = 6000;

ParticleSystem::ParticleSystem() {
    m_pool.resize(kMaxParticles);
    for (int t = 0; t < 3; ++t)
        for (int b = 0; b < 2; ++b) m_va[t][b].setPrimitiveType(sf::Triangles);
}

void ParticleSystem::setTextures(const sf::Texture* soft, const sf::Texture* star, const sf::Texture* dust) {
    m_tex[0] = soft; m_tex[1] = star; m_tex[2] = dust;
}

Particle* ParticleSystem::spawn() {
    if (m_count >= m_pool.size()) return nullptr;
    Particle& p = m_pool[m_count++];
    p = Particle();
    return &p;
}

void ParticleSystem::update(float dt) {
    size_t i = 0;
    while (i < m_count) {
        Particle& p = m_pool[i];
        p.life += dt;
        if (p.life >= p.maxLife) {
            m_pool[i] = m_pool[m_count - 1];
            --m_count;
            continue;
        }
        p.vel.y += p.gravity * dt;
        if (p.drag > 0.f) p.vel *= std::max(0.f, 1.f - p.drag * dt);
        p.pos += p.vel * dt;
        p.rot += p.rotVel * dt;
        ++i;
    }
}

void ParticleSystem::draw(sf::RenderTarget& target) const {
    for (int t = 0; t < 3; ++t)
        for (int b = 0; b < 2; ++b) m_va[t][b].clear();
    for (size_t i = 0; i < m_count; ++i) {
        const Particle& p = m_pool[i];
        const float t = p.life / p.maxLife;
        const float s = lerpf(p.size0, p.size1, t) * 0.5f;
        const sf::Color c = mixColor(p.c0, p.c1, t);
        const int ti = (p.tex >= 0 && p.tex < 3) ? p.tex : 0;
        const sf::Texture* tex = m_tex[ti];
        const float tw = tex ? static_cast<float>(tex->getSize().x) : 1.f;
        const float th = tex ? static_cast<float>(tex->getSize().y) : 1.f;
        const float ca = std::cos(p.rot), sa = std::sin(p.rot);
        const sf::Vector2f ax(ca * s, sa * s), ay(-sa * s, ca * s);
        const sf::Vector2f q[4] = {p.pos - ax - ay, p.pos + ax - ay, p.pos + ax + ay, p.pos - ax + ay};
        const sf::Vector2f uv[4] = {{0.f, 0.f}, {tw, 0.f}, {tw, th}, {0.f, th}};
        sf::VertexArray& va = m_va[ti][p.additive ? 1 : 0];
        const int idx[6] = {0, 1, 2, 0, 2, 3};
        for (int k : idx) va.append(sf::Vertex(q[k], c, uv[k]));
    }
    for (int t = 0; t < 3; ++t) {
        for (int b = 0; b < 2; ++b) {
            if (m_va[t][b].getVertexCount() == 0) continue;
            sf::RenderStates st;
            st.texture = m_tex[t];
            st.blendMode = b ? sf::BlendAdd : sf::BlendAlpha;
            target.draw(m_va[t][b], st);
        }
    }
}

void ParticleSystem::burst(sf::Vector2f p, int n, sf::Color a, sf::Color b, float speed, float life, float size, int tex, float gravity) {
    Rng& r = globalRng();
    for (int i = 0; i < n; ++i) {
        Particle* q = spawn();
        if (!q) return;
        const float ang = r.uniform(0.f, 2.f * kPi);
        const float sp = r.uniform(0.3f, 1.f) * speed;
        q->pos = p;
        q->vel = sf::Vector2f(std::cos(ang) * sp, std::sin(ang) * sp);
        q->maxLife = life * r.uniform(0.6f, 1.2f);
        q->size0 = size * r.uniform(0.6f, 1.3f);
        q->size1 = 0.f;
        q->c0 = a; q->c1 = b;
        q->tex = tex;
        q->gravity = gravity;
        q->drag = 2.f;
        q->rot = r.uniform(0.f, 6.f);
        q->rotVel = r.uniform(-4.f, 4.f);
    }
}

void ParticleSystem::dust(sf::Vector2f feet, int n, sf::Color c) {
    Rng& r = globalRng();
    for (int i = 0; i < n; ++i) {
        Particle* q = spawn();
        if (!q) return;
        const float dir = (i % 2 == 0) ? -1.f : 1.f;
        q->pos = feet + sf::Vector2f(r.uniform(-8.f, 8.f), r.uniform(-4.f, 0.f));
        q->vel = sf::Vector2f(dir * r.uniform(40.f, 160.f), r.uniform(-60.f, -10.f));
        q->maxLife = r.uniform(0.3f, 0.55f);
        q->size0 = r.uniform(14.f, 24.f);
        q->size1 = q->size0 * 1.6f;
        q->c0 = c; q->c1 = withAlpha(c, 0.f);
        q->tex = 2;
        q->additive = false;
        q->drag = 4.f;
    }
}

void ParticleSystem::sparkle(sf::Vector2f p, sf::Color c, int n) {
    burst(p, n, c, withAlpha(sf::Color::White, 0.f), 180.f, 0.5f, 14.f, 1, 0.f);
    burst(p, n / 2 + 1, withAlpha(c, 0.7f), withAlpha(c, 0.f), 60.f, 0.4f, 34.f, 0, 0.f);
}

void ParticleSystem::trail(sf::Vector2f p, sf::Color c) {
    Particle* q = spawn();
    if (!q) return;
    Rng& r = globalRng();
    q->pos = p + sf::Vector2f(r.uniform(-4.f, 4.f), r.uniform(-4.f, 4.f));
    q->vel = sf::Vector2f(r.uniform(-20.f, 20.f), r.uniform(-30.f, 10.f));
    q->maxLife = 0.35f;
    q->size0 = 16.f; q->size1 = 2.f;
    q->c0 = c; q->c1 = withAlpha(c, 0.f);
    q->tex = r.chance(0.3f) ? 1 : 0;
}

void ParticleSystem::explosion(sf::Vector2f p, sf::Color c, float scale) {
    burst(p, static_cast<int>(26 * scale), sf::Color(255, 240, 200), withAlpha(c, 0.f), 420.f * scale, 0.6f, 26.f * scale, 0, 300.f);
    burst(p, static_cast<int>(10 * scale), withAlpha(sf::Color(80, 60, 70), 0.8f), withAlpha(sf::Color(40, 30, 40), 0.f), 160.f * scale, 0.9f, 40.f * scale, 2, -40.f);
    for (int i = 0; i < 10 * scale; ++i) {
        Particle* q = spawn();
        if (!q) return;
        q->pos = p; q->additive = false; q->tex = 2;
        q->vel = sf::Vector2f(globalRng().uniform(-60.f, 60.f), globalRng().uniform(-60.f, 60.f));
        q->maxLife = 0.7f; q->size0 = 30.f * scale; q->size1 = 70.f * scale;
        q->c0 = withAlpha(sf::Color(255, 255, 255), 0.5f); q->c1 = withAlpha(sf::Color(255, 255, 255), 0.f);
    }
}

void ParticleSystem::ambient(AmbientKind kind, const sf::FloatRect& view, float dt) {
    if (kind == AmbientKind::None) return;
    Rng& r = globalRng();
    const float rate = kind == AmbientKind::Snow ? 55.f : (kind == AmbientKind::Embers ? 30.f : 14.f);
    m_ambientAcc += dt * rate;
    while (m_ambientAcc >= 1.f) {
        m_ambientAcc -= 1.f;
        Particle* q = spawn();
        if (!q) return;
        q->pos = sf::Vector2f(view.left + r.uniform(-100.f, view.width + 100.f), view.top + r.uniform(-40.f, view.height));
        q->maxLife = r.uniform(3.f, 6.f);
        q->rot = r.uniform(0.f, 6.f);
        switch (kind) {
        case AmbientKind::Petals:
            q->pos.y = view.top - 20.f;
            q->vel = sf::Vector2f(r.uniform(-40.f, 20.f), r.uniform(40.f, 80.f));
            q->c0 = sf::Color(255, 160, 220, 220); q->c1 = sf::Color(255, 200, 240, 0);
            q->size0 = 10.f; q->size1 = 8.f; q->rotVel = r.uniform(-2.f, 2.f); q->tex = 2; q->additive = false;
            break;
        case AmbientKind::Notes:
            q->vel = sf::Vector2f(r.uniform(-15.f, 15.f), r.uniform(-50.f, -20.f));
            q->c0 = sf::Color(255, 230, 140, 200); q->c1 = sf::Color(255, 140, 60, 0);
            q->size0 = 12.f; q->size1 = 20.f; q->tex = 1;
            break;
        case AmbientKind::Neon:
            q->vel = sf::Vector2f(r.uniform(-10.f, 10.f), r.uniform(-30.f, -5.f));
            q->c0 = r.chance(0.5f) ? sf::Color(0, 240, 255, 200) : sf::Color(255, 60, 220, 200); q->c1 = withAlpha(q->c0, 0.f);
            q->size0 = 6.f; q->size1 = 2.f; q->tex = 0;
            break;
        case AmbientKind::Snow:
            q->pos.y = view.top - 20.f;
            q->vel = sf::Vector2f(r.uniform(-140.f, -40.f), r.uniform(60.f, 140.f));
            q->c0 = sf::Color(255, 255, 255, 230); q->c1 = sf::Color(255, 255, 255, 0);
            q->size0 = r.uniform(4.f, 9.f); q->size1 = q->size0; q->tex = 0; q->additive = false;
            break;
        case AmbientKind::Embers:
            q->pos.y = view.top + view.height + 10.f;
            q->vel = sf::Vector2f(r.uniform(-20.f, 20.f), r.uniform(-120.f, -50.f));
            q->c0 = sf::Color(255, 180, 60, 255); q->c1 = sf::Color(255, 60, 20, 0);
            q->size0 = r.uniform(5.f, 10.f); q->size1 = 1.f; q->tex = 0;
            break;
        case AmbientKind::MenuDust:
            q->vel = sf::Vector2f(r.uniform(-12.f, 12.f), r.uniform(-25.f, -8.f));
            q->c0 = sf::Color(255, 240, 200, 0); q->c1 = sf::Color(255, 200, 255, 0);
            q->size0 = r.uniform(6.f, 14.f); q->size1 = 2.f; q->tex = r.chance(0.3f) ? 1 : 0;
            q->c0.a = 200;
            break;
        default: break;
        }
    }
}

} // namespace rayman
