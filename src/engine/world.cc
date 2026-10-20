#include "engine/world.h"

#include "audio/audio.h"
#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "deusex/deusex.h"
#include "engine/ai.h"
#include "engine/actor.h"
#include "engine/anim.h"
#include "engine/latent.h"
#include "engine/props_deusex.h"
#include "engine/resources.h"
#include "engine/savegame.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <deque>
#include <unordered_map>
#include <unordered_set>

#include <unistd.h>

namespace dx {
    World* World::current = nullptr;

    namespace {
        EngineClasses classesInstance;

        struct ParsedUrl {
            std::string map;
            std::string portal;
            std::vector<std::string> options;
        };

        ParsedUrl parseUrl(const std::string& text)
        {
            ParsedUrl url;
            std::string rest = text;
            size_t hash = rest.find('#');
            if (hash != std::string::npos) {
                url.portal = rest.substr(hash + 1);
                rest = rest.substr(0, hash);
            }
            size_t question = rest.find('?');
            url.map = rest.substr(0, question);
            while (question != std::string::npos) {
                size_t next = rest.find('?', question + 1);
                url.options.push_back(rest.substr(question + 1, next == std::string::npos ? std::string::npos : next - question - 1));
                question = next;
            }
            size_t dot = url.map.rfind('.');
            if (dot != std::string::npos) {
                url.map = url.map.substr(0, dot);
            }
            size_t slash = url.map.find_last_of("/\\");
            if (slash != std::string::npos) {
                url.map = url.map.substr(slash + 1);
            }
            return url;
        }

        std::string option(const ParsedUrl& url, std::string_view key)
        {
            for (const std::string& item : url.options) {
                size_t equals = item.find('=');
                std::string itemKey = item.substr(0, equals);
                if (iequals(itemKey, key)) {
                    return equals == std::string::npos ? std::string() : item.substr(equals + 1);
                }
            }
            return {};
        }

        std::string optionsString(const ParsedUrl& url)
        {
            std::string result;
            for (const std::string& item : url.options) {
                result += "?" + item;
            }
            return result;
        }

        std::unordered_map<UClass*, std::vector<size_t>>& referenceOffsets()
        {
            static std::unordered_map<UClass*, std::vector<size_t>> table;
            return table;
        }

        const std::vector<size_t>& objectReferences(UClass* cls)
        {
            auto& table = referenceOffsets();
            auto it = table.find(cls);
            if (it != table.end()) {
                return it->second;
            }
            std::vector<size_t> offsets;
            for (UProperty* property : cls->properties) {
                if (property->type != PropType::Object) {
                    continue;
                }
                if (property->name == Name("Outer") || property->name == Name("Class")) {
                    continue;
                }
                for (int32_t i = 0; i < property->arrayDim; i++) {
                    offsets.push_back(property->offset + static_cast<size_t>(i) * property->elementSize());
                }
            }
            return table.emplace(cls, std::move(offsets)).first->second;
        }
    }

    EngineClasses& engineClasses()
    {
        return classesInstance;
    }

    void bindEngineClasses()
    {
        EngineClasses& c = classesInstance;
        c.actor = objects::findClass("Engine.Actor");
        c.pawn = objects::findClass("Engine.Pawn");
        c.playerPawn = objects::findClass("Engine.PlayerPawn");
        c.decoration = objects::findClass("Engine.Decoration");
        c.inventory = objects::findClass("Engine.Inventory");
        c.weapon = objects::findClass("Engine.Weapon");
        c.projectile = objects::findClass("Engine.Projectile");
        c.mover = objects::findClass("Engine.Mover");
        c.brush = objects::findClass("Engine.Brush");
        c.levelInfo = objects::findClass("Engine.LevelInfo");
        c.zoneInfo = objects::findClass("Engine.ZoneInfo");
        c.skyZoneInfo = objects::findClass("Engine.SkyZoneInfo");
        c.warpZoneInfo = objects::findClass("Engine.WarpZoneInfo");
        c.navigationPoint = objects::findClass("Engine.NavigationPoint");
        c.interpolationPoint = objects::findClass("Engine.InterpolationPoint");
        c.carcass = objects::findClass("Engine.Carcass");
        c.gameInfo = objects::findClass("Engine.GameInfo");
        c.camera = objects::findClass("Engine.Camera");
        c.light = objects::findClass("Engine.Light");
        c.viewport = objects::findClass("Engine.Viewport");
        if (!c.viewport) {
            c.viewport = objects::findClass("Engine.Player");
        }
        c.canvas = objects::findClass("Engine.Canvas");
        c.playerPawnExt = objects::findClass("Extension.PlayerPawnExt");
        c.scriptedPawn = objects::findClass("DeusEx.ScriptedPawn");
        c.deusExPlayer = objects::findClass("DeusEx.DeusExPlayer");
        objects::flushLoads();
    }

    UObject* playerPawnOf(UObject* a)
    {
        if (isPlayerPawn(a) && playerpawn::Player(a)) {
            return a;
        }
        return nullptr;
    }

