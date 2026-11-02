#include "SaveSystem.h"
#include <ctime>
#include <fstream>
#include <map>
#include <sstream>

namespace rayman {

namespace fs = std::filesystem;

const char* achievementName(int a) {
    static const char* n[AchCount] = {"First Adventure", "Collector", "Treasure Hunter", "Perfect Level", "Boss Slayer",
                                      "Secret Finder", "No Damage", "Speed Runner", "Complete All Worlds", "Lumling Savior",
                                      "Twice the Sky", "Explorer"};
    return (a >= 0 && a < AchCount) ? n[a] : "?";
}

const char* achievementDesc(int a) {
    static const char* d[AchCount] = {"Complete your first stage.", "Collect 1000 Tings in total.", "Collect 25 Dream Gems.",
                                      "Earn three stars on any stage.", "Defeat your first boss.", "Discover a secret stage.",
                                      "Finish a stage without taking damage.", "Finish a stage in under half its par time.",
                                      "Defeat Umbra and restore the Dream.", "Free 50 Lumlings from their cages.",
                                      "Find the Double Jump upgrade.", "Complete all five secret stages."};
    return (a >= 0 && a < AchCount) ? d[a] : "";
}

void SaveData::resetProgress() {
    SaveData fresh;
    *this = fresh;
    exists = true;
    stage(1, 1).unlocked = true;
}

float SaveData::totalCompletion() const {
    float sum = 0.f;
    for (int w = 1; w <= kWorlds; ++w)
        for (int s = 1; s <= kStagesPerWorld; ++s) sum += stage(w, s).completion;
    return sum / static_cast<float>(kWorlds * kStagesPerWorld);
}

static std::uint32_t fnv1a(const std::string& s) {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}

static std::string withChecksum(const std::string& body) { return body + "checksum=" + toStr(fnv1a(body)) + "\n"; }

static bool splitChecksum(const std::string& text, std::string& body) {
    const auto pos = text.rfind("checksum=");
    if (pos == std::string::npos) return false;
    body = text.substr(0, pos);
    const std::string sum = trim(text.substr(pos + 9));
    try {
        return static_cast<std::uint32_t>(std::stoul(sum)) == fnv1a(body);
    } catch (...) {
        return false;
    }
}

static std::map<std::string, std::string> parseKv(const std::string& body) {
    std::map<std::string, std::string> kv;
    std::istringstream ss(body);
    std::string line;
    while (std::getline(ss, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        kv[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }
    return kv;
}

template <typename T>
static void getNum(const std::map<std::string, std::string>& kv, const std::string& k, T& out) {
    const auto it = kv.find(k);
    if (it == kv.end()) return;
    std::istringstream ss(it->second);
    T v{};
    if (ss >> v) out = v;
}

SaveSystem::SaveSystem() : m_dir(userDataDir()) {}

std::string SaveSystem::readAll(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return std::string();
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string SaveSystem::serialize(const SaveData& d) {
    std::ostringstream o;
    o << "LUMOSAVE\nversion=" << SaveData::kVersion << "\n";
    o << "mapWorld=" << d.mapWorld << "\nmapStage=" << d.mapStage << "\n";
    o << "resumeWorld=" << d.resumeWorld << "\nresumeStage=" << d.resumeStage << "\nresumeCheckpoint=" << d.resumeCheckpoint << "\n";
    o << "doubleJump=" << (d.hasDoubleJump ? 1 : 0) << "\nachievements=" << d.achievements << "\nenemiesSeen=" << d.enemiesSeen << "\n";
    o << "gameCompleted=" << (d.gameCompleted ? 1 : 0) << "\n";
    for (int w = 1; w <= kWorlds; ++w) o << "boss" << w << "=" << (d.bossDefeated[static_cast<size_t>(w - 1)] ? 1 : 0) << "\n";
    const Stats& s = d.stats;
    o << "st.tings=" << s.totalTings << "\nst.enemies=" << s.enemiesDefeated << "\nst.deaths=" << s.deaths << "\nst.jumps=" << s.jumps
      << "\nst.lumlings=" << s.lumlingsFreed << "\nst.gems=" << s.gemsCollected << "\nst.relics=" << s.relicsCollected
      << "\nst.secrets=" << s.secretsFound << "\nst.bosses=" << s.bossesDefeated << "\nst.levels=" << s.levelsCompleted
      << "\nst.time=" << s.playTime << "\n";
    for (int w = 1; w <= kWorlds; ++w) {
        for (int st = 1; st <= kStagesPerWorld; ++st) {
            const StageRecord& r = d.stage(w, st);
            o << "stage" << w << "_" << st << "=" << (r.unlocked ? 1 : 0) << ' ' << (r.completed ? 1 : 0) << ' ' << r.bestScore << ' '
              << r.bestTings << ' ' << r.tingsTotal << ' ' << r.bestCages << ' ' << r.cagesTotal << ' ' << r.bestGems << ' '
              << r.gemsTotal << ' ' << r.bestRelics << ' ' << r.relicsTotal << ' ' << r.secretsFound << ' ' << r.secretsTotal << ' '
              << r.stars << ' ' << r.bestTime << ' ' << (r.keyFound ? 1 : 0) << ' ' << r.completion << "\n";
        }
    }
    return withChecksum(o.str());
}

bool SaveSystem::deserialize(const std::string& text, SaveData& out) {
    std::string body;
    if (text.compare(0, 8, "LUMOSAVE") != 0) return false;
    if (!splitChecksum(text, body)) return false;
    const auto kv = parseKv(body);
    int version = 0;
    getNum(kv, "version", version);
    if (version < 1 || version > SaveData::kVersion) return false;
    SaveData d;
    d.exists = true;
    getNum(kv, "mapWorld", d.mapWorld);
    getNum(kv, "mapStage", d.mapStage);
    getNum(kv, "resumeWorld", d.resumeWorld);
    getNum(kv, "resumeStage", d.resumeStage);
    getNum(kv, "resumeCheckpoint", d.resumeCheckpoint);
    int dj = 0, gc = 0;
    getNum(kv, "doubleJump", dj);
    getNum(kv, "gameCompleted", gc);
    d.hasDoubleJump = dj != 0;
    d.gameCompleted = gc != 0;
    getNum(kv, "achievements", d.achievements);
    getNum(kv, "enemiesSeen", d.enemiesSeen);
    for (int w = 1; w <= kWorlds; ++w) {
        int b = 0;
        getNum(kv, "boss" + toStr(w), b);
        d.bossDefeated[static_cast<size_t>(w - 1)] = b != 0;
    }
    getNum(kv, "st.tings", d.stats.totalTings);
    getNum(kv, "st.enemies", d.stats.enemiesDefeated);
    getNum(kv, "st.deaths", d.stats.deaths);
    getNum(kv, "st.jumps", d.stats.jumps);
    getNum(kv, "st.lumlings", d.stats.lumlingsFreed);
    getNum(kv, "st.gems", d.stats.gemsCollected);
    getNum(kv, "st.relics", d.stats.relicsCollected);
    getNum(kv, "st.secrets", d.stats.secretsFound);
    getNum(kv, "st.bosses", d.stats.bossesDefeated);
    getNum(kv, "st.levels", d.stats.levelsCompleted);
    getNum(kv, "st.time", d.stats.playTime);
    for (int w = 1; w <= kWorlds; ++w) {
        for (int st = 1; st <= kStagesPerWorld; ++st) {
            const auto it = kv.find("stage" + toStr(w) + "_" + toStr(st));
            if (it == kv.end()) continue;
            std::istringstream ss(it->second);
            StageRecord r;
            int u = 0, c = 0, k = 0;
            ss >> u >> c >> r.bestScore >> r.bestTings >> r.tingsTotal >> r.bestCages >> r.cagesTotal >> r.bestGems >> r.gemsTotal >>
                r.bestRelics >> r.relicsTotal >> r.secretsFound >> r.secretsTotal >> r.stars >> r.bestTime >> k >> r.completion;
            if (ss.fail()) return false;
            r.unlocked = u != 0;
            r.completed = c != 0;
            r.keyFound = k != 0;
            r.stars = std::max(0, std::min(3, r.stars));
            r.completion = clampf(r.completion, 0.f, 1.f);
            d.stage(w, st) = r;
        }
    }
    d.mapWorld = std::max(1, std::min(kWorlds, d.mapWorld));
    d.mapStage = std::max(1, std::min(kStagesPerWorld, d.mapStage));
    d.stage(1, 1).unlocked = true;
    out = d;
    return true;
}

std::string SaveSystem::serializeSettings(const Settings& s) {
    std::ostringstream o;
    o << "LUMOCFG\nversion=1\n";
    o << "master=" << s.master << "\nmusic=" << s.music << "\nsfx=" << s.sfx << "\nfullscreen=" << (s.fullscreen ? 1 : 0)
      << "\nresolution=" << s.resolution << "\nvsync=" << (s.vsync ? 1 : 0) << "\nfps=" << (s.showFps ? 1 : 0) << "\n";
    for (int i = 0; i < kActionCount; ++i) o << "key" << i << "=" << s.keys[static_cast<size_t>(i)] << "\n";
    return withChecksum(o.str());
}

bool SaveSystem::deserializeSettings(const std::string& text, Settings& out) {
    std::string body;
    if (text.compare(0, 7, "LUMOCFG") != 0) return false;
    if (!splitChecksum(text, body)) return false;
    const auto kv = parseKv(body);
    Settings s;
    int fs = 0, vs = 1, fps = 0;
    getNum(kv, "master", s.master);
    getNum(kv, "music", s.music);
    getNum(kv, "sfx", s.sfx);
    getNum(kv, "fullscreen", fs);
    getNum(kv, "resolution", s.resolution);
    getNum(kv, "vsync", vs);
    getNum(kv, "fps", fps);
    s.fullscreen = fs != 0;
    s.vsync = vs != 0;
    s.showFps = fps != 0;
    s.master = clampf(s.master, 0.f, 1.f);
    s.music = clampf(s.music, 0.f, 1.f);
    s.sfx = clampf(s.sfx, 0.f, 1.f);
    if (s.resolution < 0 || s.resolution >= kResolutionCount) s.resolution = 0;
    for (int i = 0; i < kActionCount; ++i) {
        int k = s.keys[static_cast<size_t>(i)];
        getNum(kv, "key" + toStr(i), k);
        if (k >= 0 && k < sf::Keyboard::KeyCount) s.keys[static_cast<size_t>(i)] = k;
    }
    out = s;
    return true;
}

static bool validateSaveText(const std::string& t);
static bool validateSettingsText(const std::string& t);

bool SaveSystem::atomicWrite(const fs::path& target, const std::string& content, bool (*validate)(const std::string&)) {
    std::error_code ec;
    fs::create_directories(target.parent_path(), ec);
    const fs::path tmp = target.string() + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) { m_msg = "Could not open temp file for writing"; return false; }
        f << content;
        f.flush();
        if (!f) { m_msg = "Write failed"; return false; }
    }
    // Validate what actually landed on disk before replacing anything.
    if (!validate(readAll(tmp))) {
        m_msg = "Validation of temp save failed; previous save kept";
        return false;
    }
    if (fs::exists(target, ec)) {
        const fs::path bak = target.string() + ".bak";
        fs::copy_file(target, bak, fs::copy_options::overwrite_existing, ec);
    }
    fs::rename(tmp, target, ec);  // MSVC STL: MoveFileExW with MOVEFILE_REPLACE_EXISTING
    if (ec) {
        // Fallback for filesystems that refuse replace-on-rename: the .bak copy above protects the old data.
        std::error_code ec2;
        fs::copy_file(tmp, target, fs::copy_options::overwrite_existing, ec2);
        if (ec2) { m_msg = "Could not replace save file"; return false; }
        fs::remove(tmp, ec2);
    }
    m_msg = "Saved";
    return true;
}

bool SaveSystem::saveFileExists() const {
    std::error_code ec;
    return fs::exists(m_dir / "save.dat", ec) || fs::exists(m_dir / "save.dat.bak", ec);
}

bool SaveSystem::load(SaveData& out) {
    const fs::path main = m_dir / "save.dat";
    const fs::path bak = m_dir / "save.dat.bak";
    std::error_code ec;
    if (fs::exists(main, ec)) {
        if (deserialize(readAll(main), out)) { m_msg = "Loaded"; return true; }
        // Keep the corrupted file for inspection; never delete it.
        const fs::path keep = m_dir / ("save.corrupt." + toStr(static_cast<long long>(std::time(nullptr))));
        fs::copy_file(main, keep, fs::copy_options::skip_existing, ec);
        logLine("[save] save.dat failed validation; preserved as " + keep.filename().string());
    }
    if (fs::exists(bak, ec) && deserialize(readAll(bak), out)) {
        m_msg = "Recovered from backup";
        logLine("[save] recovered from save.dat.bak");
        return true;
    }
    out = SaveData();
    out.stage(1, 1).unlocked = true;
    out.exists = false;
    m_msg = "New save";
    return false;
}

bool SaveSystem::save(const SaveData& data) { return atomicWrite(m_dir / "save.dat", serialize(data), &validateSaveText); }

bool SaveSystem::loadSettings(Settings& out) {
    const fs::path p = m_dir / "settings.cfg";
    std::error_code ec;
    if (fs::exists(p, ec) && deserializeSettings(readAll(p), out)) return true;
    const fs::path bak = m_dir / "settings.cfg.bak";
    if (fs::exists(bak, ec) && deserializeSettings(readAll(bak), out)) return true;
    out = Settings();
    return false;
}

bool SaveSystem::saveSettings(const Settings& s) { return atomicWrite(m_dir / "settings.cfg", serializeSettings(s), &validateSettingsText); }

static bool validateSaveText(const std::string& t) { SaveData d; return SaveSystem::deserialize(t, d); }
static bool validateSettingsText(const std::string& t) { Settings s; return SaveSystem::deserializeSettings(t, s); }

} // namespace rayman
