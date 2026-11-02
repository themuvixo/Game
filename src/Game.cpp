#include "Game.h"
#include "Boss.h"
#include "Enemy.h"

namespace rayman {

namespace {
const char* kTips[] = {
    "Hold Jump while falling to spin your hood and glide.",
    "Hold Attack to charge the Magic Fist: it flies further and breaks shells.",
    "Armored foes block punches from the front. Hit them from behind!",
    "Free every Lumling in a stage to earn a star.",
    "Hidden walls shimmer faintly. Walk into suspicious rocks.",
    "Find the secret key in each world's third stage to open a secret path.",
    "Bosses can only be hurt while their weak spot glows.",
    "Collect 100 Tings for an extra life.",
    "Springs and bounce pads launch you to hidden heights.",
};
constexpr int kTipCount = static_cast<int>(sizeof(kTips) / sizeof(kTips[0]));
}

Game::Game(const std::filesystem::path& exeDir) : m_exeDir(exeDir) {}

int Game::run() {
    m_saves.loadSettings(m_settings);
    m_input.setBindings(m_settings.keys);
    createWindow();
    if (!m_scene.create(kViewW, kViewH)) {
        logLine("[gfx] could not create the render texture");
        return 2;
    }
    m_scene.setSmooth(true);
    const bool found = m_assets.locateAssetRoot(m_exeDir);
    if (!found) logLine("[assets] asset folder not found next to the executable: " + m_assets.root().string());
    m_assets.queueManifest();
    m_audio = std::make_unique<Audio>(m_assets);
    applyAudioSettings();
    m_fader.set(0.f);
    sf::Clock clock;
    float acc = 0.f;
    while (m_window.isOpen()) {
        float frame = clock.restart().asSeconds();
        if (frame > 0.25f) frame = 0.25f;
        m_fps = lerpf(m_fps, frame > 0.f ? 1.f / frame : 60.f, 0.05f);
        processEvents();
        if (!m_window.isOpen()) break;
        m_input.poll(m_window.hasFocus());
        acc += frame;
        int steps = 0;
        while (acc >= kFixedDt && steps < 8) {
            update(kFixedDt);
            m_input.consumeEdges();
            m_input.clearMouseMoved();
            acc -= kFixedDt;
            ++steps;
        }
        if (steps == 8) acc = 0.f;  // spiral-of-death guard
        render();
    }
    if (m_save.exists) persist();
    if (m_settingsDirty) m_saves.saveSettings(m_settings);
    return 0;
}

void Game::createWindow() {
    const sf::ContextSettings cs(0, 0, 0);
    if (m_settings.fullscreen) {
        m_window.create(sf::VideoMode::getDesktopMode(), "Lumo and the Shattered Dream", sf::Style::Fullscreen, cs);
    } else {
        const Resolution r = resolutionAt(m_settings.resolution);
        m_window.create(sf::VideoMode(r.w, r.h), "Lumo and the Shattered Dream", sf::Style::Titlebar | sf::Style::Close, cs);
    }
    m_window.setKeyRepeatEnabled(false);
    m_window.setVerticalSyncEnabled(m_settings.vsync);
    m_window.setFramerateLimit(m_settings.vsync ? 0 : 60);
    if (const sf::Image* icon = m_assets.image("icon")) m_window.setIcon(icon->getSize().x, icon->getSize().y, icon->getPixelsPtr());
}

sf::FloatRect Game::viewport() const {
    const sf::Vector2u ws = m_window.getSize();
    const float sx = static_cast<float>(ws.x) / static_cast<float>(kViewW), sy = static_cast<float>(ws.y) / static_cast<float>(kViewH);
    const float s = std::min(sx, sy);
    const float w = kViewW * s, h = kViewH * s;
    return sf::FloatRect((static_cast<float>(ws.x) - w) * 0.5f, (static_cast<float>(ws.y) - h) * 0.5f, w, h);
}

void Game::processEvents() {
    sf::Event e;
    while (m_window.pollEvent(e)) {
        switch (e.type) {
        case sf::Event::Closed:
            m_window.close();
            break;
        case sf::Event::LostFocus:
            if (m_screen == Screen::Playing) { m_pause.open(); m_screen = Screen::Paused; m_audio->setDucked(true); }
            break;
        case sf::Event::KeyPressed:
            m_input.setLastKey(static_cast<int>(e.key.code));
            break;
        case sf::Event::MouseMoved: {
            const sf::FloatRect vp = viewport();
            m_input.setMouse(sf::Vector2f((static_cast<float>(e.mouseMove.x) - vp.left) / vp.width * kViewW,
                                          (static_cast<float>(e.mouseMove.y) - vp.top) / vp.height * kViewH), true);
            break;
        }
        case sf::Event::MouseButtonPressed:
            if (e.mouseButton.button == sf::Mouse::Left) {
                const sf::FloatRect vp = viewport();
                m_input.setMouse(sf::Vector2f((static_cast<float>(e.mouseButton.x) - vp.left) / vp.width * kViewW,
                                              (static_cast<float>(e.mouseButton.y) - vp.top) / vp.height * kViewH), false);
                m_input.click();
            }
            break;
        case sf::Event::MouseWheelScrolled:
            m_input.wheel(e.mouseWheelScroll.delta);
            break;
        default:
            break;
        }
    }
}

void Game::applyAudioSettings() {
    if (m_audio) m_audio->setVolumes(m_settings.master, m_settings.music, m_settings.sfx);
}

void Game::persist() {
    if (!m_saves.save(m_save)) logLine("[save] " + m_saves.lastMessage());
}

void Game::transition(Screen to, std::function<void()> onSwitch, float duration) {
    if (m_transitioning) return;
    m_transitioning = true;
    m_pendingScreen = to;
    m_pendingAction = std::move(onSwitch);
    m_fader.fadeOut(duration);
}

std::string Game::ambienceFor(int world) const {
    const char* a[kWorlds] = {"amb_forest", "amb_band", "amb_city", "amb_snow", "amb_lava"};
    return (world >= 1 && world <= kWorlds) ? a[world - 1] : "";
}

// ------------------------------------------------------------------ loading
void Game::updateLoading() {
    sf::Clock budget;
    while (!m_assets.done() && budget.getElapsedTime().asMilliseconds() < 12) m_assets.loadNext();
    if (m_assets.done() && !m_transitioning) finishLoading();
}

void Game::finishLoading() {
    if (!m_assets.fontOk() || m_assets.criticalMissing()) {
        m_screen = Screen::LoadError;
        logLine("[assets] critical assets missing; showing error screen");
        return;
    }
    m_post.init(m_assets);
    m_fx.setTextures(&m_assets.texture("particle_soft"), &m_assets.texture("particle_star"), &m_assets.texture("particle_dust"));
    m_menuFx.setTextures(&m_assets.texture("particle_soft"), &m_assets.texture("particle_star"), &m_assets.texture("particle_dust"));
    m_mainMenu.setup(m_assets);
    m_victory.setup(m_assets);
    m_map.setup(m_assets);
    m_player.setup(m_assets);
    if (const sf::Image* icon = m_assets.image("icon")) m_window.setIcon(icon->getSize().x, icon->getSize().y, icon->getPixelsPtr());
    m_saves.load(m_save);
    m_save.stage(1, 1).unlocked = true;
    transition(Screen::MainMenu, [this]() {
        m_mainMenu.open(m_save.exists);
        m_audio->playMusic("menu", 1.5f);
    }, 0.5f);
}

// ------------------------------------------------------------------ flow helpers
void Game::goToMainMenu() {
    transition(Screen::MainMenu, [this]() {
        m_level.reset();
        m_fx.clear();
        m_mainMenu.open(m_save.exists);
        m_audio->playMusic("menu");
        m_audio->playAmbience("");
        m_audio->setDucked(false);
    });
}

void Game::goToMap(int revealWorld, int revealStage) {
    transition(Screen::WorldMap, [this, revealWorld, revealStage]() {
        m_level.reset();
        m_fx.clear();
        m_map.enter(m_save, revealWorld, revealStage);
        m_audio->playMusic("worldmap");
        m_audio->playAmbience("");
        m_audio->setDucked(false);
    });
}

void Game::newGame() {
    m_save.resetProgress();
    m_lives = 5;
    persist();
    m_audio->playAmbience("");
    startLevel(1, 1);
}

void Game::continueGame() {
    m_lives = 5;
    if (m_save.resumeWorld > 0 && m_save.resumeStage > 0 && m_save.stage(m_save.resumeWorld, m_save.resumeStage).unlocked)
        startLevel(m_save.resumeWorld, m_save.resumeStage, m_save.resumeCheckpoint);
    else
        goToMap();
}

void Game::startLevel(int world, int stage, int checkpoint) {
    transition(Screen::Playing, [this, world, stage, checkpoint]() {
        auto lvl = std::make_unique<Level>();
        if (!lvl->load(m_assets, world, stage)) {
            m_hud.toast("Stage unavailable", "Level data for " + toStr(world) + "-" + toStr(stage) + " is missing.");
            m_pendingScreen = Screen::WorldMap;
            m_map.enter(m_save);
            m_audio->playMusic("worldmap");
            return;
        }
        m_level = std::move(lvl);
        m_world = world;
        m_stage = stage;
        if (checkpoint >= 0) m_level->restart(checkpoint);
        m_player.setDoubleJump(m_save.hasDoubleJump);
        m_player.spawn(m_level->respawnPos(), true);
        m_lastHealth = m_player.health();
        m_camera.reset(m_player.center(), m_level->bounds());
        m_fx.clear();
        m_exitTimer = -1.f;
        m_respawnTimer = -1.f;
        m_bossMusic = false;
        m_audio->playMusic(m_level->info().music, 1.0f);
        m_audio->playAmbience(ambienceFor(world));
        m_audio->setDucked(false);
        m_save.mapWorld = world;
        m_save.mapStage = stage;
        m_hud.toast(toStr(world) + "-" + toStr(stage) + "  " + m_level->info().name,
                    m_level->info().kind == StageKind::Boss ? "Boss stage - find the weak spot!" :
                    m_level->info().kind == StageKind::Challenge ? "Challenge stage - beat the par time!" :
                    m_level->info().kind == StageKind::Secret ? "Secret stage - treasure awaits." : "Free the Lumlings!");
    });
}

void Game::respawn() {
    m_player.spawn(m_level->respawnPos(), true);
    m_level->onPlayerRespawn();
    m_lastHealth = m_player.health();
    m_camera.reset(m_player.center(), m_level->bounds());
    if (m_bossMusic) { m_audio->playMusic(m_level->info().music); m_bossMusic = false; }
}

void Game::unlock(int a) {
    if (a < 0 || a >= AchCount || m_save.hasAchievement(a)) return;
    m_save.achievements |= (1u << a);
    m_hud.toast(std::string("Achievement: ") + achievementName(a), achievementDesc(a));
    m_audio->play("achievement");
}

void Game::checkProgressAchievements() {
    if (m_save.stats.levelsCompleted >= 1) unlock(AchFirstAdventure);
    if (m_save.stats.totalTings >= 1000) unlock(AchCollector);
    if (m_save.stats.gemsCollected >= 25) unlock(AchTreasureHunter);
    if (m_save.stats.lumlingsFreed >= 50) unlock(AchLumlingSavior);
    if (m_save.stats.bossesDefeated >= 1) unlock(AchBossSlayer);
    if (m_save.hasDoubleJump) unlock(AchDoubleJump);
    bool allSecrets = true;
    for (int w = 1; w <= kWorlds; ++w) {
        if (m_save.stage(w, kSecretStage).unlocked) unlock(AchSecretFinder);
        if (!m_save.stage(w, kSecretStage).completed) allSecrets = false;
    }
    if (allSecrets) unlock(AchExplorer);
    if (m_save.bossDefeated[kWorlds - 1]) unlock(AchAllWorlds);
}

// ------------------------------------------------------------------ completion & progression
void Game::completeLevel() {
    const RunStats& s = m_level->stats();
    const LevelInfo& info = m_level->info();
    const bool boss = info.kind == StageKind::Boss;
    StageResults r;
    r.name = info.name;
    r.world = m_world;
    r.stage = m_stage;
    r.boss = boss;
    r.finalBoss = boss && m_world == kWorlds;
    r.tings = s.tings; r.tingsTotal = s.tingsTotal;
    r.cages = s.cages; r.cagesTotal = s.cagesTotal;
    r.gems = s.gems; r.gemsTotal = s.gemsTotal;
    r.relics = s.relics; r.relicsTotal = s.relicsTotal;
    r.secrets = s.secrets; r.secretsTotal = s.secretsTotal;
    r.enemies = s.enemies; r.deaths = s.deaths;
    r.time = s.time; r.par = info.par;
    // completion: weighted share of everything that can be found
    auto frac = [](int got, int total) { return total > 0 ? static_cast<float>(got) / static_cast<float>(total) : 1.f; };
    r.completion = boss ? 1.f : (0.4f * frac(s.tings, s.tingsTotal) + 0.3f * frac(s.cages, s.cagesTotal) + 0.15f * frac(s.gems, s.gemsTotal) +
                                 0.15f * frac(s.secrets + s.relics, s.secretsTotal + s.relicsTotal));
    r.stars = 1;
    if (boss) {
        if (s.deaths == 0) ++r.stars;
        if (s.time <= info.par) ++r.stars;
    } else {
        if (s.cages >= s.cagesTotal) ++r.stars;
        if (frac(s.tings, s.tingsTotal) >= 0.9f && s.gems >= s.gemsTotal && s.time <= info.par) ++r.stars;
    }
    r.rank = r.completion >= 0.95f ? "S" : (r.completion >= 0.8f ? "A" : (r.completion >= 0.6f ? "B" : "C"));
    int score = s.score;
    const int timeBonus = static_cast<int>(std::max(0.f, info.par - s.time) * 10.f);
    score += timeBonus;
    if (s.deaths == 0) score += 1000;
    if (s.cagesTotal > 0 && s.cages >= s.cagesTotal) score += 1000;
    r.score = score;
    if (timeBonus > 0) r.rewards.push_back("Time bonus +" + toStr(timeBonus));
    if (s.deaths == 0) r.rewards.push_back("Flawless run +1000");

    StageRecord& rec = m_save.stage(m_world, m_stage);
    r.newBest = score > rec.bestScore;
    rec.completed = true;
    rec.unlocked = true;
    rec.bestScore = std::max(rec.bestScore, score);
    rec.bestTings = std::max(rec.bestTings, s.tings); rec.tingsTotal = s.tingsTotal;
    rec.bestCages = std::max(rec.bestCages, s.cages); rec.cagesTotal = s.cagesTotal;
    rec.bestGems = std::max(rec.bestGems, s.gems); rec.gemsTotal = s.gemsTotal;
    rec.bestRelics = std::max(rec.bestRelics, s.relics); rec.relicsTotal = s.relicsTotal;
    rec.secretsFound = std::max(rec.secretsFound, s.secrets); rec.secretsTotal = s.secretsTotal;
    rec.stars = std::max(rec.stars, r.stars);
    rec.completion = std::max(rec.completion, r.completion);
    rec.bestTime = rec.bestTime <= 0.f ? s.time : std::min(rec.bestTime, s.time);
    if (s.keyFound) rec.keyFound = true;

    // unlocks
    m_revealWorld = 0;
    m_revealStage = 0;
    auto open = [this, &r](int w, int st, const std::string& what) {
        if (w > kWorlds) return;
        StageRecord& nr = m_save.stage(w, st);
        if (!nr.unlocked) {
            nr.unlocked = true;
            if (m_revealWorld == 0) { m_revealWorld = w; m_revealStage = st; }
            r.rewards.push_back(what);
        }
    };
    if (m_stage >= 1 && m_stage <= 4) open(m_world, m_stage + 1, "Stage " + toStr(m_world) + "-" + toStr(m_stage + 1) + " unlocked");
    if (m_stage == 5) open(m_world, kBossStage, "Boss lair unlocked");
    if (m_stage == 3 && rec.keyFound) open(m_world, kSecretStage, "Secret path revealed!");
    if (boss) {
        if (!m_save.bossDefeated[static_cast<size_t>(m_world - 1)]) ++m_save.stats.bossesDefeated;
        m_save.bossDefeated[static_cast<size_t>(m_world - 1)] = true;
        if (m_world < kWorlds) open(m_world + 1, 1, "World " + toStr(m_world + 1) + " unlocked");
        else m_save.gameCompleted = true;
        if (m_world == 1 && !m_save.hasDoubleJump) {
            m_save.hasDoubleJump = true;
            r.rewards.push_back("Double Jump learned!");
        }
    }
    if (s.upgradeFound) m_save.hasDoubleJump = true;
    // stats
    m_save.stats.totalTings += s.tings;
    m_save.stats.lumlingsFreed += s.cages;
    m_save.stats.gemsCollected += s.gems;
    m_save.stats.relicsCollected += s.relics;
    m_save.stats.secretsFound += s.secrets;
    ++m_save.stats.levelsCompleted;
    m_save.resumeWorld = m_save.resumeStage = 0;
    m_save.resumeCheckpoint = -1;
    m_save.mapWorld = m_world;
    m_save.mapStage = m_stage;
    // gameplay-event achievements
    if (r.stars >= 3) unlock(AchPerfectLevel);
    if (s.damageTaken == 0 && s.deaths == 0) unlock(AchNoDamage);
    if (s.time < info.par * 0.5f) unlock(AchSpeedRunner);
    checkProgressAchievements();
    persist();
    m_complete.open(r);
    m_screen = Screen::LevelComplete;
    m_audio->playMusic(r.finalBoss ? "victory" : "worldmap", 1.5f);
    m_audio->playAmbience("");
}

// ------------------------------------------------------------------ update
void Game::update(float dt) {
    m_time += dt;
    m_fader.update(dt);
    m_hud.update(dt);
    m_flash = std::max(0.f, m_flash - dt * 2.5f);
    m_post.setFlash(m_flash * 0.55f);
    if (m_audio) m_audio->update(dt);
    if (m_transitioning && m_fader.fullyOut()) {
        Screen target = m_pendingScreen;
        if (m_pendingAction) {
            auto act = std::move(m_pendingAction);
            m_pendingAction = nullptr;
            act();
            target = m_pendingScreen;  // action may redirect
        }
        m_screen = target;
        m_transitioning = false;
        m_fader.fadeIn(0.4f);
    }
    if (m_screen != Screen::Playing) m_menuFx.update(dt);

    switch (m_screen) {
    case Screen::Loading:
        updateLoading();
        break;
    case Screen::LoadError:
        if (m_input.pressed(Action::Confirm) || m_input.pressed(Action::Back)) m_window.close();
        break;
    case Screen::MainMenu: {
        if (m_transitioning) break;
        switch (m_mainMenu.update(dt, m_input, *m_audio, m_menuFx)) {
        case MenuAction::NewGame: {
            bool progress = false;
            for (int w = 1; w <= kWorlds; ++w) for (int s = 1; s <= kStagesPerWorld; ++s) progress = progress || m_save.stage(w, s).completed;
            if (m_save.exists && progress) {
                m_confirm.open("Start a new game?", "Your current progress will be overwritten.");
                m_screen = Screen::Confirm;
            } else {
                newGame();
            }
            break;
        }
        case MenuAction::Continue: continueGame(); break;
        case MenuAction::LevelSelect: m_lives = 5; goToMap(); break;
        case MenuAction::Options: m_options.open(m_settings); m_optionsReturn = Screen::MainMenu; m_screen = Screen::Options; break;
        case MenuAction::Extras: m_extras.open(m_save); m_screen = Screen::Extras; break;
        case MenuAction::Credits: m_credits.open(); m_screen = Screen::Credits; break;
        case MenuAction::Quit: m_window.close(); break;
        default: break;
        }
        break;
    }
    case Screen::Confirm: {
        const MenuAction a = m_confirm.update(dt, m_input, *m_audio);
        if (a == MenuAction::Yes) newGame();
        else if (a == MenuAction::No) m_screen = Screen::MainMenu;
        break;
    }
    case Screen::Options: {
        const OptionsResult r = m_options.update(dt, m_input, *m_audio);
        if (r.audioChanged) { applyAudioSettings(); m_settingsDirty = true; }
        if (r.keysChanged) { m_input.setBindings(m_settings.keys); m_settingsDirty = true; }
        if (r.videoChanged) { createWindow(); m_settingsDirty = true; }
        if (r.back) {
            if (m_settingsDirty) { m_saves.saveSettings(m_settings); m_settingsDirty = false; }
            m_screen = m_optionsReturn;
            if (m_screen == Screen::MainMenu) m_mainMenu.open(m_save.exists);
        }
        break;
    }
    case Screen::Extras:
        if (m_extras.update(dt, m_input, *m_audio)) { m_screen = Screen::MainMenu; m_mainMenu.open(m_save.exists); }
        break;
    case Screen::Credits:
        if (m_credits.update(dt, m_input)) { m_screen = Screen::MainMenu; m_mainMenu.open(m_save.exists); }
        break;
    case Screen::WorldMap: {
        if (m_transitioning) break;
        const MapResult r = m_map.update(dt, m_input, *m_audio, m_menuFx, m_save);
        if (r.kind == MapResult::Enter) startLevel(r.world, r.stage);
        else if (r.kind == MapResult::Back) { persist(); goToMainMenu(); }
        break;
    }
    case Screen::Playing:
        updatePlaying(dt);
        break;
    case Screen::Paused: {
        m_fx.update(0.f);
        const MenuAction a = m_pause.update(dt, m_input, *m_audio);
        if (a == MenuAction::Resume) { m_screen = Screen::Playing; m_audio->setDucked(false); }
        else if (a == MenuAction::Restart) { m_audio->setDucked(false); startLevel(m_world, m_stage); }
        else if (a == MenuAction::Options) { m_options.open(m_settings); m_optionsReturn = Screen::Paused; m_screen = Screen::Options; }
        else if (a == MenuAction::ToMap) {
            m_save.resumeWorld = m_save.resumeStage = 0;
            persist();
            goToMap();
        } else if (a == MenuAction::ToMainMenu) { persist(); goToMainMenu(); }
        break;
    }
    case Screen::LevelComplete: {
        if (m_transitioning) break;
        if (m_level) {
            m_player.update(dt, m_input, *m_level, *m_audio, m_fx);
            m_fx.update(dt);
            m_camera.update(dt, m_player.center(), m_player.velocity(), m_player.facing(), m_player.grounded());
        }
        const MenuAction a = m_complete.update(dt, m_input, *m_audio);
        if (a == MenuAction::Next) {
            if (m_world == kWorlds && m_stage == kBossStage) {
                transition(Screen::Victory, [this]() {
                    m_level.reset();
                    m_victory.open(m_save);
                    m_menuFx.clear();
                    m_audio->playMusic("victory");
                }, 0.8f);
            } else {
                int nw = m_world, ns = m_stage + 1;
                if (m_stage == 5) ns = kBossStage;
                if (m_stage == kBossStage) { nw = m_world + 1; ns = 1; }
                if (m_stage == kSecretStage) ns = 4;
                if (nw <= kWorlds && m_save.stage(nw, ns).unlocked) startLevel(nw, ns);
                else goToMap(m_revealWorld, m_revealStage);
            }
        } else if (a == MenuAction::ToMap) {
            goToMap(m_revealWorld, m_revealStage);
        }
        break;
    }
    case Screen::GameOver: {
        if (m_transitioning) break;
        const MenuAction a = m_gameOver.update(dt, m_input, *m_audio);
        if (a == MenuAction::RetryLevel) { m_lives = 5; startLevel(m_world, m_stage); }
        else if (a == MenuAction::ToMap) { m_lives = 5; goToMap(); }
        break;
    }
    case Screen::Victory: {
        if (m_transitioning) break;
        const MenuAction a = m_victory.update(dt, m_input, *m_audio, m_menuFx);
        if (a == MenuAction::PlayAgain) { m_lives = 5; startLevel(1, 1); }
        else if (a == MenuAction::ToMainMenu) goToMainMenu();
        break;
    }
    }
}

void Game::updatePlaying(float dt) {
    if (!m_level) return;
    if (m_transitioning) return;
    if (m_input.pressed(Action::Pause) && m_exitTimer < 0.f && m_player.alive()) {
        m_pause.open();
        m_screen = Screen::Paused;
        m_audio->setDucked(true);
        m_audio->play("menu_select");
        return;
    }
    m_save.stats.playTime += dt;
    const int jumpsBefore = m_player.jumpsMade();
    m_player.update(dt, m_input, *m_level, *m_audio, m_fx);
    m_save.stats.jumps += m_player.jumpsMade() - jumpsBefore;
    m_level->update(dt, m_player, *m_audio, m_fx, m_camera);
    if (m_player.health() < m_lastHealth) {
        m_level->stats().damageTaken += m_lastHealth - m_player.health();
        m_camera.shake(6.f, 0.25f);
        m_flash = std::max(m_flash, 0.35f);
    }
    m_lastHealth = m_player.health();
    m_camera.update(dt, m_player.center(), m_player.velocity(), m_player.facing(), m_player.grounded());
    m_fx.update(dt);
    const AmbientKind amb[kWorlds] = {AmbientKind::Petals, AmbientKind::Notes, AmbientKind::Neon, AmbientKind::Snow, AmbientKind::Embers};
    m_fx.ambient(amb[m_world - 1], m_camera.visibleRect(), dt);

    LevelEvents& ev = m_level->events();
    if (ev.checkpoint) {
        ev.checkpoint = false;
        m_hud.flashCheckpoint();
        m_save.resumeWorld = m_world;
        m_save.resumeStage = m_stage;
        m_save.resumeCheckpoint = m_level->activeCheckpoint();
        persist();
    }
    if (ev.key) {
        ev.key = false;
        m_save.stage(m_world, m_stage).keyFound = true;
        if (m_stage == 3 && !m_save.stage(m_world, kSecretStage).unlocked) {
            m_save.stage(m_world, kSecretStage).unlocked = true;
            m_hud.toast("Secret path revealed!", "A hidden stage has appeared on the world map.");
        }
        checkProgressAchievements();
        persist();
    }
    if (ev.upgrade) {
        ev.upgrade = false;
        m_save.hasDoubleJump = true;
        m_hud.toast("Double Jump!", "Press Jump again in mid-air.");
        checkProgressAchievements();
        persist();
    }
    if (ev.secret) { ev.secret = false; }
    if (ev.extraLife) { ev.extraLife = false; ++m_lives; }
    for (int code : ev.enemyKills) {
        if (code >= 0 && code < 32) m_save.enemiesSeen |= (1u << code);
        ++m_save.stats.enemiesDefeated;
    }
    ev.enemyKills.clear();
    if (ev.bossDefeated) {
        ev.bossDefeated = false;
        m_audio->playMusic(m_level->info().music == "boss_final" ? "victory" : "world" + toStr(m_world), 1.5f);
        m_flash = 1.f;
    }
    if (m_level->arenaLocked() && !m_bossMusic) {
        m_bossMusic = true;
        m_audio->playMusic(m_world == kWorlds ? "boss_final" : "boss", 0.8f);
        m_hud.toast(bossName(m_world), "Dodge the attacks - strike when the weak spot glows!");
    }
    if (ev.exitReached && m_exitTimer < 0.f) {
        m_exitTimer = 0.f;
        m_player.celebrate();
    }
    if (m_exitTimer >= 0.f) {
        m_exitTimer += dt;
        if (m_exitTimer > 2.3f) {
            m_exitTimer = -2.f;
            completeLevel();
        }
        return;
    }
    if (m_player.deathFinished()) {
        ++m_level->stats().deaths;
        ++m_save.stats.deaths;
        --m_lives;
        if (m_lives <= 0) {
            m_gameOver.open(m_level->stats().score, toStr(m_world) + "-" + toStr(m_stage) + "  " + m_level->info().name);
            m_screen = Screen::GameOver;
            m_audio->playMusic("gameover", 0.5f);
            m_save.resumeWorld = m_save.resumeStage = 0;
            persist();
        } else {
            transition(Screen::Playing, [this]() { respawn(); }, 0.3f);
        }
    }
}

// ------------------------------------------------------------------ render
void Game::drawLevelScene(sf::RenderTarget& t) {
    if (!m_level) return;
    t.setView(m_camera.view());
    m_level->drawBackground(t, m_camera, m_time);
    m_level->drawWorld(t, m_camera, m_time);
    m_player.draw(t, m_time);
    m_fx.draw(t);
    m_level->drawForeground(t, m_camera, m_time);
    t.setView(t.getDefaultView());
}

void Game::drawLoading(sf::RenderTarget& t) {
    // animated gradient backdrop + logo + real progress
    sf::VertexArray bg(sf::TriangleStrip, 4);
    const float k = 0.5f + 0.5f * std::sin(m_time * 0.8f);
    bg[0] = sf::Vertex(sf::Vector2f(0.f, 0.f), mixColor(sf::Color(40, 16, 80), sf::Color(70, 20, 110), k));
    bg[1] = sf::Vertex(sf::Vector2f(static_cast<float>(kViewW), 0.f), sf::Color(30, 20, 90));
    bg[2] = sf::Vertex(sf::Vector2f(0.f, static_cast<float>(kViewH)), sf::Color(150, 60, 160));
    bg[3] = sf::Vertex(sf::Vector2f(static_cast<float>(kViewW), static_cast<float>(kViewH)), mixColor(sf::Color(200, 90, 170), sf::Color(120, 60, 200), k));
    t.draw(bg);
    for (int i = 0; i < 40; ++i) {
        const float x = std::fmod(static_cast<float>(i) * 97.f + m_time * (10.f + static_cast<float>(i % 5) * 6.f), static_cast<float>(kViewW));
        const float y = std::fmod(static_cast<float>(i) * 53.f - m_time * (20.f + static_cast<float>(i % 7) * 4.f) + 2000.f, static_cast<float>(kViewH));
        sf::CircleShape s(1.5f + static_cast<float>(i % 3));
        s.setPosition(x, y);
        s.setFillColor(sf::Color(255, 240, 200, static_cast<sf::Uint8>(100 + (i * 37) % 150)));
        t.draw(s);
    }
    if (m_assets.hasTexture("logo")) {
        const sf::Texture& logo = m_assets.texture("logo");
        sf::Sprite ls(logo);
        ls.setOrigin(static_cast<float>(logo.getSize().x) * 0.5f, static_cast<float>(logo.getSize().y) * 0.5f);
        ls.setPosition(640.f, 250.f + std::sin(m_time * 2.f) * 6.f);
        ls.setScale(0.8f, 0.8f);
        t.draw(ls);
    }
    const float p = m_assets.progress();
    ui::bar(t, sf::FloatRect(340.f, 470.f, 600.f, 24.f), p, sf::Color(255, 200, 120));
    if (m_assets.fontOk()) {
        const sf::Font& f = m_assets.font("ui");
        ui::text(t, f, "Loading  " + toStr(static_cast<int>(p * 100.f)) + "%", 24, sf::Vector2f(640.f, 430.f), sf::Color(255, 245, 220), ui::Align::Center);
        ui::text(t, m_assets.font("body"), m_assets.currentLabel(), 16, sf::Vector2f(640.f, 504.f), sf::Color(220, 200, 255), ui::Align::Center);
        const int tip = static_cast<int>(m_time / 3.f) % kTipCount;
        ui::text(t, m_assets.font("body"), std::string("Tip: ") + kTips[tip], 20, sf::Vector2f(640.f, 600.f), sf::Color(255, 230, 250), ui::Align::Center);
    }
}

void Game::render() {
    m_scene.setView(m_scene.getDefaultView());
    m_scene.clear(sf::Color(12, 6, 24));
    switch (m_screen) {
    case Screen::Loading:
        drawLoading(m_scene);
        break;
    case Screen::LoadError: {
        drawLoading(m_scene);
        if (m_assets.fontOk()) {
            ui::panel(m_scene, m_assets.texture("panel"), sf::FloatRect(200.f, 120.f, 880.f, 480.f), 0.97f);
            ui::text(m_scene, m_assets.font("ui"), "Required game files are missing", 30, sf::Vector2f(640.f, 150.f), sf::Color(255, 140, 150), ui::Align::Center);
            float y = 210.f;
            const auto& miss = m_assets.missing();
            for (size_t i = 0; i < miss.size() && i < 9; ++i) {
                ui::text(m_scene, m_assets.font("body"), miss[i], 18, sf::Vector2f(640.f, y), sf::Color::White, ui::Align::Center);
                y += 28.f;
            }
            ui::text(m_scene, m_assets.font("body"), "Re-extract the full ZIP so 'assets' sits next to the .exe.  Press Enter to quit.", 18,
                     sf::Vector2f(640.f, 540.f), sf::Color(255, 230, 160), ui::Align::Center);
        }
        break;
    }
    case Screen::MainMenu:
    case Screen::Confirm:
        m_mainMenu.draw(m_scene, m_assets, m_menuFx, m_time);
        if (m_screen == Screen::Confirm) m_confirm.draw(m_scene, m_assets, m_time);
        break;
    case Screen::Options:
        if (m_optionsReturn == Screen::Paused && m_level) drawLevelScene(m_scene);
        else drawMenuBackdrop(m_scene, m_assets, m_time, 1, 0.45f);
        m_options.draw(m_scene, m_assets, m_time);
        break;
    case Screen::Extras:
        drawMenuBackdrop(m_scene, m_assets, m_time, 2, 0.5f);
        m_extras.draw(m_scene, m_assets, m_time);
        break;
    case Screen::Credits:
        drawMenuBackdrop(m_scene, m_assets, m_time, 4, 0.55f);
        m_credits.draw(m_scene, m_assets, m_time);
        break;
    case Screen::WorldMap:
        m_map.draw(m_scene, m_assets, m_menuFx, m_time);
        m_hud.drawToasts(m_scene, m_assets);
        break;
    case Screen::Playing:
    case Screen::Paused:
    case Screen::LevelComplete:
    case Screen::GameOver:
        drawLevelScene(m_scene);
        if (m_level && m_screen != Screen::LevelComplete) m_hud.draw(m_scene, m_assets, *m_level, m_player, m_lives, m_fps, m_settings.showFps);
        if (m_screen == Screen::Paused) m_pause.draw(m_scene, m_assets, m_input, m_time);
        if (m_screen == Screen::LevelComplete) { m_complete.draw(m_scene, m_assets, m_time); m_hud.drawToasts(m_scene, m_assets); }
        if (m_screen == Screen::GameOver) m_gameOver.draw(m_scene, m_assets, m_time);
        break;
    case Screen::Victory:
        m_victory.draw(m_scene, m_assets, m_menuFx, m_time);
        m_hud.drawToasts(m_scene, m_assets);
        break;
    }
    if (m_settings.showFps && m_screen != Screen::Playing && m_assets.fontOk())
        ui::text(m_scene, m_assets.font("body"), "FPS " + toStr(static_cast<int>(m_fps + 0.5f)), 18, sf::Vector2f(20.f, kViewH - 34.f), sf::Color(180, 255, 180));
    m_fader.draw(m_scene);
    m_scene.display();
    m_window.setView(sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(m_window.getSize().x), static_cast<float>(m_window.getSize().y))));
    m_window.clear(sf::Color::Black);
    m_post.present(m_window, m_scene, viewport(), m_time, m_assets.hasTexture("vignette") ? &m_assets.texture("vignette") : nullptr);
    m_window.display();
}

} // namespace rayman