    bool isBlockedBy(UObject* a, UObject* other)
    {
        World* world = World::current;
        if (world && other == world->levelInfo) {
            return actor::bCollideWorld(a) != 0;
        }
        bool aPlayer = playerPawnOf(a) != nullptr;
        bool otherPlayer = playerPawnOf(other) != nullptr;
        if (isBrush(other)) {
            return actor::bCollideWorld(a) && (aPlayer ? actor::bBlockPlayers(other) : actor::bBlockActors(other));
        }
        if (isBrush(a)) {
            return actor::bCollideWorld(other) && (otherPlayer ? actor::bBlockPlayers(a) : actor::bBlockActors(a));
        }
        bool aProjectile = a->isA(engineClasses().projectile);
        bool otherProjectile = other->isA(engineClasses().projectile);
        bool first = (aPlayer || aProjectile) ? actor::bBlockPlayers(other) != 0 : actor::bBlockActors(other) != 0;
        bool second = (otherPlayer || otherProjectile) ? actor::bBlockPlayers(a) != 0 : actor::bBlockActors(a) != 0;
        return first && second;
    }

    bool isBasedOn(UObject* a, UObject* other)
    {
        int guard = 0;
        for (UObject* test = a; test && guard < 64; test = actor::Base(test), guard++) {
            if (test == other) {
                return true;
            }
        }
        return false;
    }

    bool isOverlapping(UObject* a, UObject* other)
    {
        World* world = World::current;
        if (isBrush(a) || isBrush(other) || (world && other == world->levelInfo)) {
            return false;
        }
        Vector d = actor::Location(a) - actor::Location(other);
        float radius = actor::CollisionRadius(a) + actor::CollisionRadius(other);
        float height = actor::CollisionHeight(a) + actor::CollisionHeight(other);
        return d.x * d.x + d.y * d.y < radius * radius && d.z * d.z < height * height;
    }

    void setOwner(UObject* a, UObject* newOwner)
    {
        UObject* old = actor::Owner(a);
        if (old) {
            vm::Call(old, Name("LostChild")).obj(a).run();
        }
        actor::Owner(a) = newOwner;
        if (newOwner) {
            vm::Call(newOwner, Name("GainedChild")).obj(a).run();
        }
    }

    void setBase(UObject* a, UObject* newBase, bool notify)
    {
        int guard = 0;
        for (UObject* loop = newBase; loop && guard < 64; loop = actor::Base(loop), guard++) {
            if (loop == a) {
                return;
            }
        }
        UObject* base = actor::Base(a);
        if (newBase == base) {
            return;
        }
        UObject* level = World::current ? World::current->levelInfo : nullptr;
        if (base && base != level) {
            actor::StandingCount(base)--;
            vm::Call(base, Name("Detach")).obj(a).run();
        }
        actor::Base(a) = newBase;
        if (newBase && newBase != level) {
            actor::StandingCount(newBase)++;
            vm::Call(newBase, Name("Attach")).obj(a).run();
        }
        if (notify) {
            vm::event(a, Name("BaseChange"));
        }
    }

    namespace {
        bool touchTo(UObject* a, UObject* other)
        {
            int available = -1;
            for (int i = 0; i < 4; i++) {
                UObject* touching = actor::Touching(a, i);
                if (!touching) {
                    available = i;
                } else if (touching == other) {
                    return true;
                }
            }
            if (available == -1) {
                for (int i = 0; i < 4; i++) {
                    UObject* touching = actor::Touching(a, i);
                    if (touching && actor::Physics(touching) == PHYS_None) {
                        endTouch(a, touching, false);
                        available = i;
                    }
                }
            }
            if (available == -1 && isPawn(other)) {
                for (int i = 0; i < 4; i++) {
                    UObject* touching = actor::Touching(a, i);
                    if (touching && !isPawn(touching)) {
                        endTouch(a, touching, false);
                        available = i;
                        break;
                    }
                }
                if (available == -1 && pawn::bIsPlayer(other)) {
                    for (int i = 0; i < 4; i++) {
                        UObject* touching = actor::Touching(a, i);
                        if (touching && (!isPawn(touching) || !pawn::bIsPlayer(touching))) {
                            endTouch(a, touching, false);
                            available = i;
                            break;
                        }
                    }
                }
            }
            if (available >= 0) {
                actor::Touching(a, available) = other;
                vm::Call(a, Name("Touch")).obj(other).run();
                if (actor::Touching(a, available) != other) {
                    return false;
                }
            }
            return true;
        }
    }

    void beginTouch(UObject* a, UObject* other)
    {
        if (a == other || a->destroyed || other->destroyed) {
            return;
        }
        if (touchTo(a, other)) {
            touchTo(other, a);
        }
    }

    void endTouch(UObject* a, UObject* other, bool noNotifySelf)
    {
        for (int i = 0; i < 4; i++) {
            if (actor::Touching(a, i) == other) {
                actor::Touching(a, i) = nullptr;
                if (!noNotifySelf) {
                    vm::Call(a, Name("UnTouch")).obj(other).run();
                }
                break;
            }
        }
        for (int i = 0; i < 4; i++) {
            if (actor::Touching(other, i) == a) {
                actor::Touching(other, i) = nullptr;
                vm::Call(other, Name("UnTouch")).obj(a).run();
                break;
            }
        }
    }

    void setCollision(UObject* a, bool collideActors, bool blockActors, bool blockPlayers)
    {
        actor::bCollideActors(a) = collideActors ? 1 : 0;
        actor::bBlockActors(a) = blockActors ? 1 : 0;
        actor::bBlockPlayers(a) = blockPlayers ? 1 : 0;
        if (World::current) {
            World::current->collisionHash.update(a);
        }
    }

