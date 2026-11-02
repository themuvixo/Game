#pragma once
// Explorable island hub: nodes for all 35 stages linked by winding paths, a walking hero,
// locked/unlocked states, secret branches, unlock animations and per-stage info panels.
#include <SFML/Graphics.hpp>
#include <array>
#include <string>
#include <vector>
#include "Animation.h"
#include "SaveSystem.h"

namespace rayman {

class AssetManager;
class Audio;
class Input;
class ParticleSystem;

struct MapResult {
    enum Kind { None, Enter, Back } kind = None;
    int world = 1, stage = 1;
};

class WorldMap {
public:
    void setup(const AssetManager& a);
    void enter(const SaveData& save, int revealWorld = 0, int revealStage = 0);
    MapResult update(float dt, const Input& in, Audio& audio, ParticleSystem& fx, SaveData& save);
    void draw(sf::RenderTarget& t, const AssetManager& a, const ParticleSystem& fx, float time) const;
    std::string stageName(int w, int s) const;
    int heroWorld() const { return m_cur / kStagesPerWorld + 1; }
    int heroStage() const { return m_cur % kStagesPerWorld + 1; }
private:
    struct Node { sf::Vector2f pos; std::string name; int kind = 0; };  // 0 normal, 1 challenge, 2 secret, 3 boss
    struct Edge { int a, b; bool secret; };
    struct Prop { sf::Vector2f pos; int kind; float phase; float scale; };
    static int idx(int w, int s) { return (w - 1) * kStagesPerWorld + (s - 1); }
    bool nodeOpen(int i) const;
    bool edgeOpen(const Edge& e) const;
    std::vector<int> pathTo(int target) const;
    void startTravel(int to, Audio& audio);
    sf::View camera() const;

    const SaveData* m_save = nullptr;
    std::array<Node, kWorlds * kStagesPerWorld> m_nodes;
    std::vector<Edge> m_edges;
    std::vector<Prop> m_props;
    std::vector<sf::Vector2f> m_waterfalls;
    int m_cur = 0;
    int m_travelTo = -1;
    std::vector<int> m_queue;
    float m_travelT = 0.f;
    sf::Vector2f m_heroPos;
    float m_heroDir = 1.f;
    sf::Vector2f m_cam;
    int m_revealEdge = -1;
    float m_revealT = 1.f;
    int m_revealNode = -1;
    int m_celebrateNode = -1;
    float m_celebrateT = 0.f;
    float m_t = 0.f;
    float m_lockedMsg = 0.f;
    Animator m_hero;
    sf::Vector2f m_mapSize{2560.f, 1600.f};
};

} // namespace rayman
