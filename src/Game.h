#pragma once
// Top-level application: window, fixed-timestep loop, screen flow, progression and achievements.
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include "AssetManager.h"
#include "Audio.h"
#include "Graphics.h"
#include "HUD.h"
#include "Input.h"
#include "Level.h"
#include "Menu.h"
#include "ParticleSystem.h"
#include "Player.h"
#include "SaveSystem.h"
#include "WorldMap.h"

namespace rayman {

enum class Screen { Loading, LoadError, MainMenu, Options, Extras, Credits, Confirm, WorldMap, Playing, Paused, LevelComplete, GameOver, Victory };

class Game {
public:
    explicit Game(const std::filesystem::path& exeDir);
    int run();
private:
    void createWindow();
    void processEvents();
    void update(float dt);
    void render();
    void updateLoading();
    void finishLoading();
    void updatePlaying(float dt);
    void transition(Screen to, std::function<void()> onSwitch = nullptr, float duration = 0.35f);
    void startLevel(int world, int stage, int checkpoint = -1);
    void respawn();
    void completeLevel();
    void newGame();
    void continueGame();
    void goToMap(int revealWorld = 0, int revealStage = 0);
    void goToMainMenu();
    void unlock(int achievement);
    void checkProgressAchievements();
    void persist();
    void applyAudioSettings();
    sf::FloatRect viewport() const;
    void drawLevelScene(sf::RenderTarget& t);
    void drawLoading(sf::RenderTarget& t);
    std::string ambienceFor(int world) const;

    std::filesystem::path m_exeDir;
    sf::RenderWindow m_window;
    sf::RenderTexture m_scene;
    AssetManager m_assets;
    std::unique_ptr<Audio> m_audio;
    Input m_input;
    SaveSystem m_saves;
    SaveData m_save;
    Settings m_settings;
    ParticleSystem m_fx;
    ParticleSystem m_menuFx;
    Camera m_camera;
    std::unique_ptr<Level> m_level;
    Player m_player;
    HUD m_hud;
    PostFX m_post;
    Fader m_fader;
    MainMenu m_mainMenu;
    OptionsMenu m_options;
    ExtrasMenu m_extras;
    CreditsScreen m_credits;
    PauseMenu m_pause;
    ConfirmDialog m_confirm;
    LevelCompleteScreen m_complete;
    GameOverScreen m_gameOver;
    VictoryScreen m_victory;
    WorldMap m_map;

    Screen m_screen = Screen::Loading;
    Screen m_optionsReturn = Screen::MainMenu;
    Screen m_pendingScreen = Screen::Loading;
    std::function<void()> m_pendingAction;
    bool m_transitioning = false;
    float m_time = 0.f;
    float m_fps = 60.f;
    int m_lives = 5;
    int m_world = 1, m_stage = 1;
    float m_exitTimer = -1.f;
    float m_respawnTimer = -1.f;
    int m_lastHealth = Player::kMaxHealth;
    bool m_bossMusic = false;
    int m_revealWorld = 0, m_revealStage = 0;
    bool m_settingsDirty = false;
    float m_loadingTip = 0.f;
    float m_flash = 0.f;
};

} // namespace rayman
