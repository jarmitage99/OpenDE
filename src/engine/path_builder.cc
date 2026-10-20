#include "engine/path_builder.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/props_engine.h"
#include "engine/world.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace dx {
    namespace paths {
        namespace {
            constexpr int32_t ReachSpecial = 32;
            constexpr int32_t ReachSwim = 4;
            constexpr uint32_t TraceMoversLevel = TRACE_Movers | TRACE_Level;

            UClass* engineClass(const char* name)
            {
                return objects::findClass(std::string("Engine.") + name);
            }

            bool isA(UObject* a, const char* className)
            {
                UClass* cls = engineClass(className);
                return a && cls && a->isA(cls);
            }

            bool lowerEquals(const std::string& a, const std::string& b)
            {
                if (a.size() != b.size()) {
                    return false;
                }
                for (size_t i = 0; i < a.size(); i++) {
                    if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
                        return false;
                    }
                }
                return true;
            }

            template <typename T>
            T& property(UObject* a, const char* name)
            {
                return *reinterpret_cast<T*>(a->propertyAddress(Name(name)));
            }

            void twoWallAdjust(const Vector& desired, Vector& delta, const Vector& hitNormal, const Vector& oldNormal, float hitTime)
            {
                if (oldNormal.dot(hitNormal) <= 0) {
                    Vector direction = hitNormal.cross(oldNormal).normal();
                    delta = direction * (delta.dot(direction) * (1.0f - hitTime));
                    if (desired.dot(delta) < 0) {
                        delta = delta * -1.0f;
                    }
                } else {
                    delta = (delta - hitNormal * delta.dot(hitNormal)) * (1.0f - hitTime);
                    if (delta.dot(desired) <= 0) {
                        delta = {};
                    }
                }
            }

            constexpr uint32_t MarkerVisible = 0x01;
            constexpr uint32_t MarkerMarked = 0x02;
            constexpr uint32_t MarkerBeacon = 0x08;
            constexpr uint32_t MarkerLeftTurn = 0x10;
            constexpr uint32_t MarkerPermanent = 0x20;
            constexpr uint32_t MarkerStair = 0x40;
            constexpr uint32_t MarkerRoutable = 0x80;
            constexpr int MaxMarkers = 3000;

            struct PathMarker {
                Vector location;
                Vector direction;
                uint32_t flags = 0;
                float radius = 0.0f;
                float budget = 0.0f;
                float weight = 0.0f;
            };

            class Builder {
            public:
                explicit Builder(ULevel* target)
                    : level(target)
                    , previous(World::current)
                {
                    World::current = &world;
                    world.level = level;
                    world.levelInfo = level->actors.empty() ? nullptr : level->actors[0];
                    world.actors = level->actors;
                    world.collisionHash.clear();
                    world.collisionHash.sync(world.actors);
                }

                ~Builder()
                {
                    World::current = previous;
                }

                Builder(const Builder&) = delete;
                Builder& operator=(const Builder&) = delete;

                UObject* levelInfo()
                {
                    return world.levelInfo;
                }

                UObject* spawn(UClass* cls, const Vector& location)
                {
                    if (!cls) {
                        return nullptr;
                    }
                    UObject* a = objects::newObject(cls, level->outermost(), Name(), RF_Transactional | 0x00070000);
                    if (!a) {
                        return nullptr;
                    }
                    a->loaded = true;
                    actor::Tag(a) = cls->name;
                    actor::Level(a) = levelInfo();
                    actor::XLevel(a) = level;
                    actor::Location(a) = location;
                    actor::OldLocation(a) = location;
                    region(a) = { levelInfo(), -1, 0 };
                    if (isPawn(a)) {
                        *reinterpret_cast<PointRegion*>(pawn::FootRegion(a)) = { levelInfo(), -1, 0 };
                        *reinterpret_cast<PointRegion*>(pawn::HeadRegion(a)) = { levelInfo(), -1, 0 };
                    }
                    level->actors.push_back(a);
                    world.actors.push_back(a);
                    world.actorsVersion++;
                    world.collisionHash.update(a);
                    world.setLocationZone(a, location);
                    return a;
                }

                void destroy(UObject* a)
                {
                    if (!a) {
                        return;
                    }
                    world.collisionHash.remove(a);
                    level->actors.erase(std::remove(level->actors.begin(), level->actors.end(), a), level->actors.end());
                    world.actors.erase(std::remove(world.actors.begin(), world.actors.end(), a), world.actors.end());
                    world.actorsVersion++;
                    a->destroyed = true;
                }

                void setCollisionSize(UObject* a, float radius, float height)
                {
                    actor::CollisionRadius(a) = radius;
                    actor::CollisionHeight(a) = height;
                    world.collisionHash.update(a);
                }

                void getScout()
                {
                    scout = nullptr;
                    for (UObject* a : level->actors) {
                        if (a && !a->destroyed && isA(a, "Scout")) {
                            scout = a;
                        }
                    }
                    if (!scout) {
                        scout = spawn(engineClass("Scout"), {});
                    }
                    if (!scout) {
                        return;
                    }
                    actor::bCollideActors(scout) = 1;
                    actor::bBlockActors(scout) = 1;
                    actor::bBlockPlayers(scout) = 1;
                    actor::bCollideWorld(scout) = 1;
                    world.collisionHash.update(scout);
                    world.setLocationZone(scout, actor::Location(scout));
                }

                bool findScoutStart(const Vector& start)
                {
                    if (!world.farMoveActor(scout, start, false, false)) {
                        return false;
                    }
                    CheckResult hit;
                    Vector down { 0, 0, -50 };
                    hit.normal = {};
                    hit.time = 1.0f;
                    int iterations = 0;
                    while (static_cast<double>(hit.normal.z) < 0.7) {
                        world.moveActor(scout, down, actor::Rotation(scout), hit, true, true, false, false);
                        if (hit.time < 1.0f && static_cast<double>(hit.normal.z) < 0.7) {
                            Vector oldNormal = hit.normal;
                            Vector delta = (down - hit.normal * down.dot(hit.normal)) * (1.0f - hit.time);
                            if (delta.dot(down) >= 0.0f) {
                                world.moveActor(scout, delta, actor::Rotation(scout), hit, true, true, false, false);
                                if (hit.time < 1.0f && static_cast<double>(hit.normal.z) < 0.7) {
                                    Vector downDirection = down.normal();
                                    twoWallAdjust(downDirection, delta, hit.normal, oldNormal, hit.time);
                                    world.moveActor(scout, delta, actor::Rotation(scout), hit, true, true, false, false);
                                }
                            }
                        }
                        iterations++;
                        if (iterations >= 50) {
                            return false;
                        }
                    }
                    return true;
                }

                bool traceClear(const Vector& end, const Vector& start)
                {
                    CheckResult hit;
                    return !world.lineCheck(hit, start, end, {}, scout, TraceMoversLevel);
                }

                int findBestReachable(ReachSpec& spec, const Vector& start, const Vector& destination, float startHeight)
                {
                    setCollisionSize(scout, 12.0f, 10.0f);
                    float stepsize = 115.0f - actor::CollisionRadius(scout);
                    float bestHeight = actor::CollisionHeight(scout);
                    float bestRadius = actor::CollisionRadius(scout);
                    bool stillTrying = true;
                    int result = 0;
                    CheckResult hit;
                    Vector begin = { start.x, start.y, start.z - 79.0f };
                    bool blocked = world.lineCheck(hit, start, begin, {}, scout, TraceMoversLevel);
                    if (blocked && hit.time != 1.0f) {
                        begin = hit.location;
                    } else {
                        begin = { start.x, start.y, start.z - startHeight };
                    }
                    Vector dest = destination;
                    CheckResult through;
                    if (world.lineCheck(through, start, destination, {}, scout, TraceMoversLevel) && through.time != 1.0f) {
                        return 0;
                    }
                    while (stillTrying) {
                        int success = world.farMoveActor(scout, { begin.x, begin.y, begin.z + actor::CollisionHeight(scout) }, false, false) ? 1 : 0;
                        if (success) {
                            success = pawnPointReachable(scout, dest, true);
                        }
                        if (success) {
                            spec.reachFlags = success;
                            result = 1;
                            bestRadius = actor::CollisionRadius(scout);
                            setCollisionSize(scout, actor::CollisionRadius(scout) + stepsize, 10.0f);
                            stepsize = static_cast<float>(static_cast<double>(stepsize) * 0.5);
                            if (actor::CollisionRadius(scout) > 115.0f) {
                                stillTrying = false;
                            }
                        } else {
                            setCollisionSize(scout, actor::CollisionRadius(scout) - stepsize, 10.0f);
                            stepsize = static_cast<float>(static_cast<double>(stepsize) * 0.5);
                            if (actor::CollisionRadius(scout) < 12.0f) {
                                stillTrying = false;
                            }
                        }
                        if (stepsize < 1.0f) {
                            stillTrying = false;
                        }
                    }
                    if (result) {
                        setCollisionSize(scout, 12.0f, actor::CollisionHeight(scout));
                        stepsize = 79.0f - actor::CollisionHeight(scout);
                        stillTrying = true;
                    }
                    while (stillTrying) {
                        int success = world.farMoveActor(scout, { begin.x, begin.y, begin.z + actor::CollisionHeight(scout) }, false, false) ? 1 : 0;
                        if (success) {
                            success = pawnPointReachable(scout, dest, true);
                        }
                        if (success) {
                            spec.reachFlags = success;
                            bestHeight = actor::CollisionHeight(scout);
                            setCollisionSize(scout, 12.0f, actor::CollisionHeight(scout) + stepsize);
                            stepsize = static_cast<float>(static_cast<double>(stepsize) * 0.5);
                            if (actor::CollisionHeight(scout) > 79.0f) {
                                stillTrying = false;
                            }
                        } else {
                            setCollisionSize(scout, 12.0f, actor::CollisionHeight(scout) - stepsize);
                            stepsize = static_cast<float>(static_cast<double>(stepsize) * 0.5);
                            if (actor::CollisionHeight(scout) < 10.0f) {
                                stillTrying = false;
                            }
                        }
                        if (stepsize < 1.0f) {
                            stillTrying = false;
                        }
                    }
                    if (result) {
                        spec.collisionRadius = static_cast<int32_t>(bestRadius);
                        spec.collisionHeight = static_cast<int32_t>(bestHeight);
                        Vector path = actor::Location(spec.end) - actor::Location(spec.start);
                        spec.distance = static_cast<int32_t>(std::sqrt(static_cast<double>(path.sizeSquared())));
                        if (spec.reachFlags & ReachSwim) {
                            spec.distance *= 2;
                        }
                    }
                    return result;
                }

                int defineFor(ReachSpec& spec, UObject* begin, UObject* dest)
                {
                    spec.start = begin;
                    spec.end = dest;
                    pawn::bCanFly(scout) = 0;
                    pawn::bCanJump(scout) = 1;
                    pawn::bCanWalk(scout) = 1;
                    pawn::bCanSwim(scout) = 1;
                    actor::Physics(scout) = 1;
                    pawn::JumpZ(scout) = 120.0f;
                    pawn::GroundSpeed(scout) = 120.0f;
                    pawn::MaxStepHeight(scout) = 25.0f;
                    pawn::BaseEyeHeight(scout) = 0.0f;
                    return findBestReachable(spec, actor::Location(begin), actor::Location(dest), actor::CollisionHeight(begin));
                }

                int insertReachSpec(UObject* node, int32_t& (*slot)(UObject*, int), const ReachSpec& spec)
                {
                    int i = 0;
                    while (i < 16 && slot(node, i) != -1 && level->reachSpecs[static_cast<size_t>(slot(node, i))].distance > spec.distance) {
                        i++;
                    }
                    int pos = i;
                    if (slot(node, 15) == -1) {
                        int32_t carried = slot(node, i);
                        if (carried == -1) {
                            return pos;
                        }
                        while (i < 15) {
                            int32_t next = slot(node, i + 1);
                            slot(node, i + 1) = carried;
                            if (next == -1) {
                                i = 15;
                            }
                            i++;
                            carried = next;
                        }
                        return pos;
                    }
                    if (pos == 0) {
                        return -1;
                    }
                    int32_t carried = slot(node, pos - 1);
                    pos--;
                    for (int k = pos; k > 0; k--) {
                        int32_t previousValue = slot(node, k - 1);
                        slot(node, k - 1) = carried;
                        carried = previousValue;
                    }
                    return pos;
                }

                void addSpec(UObject* from, UObject* to, const ReachSpec& spec)
                {
                    int index = insertReachSpec(from, navigationpoint::Paths, spec);
                    if (index == -1) {
                        return;
                    }
                    int32_t specIndex = static_cast<int32_t>(level->reachSpecs.size());
                    level->reachSpecs.push_back(spec);
                    navigationpoint::Paths(from, index) = specIndex;
                    int upstream = insertReachSpec(to, navigationpoint::upstreamPaths, spec);
                    if (upstream != -1) {
                        navigationpoint::upstreamPaths(to, upstream) = specIndex;
                    }
                }

                ReachSpec specialSpec(UObject* from, UObject* to, int32_t size, int32_t distance)
                {
                    ReachSpec spec;
                    spec.collisionRadius = size;
                    spec.collisionHeight = size;
                    spec.reachFlags = ReachSpecial;
                    spec.start = from;
                    spec.end = to;
                    spec.distance = distance;
                    return spec;
                }

                void addReachSpecs(UObject* node)
                {
                    if (isA(node, "LiftCenter")) {
                        Name liftTag = property<Name>(node, "LiftTag");
                        for (size_t i = 0; i < level->actors.size(); i++) {
                            UObject* a = level->actors[i];
                            if (!a || a->destroyed || !isA(a, "LiftExit") || property<Name>(a, "LiftTag") != liftTag) {
                                continue;
                            }
                            addSpec(node, a, specialSpec(node, a, 60, 500));
                            addSpec(a, node, specialSpec(a, node, 60, 500));
                        }
                        return;
                    }
                    bool teleporter = isA(node, "Teleporter");
                    if (teleporter || isA(node, "WarpZoneMarker")) {
                        for (size_t i = 0; i < level->actors.size(); i++) {
                            UObject* a = level->actors[i];
                            bool match = false;
                            if (teleporter) {
                                match = a && !a->destroyed && isA(a, "Teleporter") && a != node && lowerEquals(teleporter::URL(node), actor::Tag(a).str());
                            } else if (a && !a->destroyed && isA(a, "WarpZoneMarker") && a != node) {
                                UObject* zone = property<UObject*>(node, "markedWarpZone");
                                UObject* other = property<UObject*>(a, "markedWarpZone");
                                match = zone && other && lowerEquals(warpzoneinfo::OtherSideURL(zone), warpzoneinfo::ThisTag(other).str());
                            }
                            if (match) {
                                addSpec(node, a, specialSpec(node, a, 150, 100));
                                break;
                            }
                        }
                    }
                    for (size_t i = 0; i < level->actors.size(); i++) {
                        UObject* a = level->actors[i];
                        if (!a || a->destroyed || !isA(a, "NavigationPoint") || isA(a, "LiftCenter") || a == node) {
                            continue;
                        }
                        Vector between = actor::Location(node) - actor::Location(a);
                        bool close = between.sizeSquared() < 1000000.0f;
                        bool direction = true;
                        if (navigationpoint::bOneWayPath(node)) {
                            Rotator rotation = actor::Rotation(node);
                            Coords axes = rotationAxes(rotation);
                            direction = (actor::Location(a) - actor::Location(node)).dot(axes.xAxis) > 0.0f;
                        }
                        if (!close || !direction) {
                            continue;
                        }
                        ReachSpec spec;
                        if (defineFor(spec, node, a)) {
                            addSpec(node, a, spec);
                        }
                    }
                }

                int32_t specFor(UObject* start, UObject* end)
                {
                    for (int i = 0; i < 16 && navigationpoint::Paths(start, i) != -1; i++) {
                        const ReachSpec& spec = level->reachSpecs[static_cast<size_t>(navigationpoint::Paths(start, i))];
                        if (spec.end == end) {
                            return navigationpoint::Paths(start, i);
                        }
                    }
                    return -1;
                }

                static bool easierOrEqual(const ReachSpec& a, const ReachSpec& b)
                {
                    return a.collisionRadius >= b.collisionRadius && a.collisionHeight >= b.collisionHeight && (a.reachFlags | b.reachFlags) == b.reachFlags;
                }

                int prune(UObject* node)
                {
                    int pruned = 0;
                    for (int i = 0; i < 16 && navigationpoint::upstreamPaths(node, i) != -1; i++) {
                        ReachSpec part1 = level->reachSpecs[static_cast<size_t>(navigationpoint::upstreamPaths(node, i))];
                        for (int n = 0; n < 16 && navigationpoint::Paths(node, n) != -1; n++) {
                            ReachSpec part2 = level->reachSpecs[static_cast<size_t>(navigationpoint::Paths(node, n))];
                            int32_t straightIndex = specFor(part1.start, part2.end);
                            if (straightIndex == -1) {
                                continue;
                            }
                            ReachSpec straight = level->reachSpecs[static_cast<size_t>(straightIndex)];
                            ReachSpec alternate;
                            alternate.collisionRadius = std::min(part1.collisionRadius, part2.collisionRadius);
                            alternate.collisionHeight = std::min(part1.collisionHeight, part2.collisionHeight);
                            alternate.reachFlags = part1.reachFlags | part2.reachFlags;
                            alternate.distance = part1.distance + part2.distance;
                            bool shortEnough = static_cast<double>(alternate.distance) <= static_cast<double>(straight.distance) * 1.2;
                            bool botOnly = straight.collisionRadius < 24;
                            bool monster = alternate.collisionRadius >= 52 && alternate.collisionHeight >= 48 && !(alternate.reachFlags & 2);
                            if (!shortEnough || !(easierOrEqual(alternate, straight) || botOnly || monster)) {
                                continue;
                            }
                            pruned++;
                            UObject* startNode = straight.start;
                            UObject* endNode = straight.end;
                            int j = 0;
                            while (j < 15 && navigationpoint::Paths(startNode, j) != straightIndex) {
                                j++;
                            }
                            int32_t fill = -1;
                            if (navigationpoint::Paths(startNode, j) == straightIndex) {
                                while (j < 15 && navigationpoint::Paths(startNode, j) != -1) {
                                    navigationpoint::Paths(startNode, j) = navigationpoint::Paths(startNode, j + 1);
                                    j++;
                                }
                                navigationpoint::Paths(startNode, 15) = -1;
                            }
                            j = 0;
                            while (j < 15 && navigationpoint::PrunedPaths(startNode, j) != fill) {
                                j++;
                            }
                            navigationpoint::PrunedPaths(startNode, j) = straightIndex;
                            level->reachSpecs[static_cast<size_t>(straightIndex)].pruned = 1;
                            j = 0;
                            while (j < 15 && navigationpoint::upstreamPaths(endNode, j) != straightIndex) {
                                j++;
                            }
                            if (navigationpoint::upstreamPaths(endNode, j) == straightIndex) {
                                while (j < 15 && navigationpoint::upstreamPaths(endNode, j) != -1) {
                                    navigationpoint::upstreamPaths(endNode, j) = navigationpoint::upstreamPaths(endNode, j + 1);
                                    j++;
                                }
                                navigationpoint::upstreamPaths(endNode, 15) = -1;
                            }
                        }
                    }
                    return pruned;
                }

                void addVisNoReach(UObject* node)
                {
                    if (isA(node, "LiftCenter")) {
                        return;
                    }
                    setCollisionSize(scout, 22.0f, 51.0f);
                    world.farMoveActor(scout, actor::Location(node), true, false);
                    pawn::MoveTarget(scout) = node;
                    pawn::bCanDoSpecial(scout) = 1;
                    int count = 0;
                    for (UObject* nav = levelinfo::NavigationPointList(levelInfo()); nav;) {
                        float dist2 = (actor::Location(node) - actor::Location(nav)).sizeSquared();
                        if (!isA(nav, "LiftCenter") && nav != node && dist2 < 4000000.0f && count < 16) {
                            CheckResult hit;
                            bool blocked = world.lineCheck(hit, actor::Location(node), actor::Location(nav), {}, scout, TraceMoversLevel);
                            if (!blocked || !hit.actor) {
                                UObject* bestPath = nullptr;
                                float pathDistance = 200000000.0f;
                                if (pawnFindPathToward(scout, nav, false, bestPath, true) && bestPath) {
                                    pathDistance = static_cast<float>(navigationpoint::visitedWeight(bestPath));
                                }
                                if (pathDistance != 10000000.0f && dist2 * 4.0f < pathDistance * pathDistance) {
                                    navigationpoint::VisNoReachPaths(node, count) = nav;
                                    count++;
                                }
                            }
                        }
                        nav = navigationpoint::nextNavigationPoint(nav);
                    }
                }

                void definePaths()
                {
                    getScout();
                    if (!scout) {
                        return;
                    }
                    levelinfo::NavigationPointList(levelInfo()) = nullptr;
                    std::vector<UObject*> actors = level->actors;
                    for (UObject* a : actors) {
                        if (!a || a->destroyed || !isA(a, "WarpZoneInfo")) {
                            continue;
                        }
                        if (!findScoutStart(actor::Location(a)) || region(scout).zone != region(a).zone) {
                            setCollisionSize(scout, 20.0f, actor::CollisionHeight(scout));
                            if (!findScoutStart(actor::Location(a)) || region(scout).zone != region(a).zone) {
                                world.farMoveActor(scout, actor::Location(a), true, true);
                            }
                            setCollisionSize(scout, 12.0f, actor::CollisionHeight(scout));
                        }
                        UObject* marker = spawn(engineClass("WarpZoneMarker"), actor::Location(scout));
                        if (marker) {
                            property<UObject*>(marker, "markedWarpZone") = a;
                        }
                    }
                    for (size_t i = 0; i < level->actors.size(); i++) {
                        UObject* a = level->actors[i];
                        if (!a || a->destroyed || !isA(a, "NavigationPoint")) {
                            continue;
                        }
                        navigationpoint::nextNavigationPoint(a) = levelinfo::NavigationPointList(levelInfo());
                        levelinfo::NavigationPointList(levelInfo()) = a;
                        addReachSpecs(a);
                    }
                    for (UObject* nav = levelinfo::NavigationPointList(levelInfo()); nav; nav = navigationpoint::nextNavigationPoint(nav)) {
                        prune(nav);
                    }
                    for (UObject* nav = levelinfo::NavigationPointList(levelInfo()); nav; nav = navigationpoint::nextNavigationPoint(nav)) {
                        addVisNoReach(nav);
                    }
                    destroy(scout);
                    scout = nullptr;
                }

                void undefinePaths()
                {
                    level->reachSpecs.clear();
                    levelinfo::NavigationPointList(levelInfo()) = nullptr;
                    std::vector<UObject*> actors = level->actors;
                    for (UObject* a : actors) {
                        if (!a || a->destroyed || !isA(a, "NavigationPoint")) {
                            continue;
                        }
                        if (isA(a, "WarpZoneMarker") || isA(a, "TriggerMarker") || isA(a, "InventorySpot") || isA(a, "ButtonMarker")) {
                            if (isA(a, "InventorySpot")) {
                                UObject* item = property<UObject*>(a, "markedItem");
                                if (item) {
                                    inventory::myMarker(item) = nullptr;
                                }
                            }
                            destroy(a);
                            continue;
                        }
                        navigationpoint::nextNavigationPoint(a) = nullptr;
                        navigationpoint::nextOrdered(a) = nullptr;
                        navigationpoint::prevOrdered(a) = nullptr;
                        navigationpoint::startPath(a) = nullptr;
                        navigationpoint::previousPath(a) = nullptr;
                        for (int i = 0; i < 16; i++) {
                            navigationpoint::Paths(a, i) = -1;
                            navigationpoint::upstreamPaths(a, i) = -1;
                            navigationpoint::PrunedPaths(a, i) = -1;
                            navigationpoint::VisNoReachPaths(a, i) = nullptr;
                        }
                    }
                }

                int buildPaths()
                {
                    numMarkers = 0;
                    markers.assign(MaxMarkers, PathMarker());
                    getScout();
                    if (!scout) {
                        return 0;
                    }
                    setCollision(scout);
                    actor::bCollideWorld(scout) = 1;
                    pawn::JumpZ(scout) = -1.0f;
                    pawn::GroundSpeed(scout) = 320.0f;
                    pawn::MaxStepHeight(scout) = 24.0f;
                    int count = createPaths();
                    destroy(scout);
                    scout = nullptr;
                    return count;
                }

                void setCollision(UObject* a)
                {
                    actor::bCollideActors(a) = 1;
                    actor::bBlockActors(a) = 1;
                    actor::bBlockPlayers(a) = 1;
                    world.collisionHash.update(a);
                }

                Vector scoutLocation()
                {
                    return actor::Location(scout);
                }

                static float distance(const Vector& v)
                {
                    return static_cast<float>(std::sqrt(static_cast<double>(v.sizeSquared())));
                }

                int walkMove(const Vector& delta)
                {
                    CheckResult hit;
                    return pawnWalkMove(scout, delta, hit, nullptr, 4.1f, false);
                }

                bool blocked(const Vector& start, const Vector& end)
                {
                    CheckResult hit;
                    world.lineCheck(hit, start, end, {}, scout, TraceMoversLevel);
                    return hit.time < 1.0f;
                }

                int addMarker()
                {
                    if (numMarkers < MaxMarkers - 1) {
                        numMarkers++;
                    } else {
                        for (int i = 0; i < numMarkers; i++) {
                            if (!(markers[static_cast<size_t>(i)].flags & 0x3a)) {
                                markers[static_cast<size_t>(i)] = markers[static_cast<size_t>(numMarkers - 1)];
                                break;
                            }
                        }
                    }
                    return numMarkers - 1;
                }

                int placeMarker(const Vector& location, const Vector& direction, uint32_t clear, uint32_t set)
                {
                    int index = addMarker();
                    PathMarker& m = markers[static_cast<size_t>(index)];
                    m.location = location;
                    m.direction = direction;
                    m.weight = 1.0f;
                    m.flags = (m.flags & ~clear) | set;
                    return index;
                }

                int createPaths()
                {
                    int numPaths = 0;
                    numMarkers = 0;
                    for (size_t i = 0; i < level->actors.size(); i++) {
                        UObject* a = level->actors[i];
                        if (a && !a->destroyed && isA(a, "PathNode")) {
                            placeMarker(actor::Location(a), {}, 0x64, MarkerMarked | MarkerBeacon | MarkerLeftTurn | MarkerPermanent);
                        }
                    }
                    for (size_t i = 0; i < level->actors.size(); i++) {
                        UObject* a = level->actors[i];
                        if (a && !a->destroyed && (isA(a, "Pawn") || isA(a, "Inventory"))) {
                            createPathsFrom(actor::Location(a));
                        }
                    }
                    for (int i = 0; i < numMarkers; i++) {
                        if (markers[static_cast<size_t>(i)].flags & MarkerMarked) {
                            checkObstructionFrom(i);
                            markers[static_cast<size_t>(i)].flags &= ~MarkerMarked;
                        }
                    }
                    for (int i = 0; i < numMarkers; i++) {
                        if (markers[static_cast<size_t>(i)].flags & MarkerLeftTurn) {
                            mergePath(i);
                        }
                    }
                    for (int i = 0; i < numMarkers; i++) {
                        const PathMarker& m = markers[static_cast<size_t>(i)];
                        if ((m.flags & MarkerLeftTurn) && !(m.flags & MarkerPermanent)) {
                            newPath(m.location);
                            numPaths++;
                        }
                    }
                    for (size_t i = 0; i < level->actors.size(); i++) {
                        UObject* a = level->actors[i];
                        if (a && !a->destroyed && isA(a, "Pawn")) {
                            setCollision(a);
                        }
                    }
                    return numPaths;
                }

                void newPath(Vector spot)
                {
                    if (actor::CollisionHeight(scout) < 48.0f) {
                        spot.z = spot.z + 48.0f - actor::CollisionHeight(scout);
                    }
                    UObject* node = spawn(engineClass("PathNode"), spot);
                    if (!node) {
                        return;
                    }
                    for (int i = 0; i < 16; i++) {
                        navigationpoint::Paths(node, i) = -1;
                        navigationpoint::upstreamPaths(node, i) = -1;
                    }
                }

                void createPathsFrom(const Vector& start)
                {
                    bool found = findScoutStart(start) && std::fabs(scoutLocation().z - start.z) <= actor::CollisionHeight(scout);
                    if (!found) {
                        found = findScoutStart(start + Vector { 0.0f, 0.0f, 20.0f });
                    }
                    if (found) {
                        exploreWall({ 1.0f, 0.0f, 0.0f });
                    }
                }

                void exploreWall(const Vector& moveDirection)
                {
                    setCollisionSize(scout, 115.0f, 10.0f);
                    int moved = 1;
                    while (moved == 1) {
                        moved = walkMove(moveDirection * 16.0f);
                    }
                    Vector normal = moveDirection * -1.0f;
                    findBlockingNormal(normal);
                    followWall({ normal.y, normal.x * -1.0f, 0.0f });
                }

                void findBlockingNormal(Vector& blockNormal)
                {
                    Vector extent { actor::CollisionRadius(scout), actor::CollisionRadius(scout), actor::CollisionHeight(scout) };
                    Vector location = scoutLocation();
                    Vector end = location - blockNormal * 16.0f;
                    CheckResult hit;
                    world.lineCheck(hit, location, end, extent, scout, TraceMoversLevel);
                    if (hit.time < 1.0f) {
                        blockNormal = hit.normal;
                        return;
                    }
                    Vector down { 0.0f, 0.0f, -pawn::MaxStepHeight(scout) };
                    CheckResult landing;
                    world.lineCheck(landing, end, end + down, extent, scout, TraceMoversLevel);
                    if (landing.time < 1.0f) {
                        return;
                    }
                    CheckResult ledge;
                    world.lineCheck(ledge, end + down, location + down, extent, scout, TraceMoversLevel);
                    if (ledge.time < 1.0f) {
                        blockNormal = ledge.normal;
                    }
                }

                void followWall(Vector currentDirection)
                {
                    Vector upstreamSpot = scoutLocation();
                    Vector startLocation = scoutLocation();
                    int stillMoving = 1;
                    bool keepMapping = true;
                    bool blockedMarker = false;
                    int lastMarker = 0;
                    float netYaw = 0.0f;
                    while (keepMapping) {
                        Vector oldPosition = scoutLocation();
                        Vector oldDirection = currentDirection;
                        if (checkLeftPassage(currentDirection)) {
                            netYaw = static_cast<float>(static_cast<double>(netYaw) - 90.0);
                            if (!fullyReachable(scoutLocation(), upstreamSpot) && !oneWaypointTo(upstreamSpot)) {
                                upstreamSpot = oldPosition;
                                lastMarker = placeMarker(oldPosition, oldDirection, 0x64, MarkerMarked | MarkerBeacon | MarkerLeftTurn);
                                netYaw = 0.0f;
                                startLocation = oldPosition;
                            }
                        } else {
                            stillMoving = walkMove(currentDirection * 16.0f);
                            if (stillMoving == 1) {
                                markLeftReachable(oldPosition);
                                if (needPath(scoutLocation())) {
                                    if (!fullyReachable(scoutLocation(), upstreamSpot) && !oneWaypointTo(upstreamSpot) && std::fabs(upstreamSpot.z - scoutLocation().z) > pawn::MaxStepHeight(scout) + 1.0f) {
                                        lastMarker = placeMarker(oldPosition, oldDirection, 0x66, MarkerBeacon | MarkerLeftTurn);
                                        markers[0].flags |= MarkerStair;
                                        netYaw = 0.0f;
                                        upstreamSpot = oldPosition;
                                        startLocation = oldPosition;
                                    } else {
                                        lastMarker = placeMarker(oldPosition, oldDirection, 0x7c, MarkerMarked);
                                        netYaw = 0.0f;
                                        startLocation = oldPosition;
                                    }
                                } else if (sawNewLeft(scoutLocation())) {
                                    lastMarker = placeMarker(scoutLocation(), currentDirection * -1.0f, 0x7c, MarkerMarked);
                                    netYaw = 0.0f;
                                    startLocation = scoutLocation();
                                }
                            }
                        }
                        if (stillMoving == 1) {
                            blockedMarker = false;
                            float radius = actor::CollisionRadius(scout);
                            float maxDist2 = static_cast<float>(static_cast<double>(radius * radius) * 0.25);
                            for (int i = 0; i < numMarkers; i++) {
                                const PathMarker& m = markers[static_cast<size_t>(i)];
                                if (i == lastMarker || (m.location - scoutLocation()).sizeSquared() >= maxDist2) {
                                    continue;
                                }
                                Vector turn = currentDirection - m.direction;
                                double turn2 = static_cast<double>(turn.sizeSquared());
                                if (turn2 >= 1e-8) {
                                    turn = turn * static_cast<float>(1.0 / std::sqrt(turn2));
                                }
                                if (std::fabs(turn.x) < 0.0001f && std::fabs(turn.y) < 0.0001f && std::fabs(turn.z) < 0.0001f) {
                                    keepMapping = false;
                                    break;
                                }
                            }
                        } else {
                            upstreamSpot = scoutLocation();
                            if (!blockedMarker) {
                                blockedMarker = true;
                                lastMarker = placeMarker(oldPosition, oldDirection, 0x76, MarkerBeacon);
                                netYaw = 0.0f;
                                startLocation = oldPosition;
                            }
                            Vector blockNormal = currentDirection * -1.0f;
                            findBlockingNormal(blockNormal);
                            netYaw += 90.0f;
                            stillMoving = 1;
                            currentDirection = { blockNormal.y * -1.0f, blockNormal.x, 0.0f };
                        }
                        if (keepMapping && std::fabs(netYaw) >= 360.0f) {
                            keepMapping = (startLocation - scoutLocation()).dot(currentDirection) > 0.0f;
                        }
                    }
                }

                int checkLeft(Vector& left, Vector& currentDirection)
                {
                    Vector oldLocation = scoutLocation();
                    if (walkMove(left * 16.0f) == 1 && distance(scoutLocation() - oldLocation) > 10.0f) {
                        currentDirection.x = left.x;
                        currentDirection.y = left.y;
                        float leftX = left.x;
                        left.x = left.y * -1.0f;
                        left.y = leftX;
                        walkMove(left * 16.0f);
                        return 1;
                    }
                    world.farMoveActor(scout, oldLocation, false, true);
                    return 0;
                }

                int checkLeftPassage(Vector& currentDirection)
                {
                    Vector oldLocation = scoutLocation();
                    Vector left { currentDirection.y * -1.0f, currentDirection.x, 0.0f };
                    if (checkLeft(left, currentDirection)) {
                        return 1;
                    }
                    int moved = walkMove(currentDirection * 6.0f);
                    if (checkLeft(left, currentDirection)) {
                        return 1;
                    }
                    if (moved) {
                        walkMove(currentDirection * 6.0f);
                        if (checkLeft(left, currentDirection)) {
                            return 1;
                        }
                    }
                    world.farMoveActor(scout, oldLocation, false, true);
                    return 0;
                }

                void checkObstructionFrom(int index)
                {
                    PathMarker& marker = markers[static_cast<size_t>(index)];
                    world.farMoveActor(scout, marker.location, false, true);
                    CheckResult hit;
                    world.moveActor(scout, { 0.0f, 0.0f, -1000.0f }, actor::Rotation(scout), hit, false, false, false, false);
                    if (marker.flags & MarkerLeftTurn) {
                        exploreWall(marker.direction);
                        return;
                    }
                    markLeftReachable(marker.location);
                    walkMove(marker.direction * 16.0f);
                    for (int i = 0; i < numMarkers; i++) {
                        Vector location = markers[static_cast<size_t>(i)].location;
                        if (!(markers[static_cast<size_t>(i)].flags & MarkerVisible)) {
                            continue;
                        }
                        if (blocked(scoutLocation(), location) && !findPathTo(location)) {
                            Vector toward = location - scoutLocation();
                            toward.z = 0.0f;
                            double toward2 = static_cast<double>(toward.sizeSquared());
                            if (toward2 >= 1e-8) {
                                toward = toward * static_cast<float>(1.0 / std::sqrt(toward2));
                            }
                            exploreWall(toward);
                        }
                    }
                }

                void markLeftReachable(const Vector& start)
                {
                    for (int i = 0; i < numMarkers; i++) {
                        PathMarker& m = markers[static_cast<size_t>(i)];
                        if ((start - m.location).sizeSquared() < 640000.0f) {
                            m.flags &= ~(MarkerVisible | MarkerRoutable);
                            if (!blocked(start, m.location)) {
                                m.flags |= MarkerVisible;
                            }
                        } else {
                            m.flags &= ~MarkerVisible;
                        }
                    }
                }

                int needPath(const Vector& start)
                {
                    for (int i = 0; i < numMarkers; i++) {
                        Vector location = markers[static_cast<size_t>(i)].location;
                        uint32_t flags = markers[static_cast<size_t>(i)].flags;
                        if ((flags & MarkerVisible) && (flags & MarkerBeacon) && (start - location).sizeSquared() < 640000.0f) {
                            if (blocked(start, location) && !findPathTo(location)) {
                                return 1;
                            }
                        }
                    }
                    return 0;
                }

                int sawNewLeft(const Vector& start)
                {
                    for (int i = 0; i < numMarkers; i++) {
                        const PathMarker& m = markers[static_cast<size_t>(i)];
                        if (!(m.flags & (MarkerVisible | MarkerRoutable)) && (m.flags & MarkerLeftTurn) && (start - m.location).sizeSquared() < 640000.0f && fullyReachable(start, m.location)) {
                            return 1;
                        }
                    }
                    return 0;
                }

                int oneWaypointTo(const Vector& upstreamSpot)
                {
                    float radius = actor::CollisionRadius(scout);
                    float maxDist2 = static_cast<float>(static_cast<double>(radius * radius) * 4.0);
                    int success = 0;
                    for (int i = 0; i < numMarkers && !success; i++) {
                        const PathMarker& m = markers[static_cast<size_t>(i)];
                        if ((m.flags & MarkerLeftTurn) && (m.location - scoutLocation()).sizeSquared() < maxDist2) {
                            success = fullyReachable(m.location, upstreamSpot) && fullyReachable(scoutLocation(), m.location);
                        }
                    }
                    return success;
                }

                int findPathTo(const Vector& destination)
                {
                    float budget = static_cast<float>(static_cast<double>(distance(destination - scoutLocation())) + static_cast<double>(actor::CollisionRadius(scout)) * 5.0);
                    for (int i = 0; i < numMarkers; i++) {
                        markers[static_cast<size_t>(i)].budget = 0.0f;
                    }
                    PathMarker waypoint;
                    waypoint.location = scoutLocation();
                    return tryPathThrough(waypoint, destination, budget);
                }

                int tryPathThrough(PathMarker& waypoint, const Vector& destination, float budget)
                {
                    if (fullyReachable(waypoint.location, destination)) {
                        return 1;
                    }
                    waypoint.budget = budget;
                    int result = 0;
                    for (int i = 0; i < numMarkers && !result; i++) {
                        PathMarker& m = markers[static_cast<size_t>(i)];
                        if (!(m.flags & MarkerLeftTurn)) {
                            continue;
                        }
                        float toMarker = distance(waypoint.location - m.location);
                        float toDestination = distance(m.location - destination);
                        float nextBudget = budget - toMarker;
                        if (m.budget < nextBudget && toMarker + toDestination < budget && fullyReachable(waypoint.location, m.location)) {
                            result = tryPathThrough(m, destination, nextBudget);
                        }
                    }
                    return result;
                }

                int fullyReachable(Vector start, Vector destination)
                {
                    Vector saved = scoutLocation();
                    setCollisionSize(scout, actor::CollisionRadius(scout) - 6.0f, actor::CollisionHeight(scout));
                    int result = world.farMoveActor(scout, start, false, false) ? 1 : 0;
                    actor::Physics(scout) = 1;
                    if (result) {
                        result = pawnPointReachable(scout, destination, false);
                    }
                    if (result) {
                        world.farMoveActor(scout, destination, false, false);
                        result = pawnWalkReachable(scout, start, 15.0f, 0, nullptr) ? 1 : 0;
                    }
                    world.farMoveActor(scout, saved, false, true);
                    setCollisionSize(scout, actor::CollisionRadius(scout) + 6.0f, actor::CollisionHeight(scout));
                    return result;
                }

                void mergePath(int index)
                {
                    PathMarker& marker = markers[static_cast<size_t>(index)];
                    Vector floor { marker.location.x, marker.location.y, marker.location.z - (pawn::MaxStepHeight(scout) + 10.0f) };
                    CheckResult hit;
                    world.lineCheck(hit, marker.location, floor, {}, nullptr, TraceMoversLevel);
                    if (hit.time == 1.0f) {
                        marker.flags &= ~(MarkerBeacon | MarkerLeftTurn);
                        return;
                    }
                    marker.radius = 12.0f;
                    for (int i = 0; i < numMarkers; i++) {
                        PathMarker& other = markers[static_cast<size_t>(i)];
                        if (!(other.flags & MarkerLeftTurn) || (other.flags & MarkerPermanent) || i == index) {
                            continue;
                        }
                        setCollisionSize(scout, 12.0f, 10.0f);
                        if ((marker.location - other.location).sizeSquared() >= 26450.0f || !fullyReachable(marker.location, other.location)) {
                            continue;
                        }
                        float scale = static_cast<float>(1.0 / static_cast<double>(marker.weight + other.weight));
                        Vector average = (marker.location * marker.weight + other.location * other.weight) * scale;
                        setCollisionSize(scout, 115.0f, 10.0f);
                        if (!(marker.flags & MarkerPermanent)) {
                            marker.location = average;
                        }
                        marker.weight = marker.weight + 1.0f;
                        other.flags &= ~(MarkerBeacon | MarkerLeftTurn);
                    }
                }

                ULevel* level = nullptr;
                World* previous = nullptr;
                World world;
                UObject* scout = nullptr;
                std::vector<PathMarker> markers;
                int numMarkers = 0;
            };

            int countPathNodes(ULevel* level, int drawType, bool remove)
            {
                int count = 0;
                std::vector<UObject*> actors = level->actors;
                for (UObject* a : actors) {
                    if (!a || a->destroyed || !isA(a, "PathNode")) {
                        continue;
                    }
                    count++;
                    if (remove) {
                        level->actors.erase(std::remove(level->actors.begin(), level->actors.end(), a), level->actors.end());
                        a->destroyed = true;
                    } else {
                        actor::DrawType(a) = static_cast<uint8_t>(drawType);
                    }
                }
                return count;
            }
        }

        int removePaths(ULevel* level)
        {
            return level ? countPathNodes(level, 0, true) : 0;
        }

        int showPaths(ULevel* level)
        {
            return level ? countPathNodes(level, 1, false) : 0;
        }

        int hidePaths(ULevel* level)
        {
            return level ? countPathNodes(level, 0, false) : 0;
        }

        void definePaths(ULevel* level)
        {
            if (!level || level->actors.empty() || !level->actors[0]) {
                return;
            }
            Builder builder(level);
            builder.definePaths();
        }

        void undefinePaths(ULevel* level)
        {
            if (!level || level->actors.empty() || !level->actors[0]) {
                return;
            }
            Builder builder(level);
            builder.undefinePaths();
        }

        int buildPaths(ULevel* level, int)
        {
            if (!level || level->actors.empty() || !level->actors[0]) {
                return 0;
            }
            Builder builder(level);
            return builder.buildPaths();
        }
    }
}