    void setPhysics(UObject* a, uint8_t newPhysics, UObject* newFloor)
    {
        if (actor::Physics(a) == newPhysics) {
            return;
        }
        actor::Physics(a) = newPhysics;
        if (newPhysics == PHYS_Walking || newPhysics == PHYS_None || newPhysics == PHYS_Rolling || newPhysics == PHYS_Rotating || newPhysics == PHYS_Spider) {
            if (newFloor) {
                if (actor::Base(a) != newFloor) {
                    setSupportBase(a, newFloor);
                }
            } else {
                findBase(a);
            }
        } else if (actor::Base(a)) {
            setBase(a, nullptr, true);
        }
        if (actor::Physics(a) == PHYS_None || actor::Physics(a) == PHYS_Rotating) {
            actor::Velocity(a) = {};
            actor::Acceleration(a) = {};
        }
    }

    void setSupportBase(UObject* a, UObject* newBase)
    {
        if (newBase) {
            vm::Call(newBase, Name("SupportActor")).obj(a).run();
        } else {
            setBase(a, nullptr, true);
        }
    }

    void findBase(UObject* a)
    {
        World* world = World::current;
        if (!world) {
            return;
        }
        CheckResult hit;
        Vector location = actor::Location(a);
        world->lineCheck(hit, location, location + Vector { 0, 0, -8 }, cylinderExtent(a), a, TRACE_AllColliding);
        if (actor::Base(a) != hit.actor && (!hit.actor || !isBasedOn(hit.actor, a))) {
            setSupportBase(a, hit.actor);
        }
    }

    World::World()
    {
        current = this;
    }

    World::~World()
    {
        if (current == this) {
            current = nullptr;
        }
    }

    UObject* World::player() const
    {
        return localPlayer;
    }

    bool World::loadMap(const std::string& mapUrl)
    {
        ParsedUrl parsed = parseUrl(mapUrl);
        std::vector<std::string> defaults;
        for (const auto& [key, value] : config::user().section("DefaultPlayer")) {
            if (option(parsed, key).empty() && std::none_of(parsed.options.begin(), parsed.options.end(), [&](const std::string& item) { return iequals(item.substr(0, item.find('=')), key); })) {
                defaults.push_back(key + "=" + value);
            }
        }
        parsed.options.insert(parsed.options.begin(), defaults.begin(), defaults.end());
        Linker* linker = savegame::takePendingLevel(parsed.map);
        bool restored = linker != nullptr;
        if (!linker) {
            linker = objects::loadPackage(Name(parsed.map));
        }
        if (!linker) {
            std::fprintf(stderr, "map not found: %s\n", parsed.map.c_str());
            return false;
        }
        int32_t levelRef = linker->findExport(Name("MyLevel"), Name("Level"), "");
        if (levelRef == 0) {
            std::fprintf(stderr, "no level in %s\n", parsed.map.c_str());
            return false;
        }
        level = cast<ULevel>(linker->object(levelRef));
        objects::ensureLoaded(level);
        objects::flushLoads();
        if (replaceTravelInfo) {
            level->travelInfo = arrivalTravelInfo;
        }
        url = mapUrl;
        actors.clear();
        actorsVersion++;
        for (UObject* a : level->actors) {
            if (a || actors.size() < 2) {
                actors.push_back(a);
            }
        }
        level->actors = actors;
        collisionHash.clear();
        collisionHash.sync(actors);
        levelInfo = actors.empty() ? nullptr : actors[0];
        if (!levelInfo) {
            return false;
        }
        timeSeconds = 0;
        for (UObject* a : actors) {
            if (!a) {
                continue;
            }
            actor::XLevel(a) = level;
            actor::Level(a) = levelInfo;
            if (!levelinfo::bBegunPlay(levelInfo)) {
                for (int i = 0; i < 4; i++) {
                    actor::Touching(a, i) = nullptr;
                }
            }
        }
        deusex::levelLoaded(*this);
        if (restored) {
            savegame::levelRestored(*this);
        }
        char host[256] = {};
#ifdef _WIN32
        if (const char* name = std::getenv("COMPUTERNAME")) {
            std::snprintf(host, sizeof(host), "%s", name);
        }
#else
        gethostname(host, sizeof(host) - 1);
#endif
        std::string computerName(host);
        std::transform(computerName.begin(), computerName.end(), computerName.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });
        levelinfo::ComputerName(levelInfo) = computerName;
        levelinfo::EngineVersion(levelInfo) = "1100";
        levelinfo::MinNetVersion(levelInfo) = "1100";
        levelinfo::bHighDetailMode(levelInfo) = 1;

        UObject* game = levelinfo::Game(levelInfo);
        if (!game) {
            UClass* gameClass = nullptr;
            std::string gameName = option(parsed, "Game");
            if (!gameName.empty()) {
                gameClass = objects::findClass(gameName);
            }
            if (!gameClass) {
                gameClass = cast<UClass>(levelinfo::DefaultGameType(levelInfo));
            }
            if (!gameClass) {
                auto defaultGame = config::system().get("Engine.Engine", "DefaultGame");
                gameClass = objects::findClass(defaultGame ? *defaultGame : "DeusEx.DeusExGameInfo");
            }
            objects::flushLoads();
            game = spawn(gameClass, nullptr, Name(), {}, {}, nullptr, true);
            levelinfo::Game(levelInfo) = game;
        }
        gameInfo = game;

