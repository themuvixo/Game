#pragma once
// Versioned, checksummed text save files with atomic replace and backup recovery.
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include "Input.h"
#include "Utils.h"

namespace rayman {

struct StageRecord {
    bool unlocked = false;
    bool completed = false;
    int bestScore = 0;
    int bestTings = 0, tingsTotal = 0;
    int bestCages = 0, cagesTotal = 0;
    int bestGems = 0, gemsTotal = 0;
    int bestRelics = 0, relicsTotal = 0;
    int secretsFound = 0, secretsTotal = 0;
    int stars = 0;
    float bestTime = 0.f;
    bool keyFound = false;
    float completion = 0.f;  // 0..1
};

enum Achievement : int {
    AchFirstAdventure = 0, AchCollector, AchTreasureHunter, AchPerfectLevel, AchBossSlayer, AchSecretFinder,
    AchNoDamage, AchSpeedRunner, AchAllWorlds, AchLumlingSavior, AchDoubleJump, AchExplorer, AchCount
};

const char* achievementName(int a);
const char* achievementDesc(int a);

struct Stats {
    long long totalTings = 0;
    int enemiesDefeated = 0;
    int deaths = 0;
    int jumps = 0;
    int lumlingsFreed = 0;
    int gemsCollected = 0;
    int relicsCollected = 0;
    int secretsFound = 0;
    int bossesDefeated = 0;
    int levelsCompleted = 0;
    float playTime = 0.f;
};

struct SaveData {
    static constexpr int kVersion = 3;
    bool exists = false;
    int mapWorld = 1, mapStage = 1;          // hero position on the world map
    int resumeWorld = 0, resumeStage = 0;    // mid-level checkpoint resume (0 = none)
    int resumeCheckpoint = -1;
    std::array<std::array<StageRecord, kStagesPerWorld>, kWorlds> stages{};
    std::array<bool, kWorlds> bossDefeated{};
    bool hasDoubleJump = false;
    std::uint32_t achievements = 0;
    std::uint32_t enemiesSeen = 0;  // bit (world-1)*6 + type
    Stats stats;
    bool gameCompleted = false;

    StageRecord& stage(int w, int s) { return stages[static_cast<size_t>(w - 1)][static_cast<size_t>(s - 1)]; }
    const StageRecord& stage(int w, int s) const { return stages[static_cast<size_t>(w - 1)][static_cast<size_t>(s - 1)]; }
    bool hasAchievement(int a) const { return (achievements >> a) & 1u; }
    void resetProgress();
    float totalCompletion() const;
};

struct Settings {
    float master = 0.8f, music = 0.7f, sfx = 0.8f;
    bool fullscreen = false;
    int resolution = 0;
    bool vsync = true;
    bool showFps = false;
    std::array<int, kActionCount> keys = defaultBindings();
};

struct Resolution { unsigned w, h; };
constexpr int kResolutionCount = 5;
inline Resolution resolutionAt(int i) {
    static const Resolution r[kResolutionCount] = {{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {960, 540}};
    return r[(i >= 0 && i < kResolutionCount) ? i : 0];
}

class SaveSystem {
public:
    SaveSystem();
    bool load(SaveData& out);              // returns false when defaults were used
    bool save(const SaveData& data);
    bool loadSettings(Settings& out);
    bool saveSettings(const Settings& s);
    bool saveFileExists() const;
    const std::string& lastMessage() const { return m_msg; }
    std::filesystem::path dir() const { return m_dir; }
    static std::string serialize(const SaveData& d);
    static bool deserialize(const std::string& text, SaveData& out);
    static std::string serializeSettings(const Settings& s);
    static bool deserializeSettings(const std::string& text, Settings& out);
private:
    bool atomicWrite(const std::filesystem::path& target, const std::string& content, bool (*validate)(const std::string&));
    static std::string readAll(const std::filesystem::path& p);
    std::filesystem::path m_dir;
    std::string m_msg;
};

} // namespace rayman
