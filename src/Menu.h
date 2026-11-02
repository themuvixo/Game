#pragma once
// Menu screens: main menu, options (+ key rebinding), extras/gallery, credits, pause, confirm dialog,
// level complete, game over and victory. All screens render at the logical 1280x720 resolution.
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "Animation.h"
#include "SaveSystem.h"

namespace rayman {

class AssetManager;
class Audio;
class Input;
class ParticleSystem;

struct MenuItem { std::string label; bool enabled = true; };

class ListMenu {
public:
    void setItems(const std::vector<MenuItem>& items);
    void setSelected(int i);
    int selected() const { return m_sel; }
    // Returns the activated index or -1.
    int update(float dt, const Input& in, Audio& audio, sf::Vector2f origin, float spacing, float width);
    void draw(sf::RenderTarget& t, const AssetManager& a, sf::Vector2f origin, float spacing, float time, float width, float appear = 1.f) const;
    std::vector<MenuItem>& items() { return m_items; }
private:
    void move(int d, Audio& audio);
    std::vector<MenuItem> m_items;
    int m_sel = 0;
};

enum class MenuAction { None, NewGame, Continue, LevelSelect, Options, Extras, Credits, Quit, Back, Resume, Restart, ToMap,
                        ToMainMenu, Yes, No, Next, PlayAgain, RetryLevel };

class MainMenu {
public:
    void setup(const AssetManager& a);
    void open(bool hasSave);
    MenuAction update(float dt, const Input& in, Audio& audio, ParticleSystem& fx);
    void draw(sf::RenderTarget& t, const AssetManager& a, const ParticleSystem& fx, float time) const;
private:
    ListMenu m_list;
    Animator m_hero;
    float m_t = 0.f;
};

struct OptionsResult { bool back = false; bool audioChanged = false; bool videoChanged = false; bool keysChanged = false; };

class OptionsMenu {
public:
    void open(Settings& s, bool startOnControls = false);
    OptionsResult update(float dt, Input& in, Audio& audio);
    void draw(sf::RenderTarget& t, const AssetManager& a, float time) const;
private:
    void rebuild();
    Settings* m_s = nullptr;
    ListMenu m_list;
    bool m_controls = false;
    int m_waitingKey = -1;
    float m_t = 0.f;
};

class ExtrasMenu {
public:
    void open(const SaveData& save);
    bool update(float dt, const Input& in, Audio& audio);  // true = back
    void draw(sf::RenderTarget& t, const AssetManager& a, float time) const;
private:
    const SaveData* m_save = nullptr;
    int m_tab = 0;
    int m_sel = 0;
    float m_t = 0.f;
};

class CreditsScreen {
public:
    void open() { m_scroll = 0.f; }
    bool update(float dt, const Input& in);
    void draw(sf::RenderTarget& t, const AssetManager& a, float time) const;
private:
    float m_scroll = 0.f;
};

class PauseMenu {
public:
    void open();
    MenuAction update(float dt, const Input& in, Audio& audio);
    void draw(sf::RenderTarget& t, const AssetManager& a, const Input& in, float time) const;
    bool showingControls() const { return m_controls; }
private:
    ListMenu m_list;
    bool m_controls = false;
    float m_t = 0.f;
};

class ConfirmDialog {
public:
    void open(const std::string& question, const std::string& detail);
    MenuAction update(float dt, const Input& in, Audio& audio);
    void draw(sf::RenderTarget& t, const AssetManager& a, float time) const;
private:
    std::string m_q, m_d;
    ListMenu m_list;
    float m_t = 0.f;
};

struct StageResults {
    std::string name;
    int world = 1, stage = 1;
    bool boss = false, finalBoss = false;
    int score = 0, tings = 0, tingsTotal = 0, cages = 0, cagesTotal = 0, gems = 0, gemsTotal = 0;
    int relics = 0, relicsTotal = 0, secrets = 0, secretsTotal = 0, enemies = 0, deaths = 0;
    float time = 0.f, par = 0.f, completion = 0.f;
    int stars = 1;
    std::string rank = "C";
    bool newBest = false;
    std::vector<std::string> rewards;
};

class LevelCompleteScreen {
public:
    void open(const StageResults& r);
    MenuAction update(float dt, const Input& in, Audio& audio);
    void draw(sf::RenderTarget& t, const AssetManager& a, float time) const;
private:
    StageResults m_r;
    ListMenu m_list;
    float m_t = 0.f;
    int m_starsShown = 0;
};

class GameOverScreen {
public:
    void open(int score, const std::string& stage);
    MenuAction update(float dt, const Input& in, Audio& audio);
    void draw(sf::RenderTarget& t, const AssetManager& a, float time) const;
private:
    ListMenu m_list;
    int m_score = 0;
    std::string m_stage;
    float m_t = 0.f;
};

class VictoryScreen {
public:
    void setup(const AssetManager& a);
    void open(const SaveData& save);
    MenuAction update(float dt, const Input& in, Audio& audio, ParticleSystem& fx);
    void draw(sf::RenderTarget& t, const AssetManager& a, const ParticleSystem& fx, float time) const;
private:
    ListMenu m_list;
    Animator m_hero;
    float m_t = 0.f;
    long long m_score = 0;
    int m_tings = 0, m_lumlings = 0, m_gems = 0, m_relics = 0, m_worlds = 0;
    float m_completion = 0.f;
};

const char* enemyDisplayName(int world, int type);
void drawMenuBackdrop(sf::RenderTarget& t, const AssetManager& a, float time, int world = 1, float dim = 0.f);

} // namespace rayman