        if (!levelinfo::bBegunPlay(levelInfo)) {
            levelinfo::bBegunPlay(levelInfo) = 1;
            levelinfo::bStartup(levelInfo) = 1;
            std::string options = optionsString(parsed);
            if (gameInfo) {
                vm::Call(gameInfo, Name("InitGame")).s(options).s("").run();
            }
            auto forEachActor = [this](const std::function<void(UObject*)>& visit) {
                for (size_t i = 0; i < actors.size(); i++) {
                    UObject* a = actors[i];
                    if (a && !a->destroyed) {
                        visit(a);
                    }
                }
            };
            forEachActor([](UObject* a) {
                vm::event(a, Name("PreBeginPlay"));
            });
            forEachActor([](UObject* a) {
                vm::event(a, Name("BeginPlay"));
            });
            forEachActor([this](UObject* a) {
                region(a) = { levelInfo, -1, 0 };
                if (isPawn(a)) {
                    *reinterpret_cast<PointRegion*>(pawn::FootRegion(a)) = { levelInfo, -1, 0 };
                    *reinterpret_cast<PointRegion*>(pawn::HeadRegion(a)) = { levelInfo, -1, 0 };
                }
                setLocationZone(a, actor::Location(a), true);
            });
            forEachActor([](UObject* a) {
                actor::LastRenderTime(a) = -10.0f;
            });
            forEachActor([](UObject* a) {
                vm::event(a, Name("PostBeginPlay"));
            });
            forEachActor([](UObject* a) {
                vm::event(a, Name("SetInitialState"));
            });
            forEachActor([this](UObject* a) {
                Name attachTag = actor::AttachTag(a);
                if (!attachTag.none()) {
                    for (UObject* other : actors) {
                        if (other && !other->destroyed && actor::Tag(other) == attachTag) {
                            setBase(a, other, false);
                            break;
                        }
                    }
                    return;
                }
                if (actor::Base(a) || !actor::bCollideWorld(a)) {
                    return;
                }
                bool candidate = a->isA(engineClasses().decoration) || a->isA(engineClasses().inventory) || isPawn(a);
                uint8_t physics = actor::Physics(a);
                if (candidate && (physics == PHYS_None || physics == PHYS_Rotating)) {
                    findBase(a);
                    if (actor::Base(a) == levelInfo) {
                        setBase(a, nullptr, false);
                    }
                }
            });
            levelinfo::bStartup(levelInfo) = 0;
        }
        for (size_t i = 0; i < actors.size(); i++) {
            if (actors[i] && !actors[i]->destroyed) {
                vm::event(actors[i], Name("PostPostBeginPlay"));
            }
        }
        std::vector<UObject*> sorted(actors.begin(), actors.begin() + std::min<ptrdiff_t>(2, static_cast<ptrdiff_t>(actors.size())));
        for (size_t i = 2; i < actors.size(); i++) {
            if (actors[i] && actor::bStatic(actors[i])) {
                sorted.push_back(actors[i]);
            }
        }
        firstDynamicActor = sorted.size();
        for (size_t i = 2; i < actors.size(); i++) {
            if (actors[i] && !actor::bStatic(actors[i])) {
                sorted.push_back(actors[i]);
            }
        }
        actors = sorted;
        actorsVersion++;
        level->actors = actors;
        collisionHash.sync(actors);
        if (gameInfo) {
            vm::event(gameInfo, Name("DetailChange"));
        }
        runtimeStart = objects::all().size();

