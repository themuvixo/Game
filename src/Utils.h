#pragma once
// Shared constants and small helpers. Header-only, C++17.
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <iomanip>
#include <fstream>

namespace rayman {

constexpr int kTile = 48;
constexpr float kTileF = 48.f;
constexpr unsigned kViewW = 1280;
constexpr unsigned kViewH = 720;
constexpr float kFixedDt = 1.f / 120.f;
constexpr int kWorlds = 5;
constexpr int kStagesPerWorld = 7;   // 1-5 normal (5 = challenge), 6 = secret, 7 = boss
constexpr int kSecretStage = 6;
constexpr int kBossStage = 7;
constexpr float kPi = 3.14159265358979f;

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float signf(float v) { return v > 0.f ? 1.f : (v < 0.f ? -1.f : 0.f); }
inline float approach(float v, float target, float delta) {
    if (v < target) return std::min(v + delta, target);
    return std::max(v - delta, target);
}
inline float length(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }
inline sf::Vector2f normalize(sf::Vector2f v) {
    const float l = length(v);
    return l > 0.0001f ? sf::Vector2f(v.x / l, v.y / l) : sf::Vector2f(0.f, 0.f);
}
inline sf::Vector2f lerpv(sf::Vector2f a, sf::Vector2f b, float t) { return a + (b - a) * t; }
inline float easeOutCubic(float t) { t = clampf(t, 0.f, 1.f); const float u = 1.f - t; return 1.f - u * u * u; }
inline float easeInOut(float t) { t = clampf(t, 0.f, 1.f); return t * t * (3.f - 2.f * t); }
inline float easeOutBack(float t) {
    t = clampf(t, 0.f, 1.f);
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    return 1.f + c3 * std::pow(t - 1.f, 3.f) + c1 * std::pow(t - 1.f, 2.f);
}
inline sf::Color withAlpha(sf::Color c, float a) { c.a = static_cast<sf::Uint8>(clampf(a, 0.f, 1.f) * 255.f); return c; }
inline sf::Color mixColor(sf::Color a, sf::Color b, float t) {
    t = clampf(t, 0.f, 1.f);
    return sf::Color(static_cast<sf::Uint8>(a.r + (b.r - a.r) * t), static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
                     static_cast<sf::Uint8>(a.b + (b.b - a.b) * t), static_cast<sf::Uint8>(a.a + (b.a - a.a) * t));
}
inline sf::Vector2f center(const sf::FloatRect& r) { return sf::Vector2f(r.left + r.width * 0.5f, r.top + r.height * 0.5f); }

class Rng {
public:
    explicit Rng(std::uint32_t seed = 1337u) : m_gen(seed) {}
    float uniform(float lo, float hi) { std::uniform_real_distribution<float> d(lo, hi); return d(m_gen); }
    int range(int lo, int hiInclusive) { std::uniform_int_distribution<int> d(lo, hiInclusive); return d(m_gen); }
    bool chance(float p) { return uniform(0.f, 1.f) < p; }
    void seed(std::uint32_t s) { m_gen.seed(s); }
private:
    std::mt19937 m_gen;
};

inline Rng& globalRng() { static Rng rng(20261102u); return rng; }

template <typename T>
inline std::string toStr(const T& v) { std::ostringstream os; os << v; return os.str(); }

inline std::string padNumber(long long v, int width) {
    std::ostringstream os; os << std::setw(width) << std::setfill('0') << v; return os.str();
}

inline std::string formatTime(float seconds) {
    if (seconds < 0.f) seconds = 0.f;
    const int total = static_cast<int>(seconds);
    const int cs = static_cast<int>((seconds - static_cast<float>(total)) * 100.f);
    std::ostringstream os;
    os << total / 60 << ':' << std::setw(2) << std::setfill('0') << total % 60 << '.' << std::setw(2) << std::setfill('0') << cs;
    return os.str();
}

inline std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return std::string();
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Per-user writable directory (Windows: %APPDATA%\LumoShatteredDream). Never inside the install directory.
inline std::filesystem::path userDataDir() {
    namespace fs = std::filesystem;
    fs::path base;
#ifdef _WIN32
    char* buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, "APPDATA") == 0 && buf != nullptr) {
        base = fs::path(buf);
        std::free(buf);
    }
#else
    if (const char* home = std::getenv("HOME")) base = fs::path(home) / ".local" / "share";
#endif
    if (base.empty()) base = fs::temp_directory_path();
    fs::path dir = base / "LumoShatteredDream";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

inline void logLine(const std::string& msg) {
    const auto path = userDataDir() / "game.log";
    std::ofstream f(path, std::ios::app);
    if (f) f << msg << '\n';
}

} // namespace rayman
