#pragma once

#include "core/math.h"
#include "core/object.h"
#include "engine/collision_hash.h"
#include "engine/decal.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace dx {
    class ULevel;
    class UModel;

    struct CheckResult {
        UObject* actor = nullptr;
        Vector location;
        Vector normal;
        float time = 1.0f;
        int32_t item = -1;
        UObject* material = nullptr;
    };

    constexpr uint32_t TRACE_Pawns = 0x01;
    constexpr uint32_t TRACE_Movers = 0x02;
    constexpr uint32_t TRACE_Level = 0x04;
    constexpr uint32_t TRACE_ZoneChanging = 0x08;
    constexpr uint32_t TRACE_Others = 0x10;
    constexpr uint32_t TRACE_OnlyProjActor = 0x20;
    constexpr uint32_t TRACE_Blocking = 0x40;
    constexpr uint32_t TRACE_LevelGeometry = 0x80;
    constexpr uint32_t TRACE_ShadowCast = 0x100;
    constexpr uint32_t TRACE_StopAtFirstHit = 0x200;
    constexpr uint32_t TRACE_World = TRACE_Movers | TRACE_Level | TRACE_LevelGeometry;
    constexpr uint32_t TRACE_AllColliding = TRACE_Pawns | TRACE_Movers | TRACE_Level | TRACE_Others | TRACE_LevelGeometry;

    struct TravelValue {
        UProperty* property = nullptr;
        int32_t index = 0;
        std::vector<uint8_t> data;
    };

    struct TravelActor {
        UClass* cls = nullptr;
        UObject* original = nullptr;
        bool isPlayer = false;
        std::vector<TravelValue> values;
    };

    class World {
    public:
        World();
        ~World();
        World(const World&) = delete;
        World& operator=(const World&) = delete;

        static World* current;

        bool loadMap(const std::string& url);
        std::vector<TravelActor> collectTravel();
        std::string travelString(UObject* player) const;
        std::vector<std::pair<std::string, std::string>> arrivalTravelInfo;
        bool replaceTravelInfo = false;
        static void releaseTravel(std::vector<TravelActor>& travel);
        std::vector<UObject*> applyTravel(UObject* player);
        void tick(float deltaSeconds);
        bool calcView(Vector& location, Rotator& rotation, float& fov);
        UObject* player() const;

        UObject* spawn(UClass* cls, UObject* owner, Name tag, const Vector& location, const Rotator& rotation, UObject* templateActor, bool noCollisionFail, UObject* instigator = nullptr);
        bool destroy(UObject* actor);
        bool inStasis(UObject* actor);

        bool findSpot(const Vector& extent, Vector& location, bool checkActors, bool assumeFit);
        bool pointCheck(CheckResult& hit, const Vector& location, const Vector& extent, UObject* source, bool checkActors);
        bool lineCheck(CheckResult& hit, const Vector& start, const Vector& end, const Vector& extent, UObject* source, uint32_t flags);
        std::vector<CheckResult> multiLineCheck(const Vector& start, const Vector& end, const Vector& extent, UObject* source, uint32_t flags, uint8_t nodeFlags = 0);
        bool bspLineCheck(CheckResult& hit, const Vector& start, const Vector& end, const Vector& extent, uint8_t nodeFlags = 0);
        bool fastLineCheck(const Vector& end, const Vector& start);
        bool encroaches(UObject* actor, const Vector& location);
        bool moveActor(UObject* actor, const Vector& delta, const Rotator& rotation, CheckResult& hit, bool test, bool ignorePawns, bool ignoreBases, bool noFail);
        bool farMoveActor(UObject* actor, const Vector& location, bool test, bool noCheck);
        void setLocationZone(UObject* actor, const Vector& location, bool test = false);
        UObject* zoneAt(const Vector& location, int32_t* leaf, uint8_t* zoneNumber);
        int32_t pointLeaf(const Vector& location);
        std::shared_ptr<const std::vector<UObject*>> liveActors();

        bool tickActor(UObject* actor, float deltaSeconds);
        void tickPawnSenses(UObject* pawn, float deltaSeconds);
        void aiEvents();
        void cleanupDestroyed();

        ULevel* level = nullptr;
        UObject* levelInfo = nullptr;
        UObject* gameInfo = nullptr;
        UObject* localPlayer = nullptr;
        UObject* viewport = nullptr;
        std::vector<UObject*> actors;
        CollisionHash collisionHash;
        DecalRegistry decals;
        uint64_t actorsVersion = 0;
        float timeSeconds = 0;
        double realTime = 0;
        uint32_t frame = 0;
        std::string url;
        std::string travelUrl;
        int32_t travelType = -1;
        bool travelPending = false;
        bool travelItems = false;
        std::vector<TravelActor> incomingTravel;
        bool paused = false;
        bool pendingCleanup = false;
        std::vector<UObject*> destroyedActors;
        uint32_t tickParity = 0;
        size_t firstDynamicActor = 0;
        bool inTick = false;
        std::vector<UObject*> newlySpawned;
        size_t runtimeStart = 0;
        std::function<void(UObject*, float)> inputHook;

    private:
        std::shared_ptr<const std::vector<UObject*>> liveActorsCache;
        uint64_t liveActorsVersion = 0;
        uint64_t liveActorsDestroys = 0;
    };

    namespace engine {
        void initialize(const std::string& gameRoot);
        void initializeCore(const std::string& gameRoot);
        void registerEngineNatives();
        void registerDeusExNatives();
    }
}