        UClass* playerClass = nullptr;
        std::string className = option(parsed, "Class");
        if (!className.empty()) {
            playerClass = objects::findClass(className);
        }
        if (!playerClass) {
            auto defaultClass = config::user().get("DefaultPlayer", "Class");
            playerClass = objects::findClass(defaultClass ? *defaultClass : "DeusEx.JCDentonMale");
        }
        objects::flushLoads();
        if (gameInfo && playerClass) {
            vm::Call login(gameInfo, Name("Login"));
            login.s(parsed.portal).s(optionsString(parsed)).s("").obj(playerClass).run();
            UObject* pawn = login.returnValue() ? *reinterpret_cast<UObject**>(login.returnValue()) : nullptr;
            if (!pawn) {
                std::string error = login.parameter(2) ? *reinterpret_cast<std::string*>(login.parameter(2)) : std::string();
                std::fprintf(stderr, "login failed: %s\n", error.c_str());
                return false;
            }
            if (!viewport) {
                viewport = objects::newObject(engineClasses().viewport, objects::packageObject(Name("Transient")), Name("Viewport0"), 0);
                UClass* consoleClass = objects::findClass(config::system().get("Engine.Engine", "Console").value_or("Engine.Console"));
                if (consoleClass) {
                    UObject* consoleObject = objects::newObject(consoleClass, objects::packageObject(Name("Transient")), Name(), 0);
                    console::Viewport(consoleObject) = viewport;
                    player::Console(viewport) = consoleObject;
                }
            }
            UObject* oldActor = player::Actor(viewport);
            if (oldActor) {
                playerpawn::Player(oldActor) = nullptr;
            }
            playerpawn::Player(pawn) = viewport;
            player::Actor(viewport) = pawn;
            localPlayer = pawn;
            vm::event(pawn, Name("Possess"));
            vm::event(pawn, Name("TravelPreAccept"));
            std::vector<UObject*> accepted = applyTravel(pawn);
            for (auto it = accepted.rbegin(); it != accepted.rend(); ++it) {
                vm::event(*it, Name("TravelPreAccept"));
            }
            vm::Call(gameInfo, Name("AcceptInventory")).obj(pawn).run();
            for (auto it = accepted.rbegin(); it != accepted.rend(); ++it) {
                vm::event(*it, Name("TravelPostAccept"));
            }
            vm::event(pawn, Name("TravelPostAccept"));
            vm::Call(gameInfo, Name("PostLogin")).obj(pawn).run();
        }
        return true;
    }

    std::string World::travelString(UObject* player) const
    {
        std::string out;
        std::deque<UObject*> queue { player };
        std::unordered_set<UObject*> seen { player };
        while (!queue.empty()) {
            UObject* object = queue.front();
            queue.pop_front();
            char header[512];
            std::snprintf(header, sizeof(header), "Class=%s Name=%s Ptr=%08x\r\n{\r\n", object->cls->pathName().c_str(), object->name.c_str(), object->handle);
            out += header;
            for (UProperty* property : object->cls->properties) {
                if (!(property->propertyFlags & CPF_Travel)) {
                    continue;
                }
                for (int32_t i = 0; i < property->arrayDim; i++) {
                    size_t offset = property->offset + static_cast<size_t>(i) * property->elementSize();
                    if (object->cls->defaultObject && property->equals(object->data + offset, object->cls->defaultObject->data + offset)) {
                        continue;
                    }
                    std::string text;
                    if (property->type == PropType::Object) {
                        UObject* value = *reinterpret_cast<UObject**>(object->data + offset);
                        if (value) {
                            char pointer[16];
                            std::snprintf(pointer, sizeof(pointer), "%08x ", value->handle);
                            text = pointer + value->name.str();
                            if (seen.insert(value).second) {
                                queue.push_back(value);
                            }
                        } else {
                            text = "None";
                        }
                    } else {
                        text = property->toText(object->data + offset);
                        std::replace(text.begin(), text.end(), '\n', '\\');
                    }
                    out += property->name.str();
                    if (property->arrayDim > 1) {
                        out += "[" + std::to_string(i) + "]";
                    }
                    out += "=" + text + "\r\n";
                }
            }
            out += "}\r\n";
        }
        return out;
    }

    std::vector<TravelActor> World::collectTravel()
    {
        std::vector<TravelActor> result;
        UObject* player = localPlayer;
        if (!player || player->destroyed) {
            return result;
        }
        std::vector<UObject*> travellers;
        std::deque<UObject*> queue { player };
        std::unordered_set<UObject*> seen { player };
        while (!queue.empty()) {
            UObject* object = queue.front();
            queue.pop_front();
            bool isActor = object->isA(engineClasses().actor);
            if (isActor && !actor::bTravel(object)) {
                continue;
            }
            if (isActor && !object->destroyed) {
                travellers.push_back(object);
            }
            for (UProperty* property : object->cls->properties) {
                if (!(property->propertyFlags & CPF_Travel) || property->type != PropType::Object) {
                    continue;
                }
                for (int32_t i = 0; i < property->arrayDim; i++) {
                    size_t offset = property->offset + static_cast<size_t>(i) * property->elementSize();
                    if (object->cls->defaultObject && property->equals(object->data + offset, object->cls->defaultObject->data + offset)) {
                        continue;
                    }
                    UObject* value = *reinterpret_cast<UObject**>(object->data + offset);
                    if (value && seen.insert(value).second) {
                        queue.push_back(value);
                    }
                }
            }
        }
        for (UObject* a : travellers) {
            TravelActor entry;
            entry.cls = a->cls;
            entry.original = a;
            entry.isPlayer = a == player;
            for (UProperty* property : a->cls->properties) {
                if (!(property->propertyFlags & CPF_Travel)) {
                    continue;
                }
                for (int32_t i = 0; i < property->arrayDim; i++) {
                    size_t offset = property->offset + static_cast<size_t>(i) * property->elementSize();
                    if (a->cls->defaultObject && property->equals(a->data + offset, a->cls->defaultObject->data + offset)) {
                        continue;
                    }
                    TravelValue value;
                    value.property = property;
                    value.index = i;
                    value.data.resize(property->elementSize() + 16);
                    property->construct(value.data.data());
                    property->copy(value.data.data(), a->data + offset);
                    entry.values.push_back(std::move(value));
                }
            }
            result.push_back(std::move(entry));
        }
        return result;
    }

    void World::releaseTravel(std::vector<TravelActor>& travel)
    {
        for (TravelActor& entry : travel) {
            for (TravelValue& value : entry.values) {
                value.property->destroy(value.data.data());
            }
        }
        travel.clear();
    }

    std::vector<UObject*> World::applyTravel(UObject* player)
    {
        std::vector<UObject*> accepted;
        if (incomingTravel.empty()) {
            return accepted;
        }
        std::unordered_map<UObject*, UObject*> remap;
        std::vector<std::pair<TravelActor*, UObject*>> targets;
        for (TravelActor& entry : incomingTravel) {
            UObject* target = nullptr;
            if (entry.isPlayer) {
                target = player;
            } else {
                target = spawn(entry.cls, player, Name(), actor::Location(player), actor::Rotation(player), nullptr, true);
                if (target) {
                    accepted.push_back(target);
                }
            }
            if (target) {
                remap[entry.original] = target;
                targets.emplace_back(&entry, target);
            }
        }
        for (auto& [entry, target] : targets) {
            for (TravelValue& value : entry->values) {
                UProperty* property = value.property;
                if (!target->cls->isChildOf(cast<UStruct>(property->outer))) {
                    continue;
                }
                uint8_t* destination = target->data + property->offset + static_cast<size_t>(value.index) * property->elementSize();
                if (property->type == PropType::Object) {
                    UObject* reference = *reinterpret_cast<UObject**>(value.data.data());
                    auto mapped = remap.find(reference);
                    if (mapped != remap.end()) {
                        reference = mapped->second;
                    } else if (reference && reference->isA(engineClasses().actor)) {
                        reference = nullptr;
                    }
                    *reinterpret_cast<UObject**>(destination) = reference;
                } else {
                    property->copy(destination, value.data.data());
                }
            }
        }
        releaseTravel(incomingTravel);
        return accepted;
    }

    UObject* World::spawn(UClass* cls, UObject* owner, Name tag, const Vector& location, const Rotator& rotation, UObject* templateActor, bool noCollisionFail, UObject* instigator)
    {
        if (!cls) {
            return nullptr;
        }
        objects::ensureLoaded(cls);
        cls->linkClass();
        if ((cls->classFlags & CLASS_Abstract) || !cls->isChildOf(engineClasses().actor)) {
            std::fprintf(stderr, "spawn failed: class %s is abstract or not an actor\n", cls->name.c_str());
            return nullptr;
        }
        UObject* defaults = cls->defaultObject;
        if (actor::bStatic(defaults) || actor::bNoDelete(defaults)) {
            std::fprintf(stderr, "spawn failed: class %s is static or nodelete\n", cls->name.c_str());
            return nullptr;
        }
        UObject* source = templateActor ? templateActor : defaults;
        Vector spawnLocation = location;
        if ((actor::bCollideWorld(source) || actor::bCollideWhenPlacing(source)) && !noCollisionFail) {
            if (!findSpot(cylinderExtent(source), spawnLocation, false, true)) {
                return nullptr;
            }
        }
        UObject* a = objects::newObject(cls, level && level->outer ? level->outer : objects::packageObject(Name("Transient")), Name(), 0);
        if (templateActor) {
            cls->copyValue(a->data, templateActor->data);
            objects::syncObjectProperties(a);
        }
        actors.push_back(a);
        actorsVersion++;
        if (level) {
            level->actors = actors;
        }
        actor::Tag(a) = tag.none() ? cls->name : tag;
        actor::Level(a) = levelInfo;
        actor::XLevel(a) = level;
        actor::bTicked(a) = tickParity ? 0 : 1;
        actor::Brush(a) = nullptr;
        actor::Location(a) = spawnLocation;
        actor::OldLocation(a) = spawnLocation;
        actor::Rotation(a) = rotation;
        collisionHash.update(a);
        region(a) = { levelInfo, -1, 0 };
        if (isPawn(a)) {
            *reinterpret_cast<PointRegion*>(pawn::FootRegion(a)) = { levelInfo, -1, 0 };
            *reinterpret_cast<PointRegion*>(pawn::HeadRegion(a)) = { levelInfo, -1, 0 };
        }
        if (engineClasses().playerPawnExt && a->isA(engineClasses().playerPawnExt)) {
            uint8_t* flagBase = a->propertyAddress(Name("flagBase"));
            UClass* flagClass = objects::findClass("Extension.FlagBase");
            if (flagBase && flagClass && !*reinterpret_cast<UObject**>(flagBase)) {
                *reinterpret_cast<UObject**>(flagBase) = objects::newObject(flagClass, objects::packageObject(Name("Transient")), Name(), 0);
            }
        }
        if (owner) {
            setOwner(a, owner);
        }
        actor::Instigator(a) = instigator;
        vm::event(a, Name("Spawned"));
        vm::event(a, Name("PreBeginPlay"));
        vm::event(a, Name("BeginPlay"));
        if (a->destroyed || actor::bDeleteMe(a)) {
            return nullptr;
        }
        setLocationZone(a, actor::Location(a));
        actor::LastRenderTime(a) = -10.0f;
        vm::event(a, Name("PostBeginPlay"));
        if (a->destroyed) {
            return nullptr;
        }
        if (!noCollisionFail && encroaches(a, actor::Location(a))) {
            destroy(a);
            return nullptr;
        }
        vm::event(a, Name("SetInitialState"));
        if (a->destroyed) {
            return nullptr;
        }
        if (!actor::Base(a) && actor::bCollideWorld(a) && (a->isA(engineClasses().decoration) || a->isA(engineClasses().inventory) || isPawn(a))) {
            uint8_t physics = actor::Physics(a);
            if (physics == PHYS_None || physics == PHYS_Rotating) {
                findBase(a);
            }
        }
        vm::event(a, Name("PostPostBeginPlay"));
        if (a->destroyed) {
            return nullptr;
        }
        if (inTick) {
            newlySpawned.push_back(a);
        }
        return a;
    }

    bool World::destroy(UObject* a)
    {
        if (!a) {
            return false;
        }
        if (actor::bStatic(a) || actor::bNoDelete(a)) {
            return false;
        }
        if (actor::bDeleteMe(a) || a->destroyed) {
            return true;
        }
        if (a->stateFrame && a->stateFrame->state) {
            vm::event(a, Name("EndState"));
            if (a->destroyed) {
                return true;
            }
        }
        if (actor::Base(a)) {
            setBase(a, nullptr, true);
        }
        if (actor::StandingCount(a) > 0) {
            for (UObject* other : actors) {
                if (other && actor::Base(other) == a) {
                    setBase(other, nullptr, true);
                }
            }
        }
        collisionHash.remove(a);
        vm::event(a, Name("Destroyed"));
        if (a->destroyed) {
            return true;
        }
        for (UObject* other : actors) {
            if (!other || other == a || other->destroyed) {
                continue;
            }
            if (actor::Owner(other) == a) {
                setOwner(other, nullptr);
            } else {
                for (int i = 0; i < 4; i++) {
                    if (actor::Touching(other, i) == a) {
                        endTouch(a, other, true);
                    }
                }
            }
        }
        if (actor::Owner(a)) {
            vm::Call(actor::Owner(a), Name("LostChild")).obj(a).run();
        }
        auto it = std::find(actors.begin(), actors.end(), a);
        if (it != actors.end()) {
            *it = nullptr;
        }
        actorsVersion++;
        actor::bDeleteMe(a) = 1;
        if (UObject* iterator = actor::RenderInterface(a)) {
            actor::RenderInterface(a) = nullptr;
            if (!iterator->destroyed) {
                objects::destroyObject(iterator);
            }
        }
        a->destroyed = true;
        collisionHash.remove(a);
        audio::actorDestroyed(a);
        if (localPlayer == a) {
            localPlayer = nullptr;
        }
        destroyedActors.push_back(a);
        pendingCleanup = true;
        return true;
    }

    std::shared_ptr<const std::vector<UObject*>> World::liveActors()
    {
        uint64_t destroys = objects::destroyCount();
        if (!liveActorsCache || liveActorsVersion != actorsVersion || liveActorsDestroys != destroys) {
            auto list = std::make_shared<std::vector<UObject*>>();
            list->reserve(actors.size());
            for (UObject* a : actors) {
                if (a && !a->destroyed) {
                    list->push_back(a);
                }
            }
            liveActorsCache = list;
            liveActorsVersion = actorsVersion;
            liveActorsDestroys = destroys;
        }
        return liveActorsCache;
    }

    void World::cleanupDestroyed()
    {
        if (!pendingCleanup) {
            return;
        }
        pendingCleanup = false;
        UEventManager* manager = levelInfo ? cast<UEventManager>(levelinfo::EventManager(levelInfo)) : nullptr;
        for (UObject* a : destroyedActors) {
            if (manager) {
                manager->destroyActor(a);
            }
        }
        destroyedActors.clear();
        auto scrub = [](UObject* object) {
            if (!object || object->destroyed || !object->data || !object->cls) {
                return;
            }
            for (size_t offset : objectReferences(object->cls)) {
                UObject*& reference = *reinterpret_cast<UObject**>(object->data + offset);
                if (reference && reference->destroyed) {
                    reference = nullptr;
                }
            }
        };
        for (UObject* a : actors) {
            scrub(a);
        }
        const std::vector<UObject*>& all = objects::all();
        for (size_t i = runtimeStart; i < all.size(); i++) {
            scrub(all[i]);
        }
        actors.erase(std::remove(actors.begin() + std::min<ptrdiff_t>(2, static_cast<ptrdiff_t>(actors.size())), actors.end(), nullptr), actors.end());
        actorsVersion++;
        if (level) {
            level->actors = actors;
        }
    }

    bool World::inStasis(UObject* a)
    {
        if (timeSeconds - actor::LastRenderTime(a) <= 5.0f || !actor::bStasis(a)) {
            return false;
        }
        uint8_t physics = actor::Physics(a);
        if (!actor::bForceStasis(a) && physics != PHYS_None && physics != PHYS_Rotating) {
            return false;
        }
        uint8_t zone = region(a).zoneNumber;
        float zoneRenderTime = level && level->model && zone < level->model->zones.size() ? level->model->zones[zone].lastRenderTime : 0.0f;
        return timeSeconds - zoneRenderTime > 5.0f || actor::DistanceFromPlayer(a) > 1200.0f;
    }

    bool World::tickActor(UObject* a, float deltaSeconds)
    {
        if (!a || a->destroyed) {
            return true;
        }
        UObject* owner = actor::Owner(a);
        if (owner && !owner->destroyed && actor::bTicked(owner) != tickParity) {
            newlySpawned.push_back(a);
            return false;
        }
        actor::bTicked(a) = tickParity;
        static const Name nameScriptedPawn("ScriptedPawn");
        if (a->isA(nameScriptedPawn) && !deusex::scriptedPawnTick(*this, a, deltaSeconds)) {
            return true;
        }
        if (inStasis(a)) {
            if (actor::bTransient(a) && actor::Role(a) >= 2) {
                destroy(a);
            }
            return true;
        }

        updateAnimation(a, deltaSeconds);
        if (a->destroyed) {
            return true;
        }

        bool isPlayer = playerPawnOf(a) != nullptr;
        if (!isPlayer) {
            if (levelinfo::bPlayersOnly(levelInfo)) {
                return true;
            }
            static const Name nameTick("Tick");
            vm::Call tick(a, nameTick);
            if (tick.valid()) {
                tick.f(deltaSeconds).run();
            }
        } else {
            if (inputHook) {
                inputHook(a, deltaSeconds);
            }
            static const Name namePlayerInput("PlayerInput");
            vm::Call playerInput(a, namePlayerInput);
            if (playerInput.valid()) {
                playerInput.f(deltaSeconds).run();
            }
            static const Name namePlayerTick("PlayerTick");
            vm::Call playerTick(a, namePlayerTick);
            if (playerTick.valid()) {
                playerTick.f(deltaSeconds).run();
            }
            if (inputHook) {
                inputHook(a, -1.0f);
            }
        }
        if (a->destroyed) {
            return true;
        }

        latent::poll(a, deltaSeconds);
        vm::runStateCode(a);
        if (a->destroyed) {
            return true;
        }

        float timerRate = actor::TimerRate(a);
        if (timerRate > 0) {
            float& counter = actor::TimerCounter(a);
            counter += deltaSeconds;
            if (counter >= timerRate) {
                int passed = static_cast<int>(counter / timerRate);
                counter -= timerRate * static_cast<float>(passed);
                if (!actor::bTimerLoop(a)) {
                    actor::TimerRate(a) = 0;
                }
                static const Name nameTimer("Timer");
                vm::event(a, nameTimer);
                if (a->destroyed) {
                    return true;
                }
            }
        }

        float& lifeSpan = actor::LifeSpan(a);
        if (lifeSpan != 0) {
            lifeSpan -= deltaSeconds;
            if (lifeSpan <= 0.0001f) {
                static const Name nameExpired("Expired");
                vm::event(a, nameExpired);
                destroy(a);
                return true;
            }
        }

        if (actor::Physics(a) != PHYS_None) {
            performPhysics(a, deltaSeconds);
        }
        if (a->destroyed) {
            return true;
        }

        if (isPawn(a)) {
            if (pawn::bIsPlayer(a)) {
                static const Name nameUpdateEyeHeight("UpdateEyeHeight");
                vm::Call eyeHeight(a, nameUpdateEyeHeight);
                if (eyeHeight.valid()) {
                    eyeHeight.f(deltaSeconds).run();
                }
            }
            tickPawnSenses(a, deltaSeconds);
            float& painTime = pawn::PainTime(a);
            if (painTime > 0) {
                painTime -= deltaSeconds;
                if (painTime < 0.001f) {
                    painTime = 0;
                    static const Name namePainTimer("PainTimer");
                    vm::event(a, namePainTimer);
                }
            }
            float& speechTime = pawn::SpeechTime(a);
            if (speechTime > 0) {
                speechTime -= deltaSeconds;
                if (speechTime < 0.001f) {
                    speechTime = 0;
                    static const Name nameSpeechTimer("SpeechTimer");
                    vm::event(a, nameSpeechTimer);
                }
            }
        }
        return true;
    }

    void World::tick(float deltaSeconds)
    {
        if (!levelInfo) {
            return;
        }
        vm::resetRunaway();
        float dilation = levelinfo::TimeDilation(levelInfo);
        if (dilation <= 0) {
            dilation = 1;
        }
        float delta = deltaSeconds * dilation;
        timeSeconds += delta;
        levelinfo::TimeSeconds(levelInfo) = timeSeconds;
        std::time_t now = std::time(nullptr);
        std::tm local = *std::localtime(&now);
        levelinfo::Year(levelInfo) = local.tm_year + 1900;
        levelinfo::Month(levelInfo) = local.tm_mon + 1;
        levelinfo::DayOfWeek(levelInfo) = local.tm_wday;
        levelinfo::Day(levelInfo) = local.tm_mday;
        levelinfo::Hour(levelInfo) = local.tm_hour;
        levelinfo::Minute(levelInfo) = local.tm_min;
        levelinfo::Second(levelInfo) = local.tm_sec;
        delta = std::clamp(delta, 0.005f, 0.4f);
        frame++;

        paused = !levelinfo::Pauser(levelInfo).empty();
        collisionHash.sync(actors);
        if (localPlayer && !localPlayer->destroyed) {
            Vector viewLocation = actor::Location(localPlayer);
            for (UObject* a : actors) {
                if (a && !a->destroyed && !actor::bStatic(a)) {
                    actor::DistanceFromPlayer(a) = (actor::Location(a) - viewLocation).size();
                }
            }
        }
        if (!paused) {
            inTick = true;
            newlySpawned.clear();
            int updated = 0;
            for (size_t i = firstDynamicActor; i < actors.size(); i++) {
                UObject* a = actors[i];
                if (!a || a->destroyed) {
                    continue;
                }
                updated += tickActor(a, delta) ? 1 : 0;
            }
            while (!newlySpawned.empty() && updated) {
                std::vector<UObject*> pending;
                pending.swap(newlySpawned);
                updated = 0;
                for (auto it = pending.rbegin(); it != pending.rend(); ++it) {
                    UObject* a = *it;
                    if (!a->destroyed && actor::bTicked(a) != tickParity) {
                        updated += tickActor(a, delta) ? 1 : 0;
                    }
                }
            }
            inTick = false;
            if (!levelinfo::bPlayersOnly(levelInfo)) {
                aiEvents();
            }
        } else {
            for (size_t i = firstDynamicActor; i < actors.size(); i++) {
                UObject* a = actors[i];
                if (!a || a->destroyed) {
                    continue;
                }
                if (playerPawnOf(a)) {
                    if (inputHook) {
                        inputHook(a, delta);
                    }
                    static const Name namePlayerInput("PlayerInput");
                    vm::Call playerInput(a, namePlayerInput);
                    if (playerInput.valid()) {
                        playerInput.f(delta).run();
                    }
                    for (UProperty* property : a->cls->properties) {
                        if ((property->propertyFlags & CPF_Input) && property->type == PropType::Float) {
                            *reinterpret_cast<float*>(a->data + property->offset) = 0;
                        }
                    }
                } else if (actor::bAlwaysTick(a)) {
                    tickActor(a, delta);
                }
            }
        }
        tickParity = tickParity ? 0 : 1;
        cleanupDestroyed();
    }

    bool World::calcView(Vector& location, Rotator& rotation, float& fov)
    {
        UObject* viewer = localPlayer;
        if (!viewer || viewer->destroyed) {
            return false;
        }
        location = actor::Location(viewer);
        rotation = actor::Rotation(viewer);
        fov = pawn::FovAngle(viewer);
        static const Name namePlayerCalcView("PlayerCalcView");
        vm::Call call(viewer, namePlayerCalcView);
        if (call.valid()) {
            call.obj(viewer).v(location).r(rotation).run();
            location = *reinterpret_cast<Vector*>(call.parameter(1));
            rotation = *reinterpret_cast<Rotator*>(call.parameter(2));
        }
        return true;
    }

}
