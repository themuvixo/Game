#pragma once
// Action-based keyboard + mouse input with rebindable primary keys and fixed alternates.
#include <SFML/Graphics.hpp>
#include <array>
#include <string>
#include "Utils.h"

namespace rayman {

enum class Action { Left = 0, Right, Up, Down, Jump, Attack, Walk, Pause, Confirm, Back, Count };
constexpr int kActionCount = static_cast<int>(Action::Count);
constexpr int kRebindableCount = 7;  // Left..Walk can be rebound

inline const char* actionName(Action a) {
    switch (a) {
    case Action::Left: return "Move Left";
    case Action::Right: return "Move Right";
    case Action::Up: return "Look Up / Map Up";
    case Action::Down: return "Crouch / Map Down";
    case Action::Jump: return "Jump / Glide";
    case Action::Attack: return "Magic Fist";
    case Action::Walk: return "Walk (hold)";
    case Action::Pause: return "Pause";
    case Action::Confirm: return "Confirm";
    case Action::Back: return "Back";
    default: return "?";
    }
}

inline std::array<int, kActionCount> defaultBindings() {
    std::array<int, kActionCount> b{};
    b[static_cast<int>(Action::Left)] = sf::Keyboard::Left;
    b[static_cast<int>(Action::Right)] = sf::Keyboard::Right;
    b[static_cast<int>(Action::Up)] = sf::Keyboard::Up;
    b[static_cast<int>(Action::Down)] = sf::Keyboard::Down;
    b[static_cast<int>(Action::Jump)] = sf::Keyboard::Space;
    b[static_cast<int>(Action::Attack)] = sf::Keyboard::X;
    b[static_cast<int>(Action::Walk)] = sf::Keyboard::LShift;
    b[static_cast<int>(Action::Pause)] = sf::Keyboard::Escape;
    b[static_cast<int>(Action::Confirm)] = sf::Keyboard::Enter;
    b[static_cast<int>(Action::Back)] = sf::Keyboard::Escape;
    return b;
}

inline std::string keyName(int key) {
    using K = sf::Keyboard;
    if (key >= K::A && key <= K::Z) return std::string(1, static_cast<char>('A' + (key - K::A)));
    if (key >= K::Num0 && key <= K::Num9) return std::string(1, static_cast<char>('0' + (key - K::Num0)));
    if (key >= K::Numpad0 && key <= K::Numpad9) return "Num" + std::string(1, static_cast<char>('0' + (key - K::Numpad0)));
    if (key >= K::F1 && key <= K::F12) return "F" + toStr(key - K::F1 + 1);
    switch (key) {
    case K::Space: return "Space";
    case K::Enter: return "Enter";
    case K::Escape: return "Esc";
    case K::LShift: return "L-Shift";
    case K::RShift: return "R-Shift";
    case K::LControl: return "L-Ctrl";
    case K::RControl: return "R-Ctrl";
    case K::LAlt: return "L-Alt";
    case K::RAlt: return "R-Alt";
    case K::Tab: return "Tab";
    case K::Backspace: return "Backspace";
    case K::Left: return "Left";
    case K::Right: return "Right";
    case K::Up: return "Up";
    case K::Down: return "Down";
    case K::Comma: return ",";
    case K::Period: return ".";
    case K::Slash: return "/";
    case K::Semicolon: return ";";
    case K::Quote: return "'";
    case K::LBracket: return "[";
    case K::RBracket: return "]";
    case K::Hyphen: return "-";
    case K::Equal: return "=";
    default: return "Key " + toStr(key);
    }
}

class Input {
public:
    Input() : m_bindings(defaultBindings()) { m_cur.fill(false); m_prev.fill(false); m_edge.fill(false); }

    void setBindings(const std::array<int, kActionCount>& b) { m_bindings = b; }
    const std::array<int, kActionCount>& bindings() const { return m_bindings; }

    // Called once per rendered frame, after events were processed.
    void poll(bool focused) {
        m_prev = m_cur;
        for (int i = 0; i < kActionCount; ++i) m_cur[i] = focused && rawDown(static_cast<Action>(i));
        for (int i = 0; i < kActionCount; ++i) m_edge[i] = m_edge[i] || (m_cur[i] && !m_prev[i]);  // kept until a fixed step consumes it
    }
    // Edge-triggered presses are only visible to the first fixed step of a frame.
    void consumeEdges() { m_edge.fill(false); m_mouseClicked = false; m_wheel = 0.f; }

    bool held(Action a) const { return m_cur[static_cast<int>(a)]; }
    bool pressed(Action a) const { return m_edge[static_cast<int>(a)]; }
    bool released(Action a) const { return !m_cur[static_cast<int>(a)] && m_prev[static_cast<int>(a)]; }

    // Mouse (logical 1280x720 coordinates)
    void setMouse(sf::Vector2f p, bool moved) { m_mouse = p; if (moved) m_mouseMoved = true; }
    void click() { m_mouseClicked = true; }
    void wheel(float d) { m_wheel += d; }
    sf::Vector2f mouse() const { return m_mouse; }
    bool mouseClicked() const { return m_mouseClicked; }
    bool mouseMovedSinceFrame() const { return m_mouseMoved; }
    void clearMouseMoved() { m_mouseMoved = false; }
    float wheelDelta() const { return m_wheel; }

    // Key capture for rebinding
    void setLastKey(int k) { m_lastKey = k; }
    int takeLastKey() { const int k = m_lastKey; m_lastKey = -1; return k; }

private:
    bool keyDown(int k) const { return k >= 0 && k < sf::Keyboard::KeyCount && sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(k)); }
    bool rawDown(Action a) const {
        const int i = static_cast<int>(a);
        if (keyDown(m_bindings[i])) return true;
        using K = sf::Keyboard;
        switch (a) {  // fixed alternates so menus are always usable
        case Action::Left: return keyDown(K::A);
        case Action::Right: return keyDown(K::D);
        case Action::Up: return keyDown(K::W);
        case Action::Down: return keyDown(K::S);
        case Action::Jump: return keyDown(K::Z) || keyDown(K::K);
        case Action::Attack: return keyDown(K::J) || keyDown(K::LControl);
        case Action::Confirm: return keyDown(K::Space);
        case Action::Pause: return keyDown(K::P);
        case Action::Back: return keyDown(K::Backspace);
        default: return false;
        }
    }
    std::array<int, kActionCount> m_bindings;
    std::array<bool, kActionCount> m_cur{}, m_prev{}, m_edge{};
    sf::Vector2f m_mouse;
    bool m_mouseClicked = false;
    bool m_mouseMoved = false;
    float m_wheel = 0.f;
    int m_lastKey = -1;
};

} // namespace rayman
