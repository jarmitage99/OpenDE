#include "core/objects.h"
#include "core/package_writer.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/ai.h"
#include "engine/package.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/savegame.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace {
    const char* MapName = "01_NYC_UNATCOIsland";
    const dx::Name TestEvent("UnitTestAlarm");

    struct Scene {
        std::unique_ptr<dx::World> world;
        dx::UEventManager* manager = nullptr;
        dx::UObject* turret = nullptr;
        dx::UObject* nearSender = nullptr;
        dx::UObject* farSender = nullptr;
    };

    dx::UObject* findActor(dx::World& world, const char* name)
    {
        for (dx::UObject* a : world.actors) {
            if (a && !a->destroyed && a->name == dx::Name(name)) {
                return a;
            }
        }
        return nullptr;
    }

    std::vector<dx::UObject*> findLights(dx::World& world, size_t count)
    {
        std::vector<dx::UObject*> lights;
        for (dx::UObject* a : world.actors) {
            if (a && !a->destroyed && a->cls->name == dx::Name("Light") && lights.size() < count) {
                lights.push_back(a);
            }
        }
        return lights;
    }

    Scene loadScene()
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::objects::unloadPackage(dx::Name(MapName));
        Scene scene;
        scene.world = std::make_unique<dx::World>();
        REQUIRE(scene.world->loadMap(MapName));
        dx::aiTimeBudget = false;
        scene.manager = dx::aiEventManager();
        REQUIRE(scene.manager != nullptr);
        scene.turret = findActor(*scene.world, "AutoTurret0");
        REQUIRE(scene.turret != nullptr);
        std::vector<dx::UObject*> lights = findLights(*scene.world, 2);
        REQUIRE(lights.size() == 2);
        dx::Vector origin = dx::actor::Location(scene.turret);
        scene.nearSender = lights[0];
        scene.farSender = lights[1];
        dx::actor::Location(scene.nearSender) = { origin.x + 100.0f, origin.y, origin.z };
        dx::actor::Location(scene.farSender) = { origin.x + 300.0f, origin.y, origin.z };
        return scene;
    }

    void releaseScene(Scene& scene)
    {
        dx::aiTimeBudget = true;
        scene.world.reset();
        dx::objects::unloadPackage(dx::Name(MapName));
    }

    std::string turretActive(dx::UObject* turret)
    {
        dx::UProperty* property = turret->cls->findProperty(dx::Name("bActive"));
        REQUIRE(property != nullptr);
        return property->toText(turret->data + property->offset);
    }

    void callNative(dx::UObject* self, const char* function, size_t given, const std::function<void(dx::UFunction*, uint8_t*)>& fill)
    {
        dx::UFunction* native = dx::vm::findFunction(self, dx::Name(function));
        REQUIRE(native != nullptr);
        REQUIRE(native->native != nullptr);
        std::vector<uint8_t> locals(native->size + 16);
        native->initValue(locals.data());
        fill(native, locals.data());
        std::vector<uint8_t> code(native->stub.begin(), native->stub.begin() + static_cast<std::ptrdiff_t>(given * 5));
        code.push_back(0x16);
        dx::Frame frame;
        frame.object = self;
        frame.node = native;
        frame.locals = locals.data();
        frame.code = code.data();
        native->native(self, frame, nullptr);
        native->destroyValue(locals.data());
    }

    template <typename T>
    void setParam(dx::UFunction* function, uint8_t* locals, size_t index, T value)
    {
        *reinterpret_cast<T*>(locals + function->params[index]->offset) = value;
    }

    void setCallback(dx::UObject* receiver, dx::Name callback)
    {
        dx::vm::Call call(receiver, dx::vm::findFunction(receiver, dx::Name("AISetEventCallback")));
        REQUIRE(call.valid());
        call.n(TestEvent).n(callback).n(dx::Name()).b(false).b(false).b(false).b(false).run();
    }

    void sendAudio(dx::UObject* sender, const char* function)
    {
        dx::vm::Call call(sender, dx::vm::findFunction(sender, dx::Name(function)));
        REQUIRE(call.valid());
        call.n(TestEvent).byte(1).f(1.0f).f(800.0f).run();
    }

    void endAudio(dx::UObject* sender)
    {
        dx::vm::Call call(sender, dx::vm::findFunction(sender, dx::Name("AIEndEvent")));
        REQUIRE(call.valid());
        call.n(TestEvent).byte(1).run();
    }

    dx::UAIReceiverEvent* receiverOf(dx::UEventManager* manager, dx::Name eventName, dx::UObject* actor)
    {
        return static_cast<dx::UAIReceiverEvent*>(manager->findEvent(1, eventName, actor, nullptr, nullptr));
    }

    dx::UAISenderEvent* senderOf(dx::UEventManager* manager, dx::Name eventName, dx::UObject* actor)
    {
        return static_cast<dx::UAISenderEvent*>(manager->findEvent(0, eventName, actor, nullptr, nullptr));
    }

    void tick(dx::World& world, int frames)
    {
        for (int i = 0; i < frames; i++) {
            world.tick(1.0f / 30.0f);
        }
    }
}

TEST_CASE("AI event names hash with the engine's case folded CRC")
{
    CHECK(dx::aiEventHash(dx::Name("LoudNoise")) == 0x3c293a14u);
    CHECK(dx::aiEventHash(dx::Name("alarm")) == 0x5681c61du);
    CHECK(dx::aiEventHash(dx::Name("Food")) == 0xc2b13543u);
    CHECK(dx::aiEventHash(dx::Name("WeaponDrawn")) == 0xb80e7710u);
}

TEST_CASE("the level's event manager is a real Engine.EventManager created at startup", "[data]")
{
    Scene scene = loadScene();
    CHECK(scene.manager->cls->pathName() == "Engine.EventManager");
    CHECK(scene.manager->outer == scene.world->level->outermost());
    CHECK(scene.manager->level == scene.world->level);
    dx::UAIReceiverEvent* alarm = receiverOf(scene.manager, dx::Name("alarm"), scene.turret);
    REQUIRE(alarm != nullptr);
    CHECK(alarm->cls->pathName() == "Engine.AIReceiverEvent");
    CHECK(alarm->cls->superClass()->name == dx::Name("AIEvent"));
    CHECK(alarm->outer == scene.manager);
    CHECK(alarm->callback == dx::Name("AlarmHeard"));
    CHECK(alarm->scoreCallback.none());
    CHECK(alarm->checkVisibility == 1);
    CHECK(alarm->checkDirection == 1);
    CHECK(alarm->checkCylinder == 0);
    CHECK(alarm->checkLineOfSight == 1);
    releaseScene(scene);
}

TEST_CASE("AI event natives apply the original default arguments", "[data]")
{
    Scene scene = loadScene();
    callNative(scene.turret, "AISetEventCallback", 2, [](dx::UFunction* function, uint8_t* locals) {
        setParam<uint32_t>(function, locals, 0, TestEvent.index());
        setParam<uint32_t>(function, locals, 1, dx::Name("AlarmHeard").index());
    });
    dx::UAIReceiverEvent* receiver = receiverOf(scene.manager, TestEvent, scene.turret);
    REQUIRE(receiver != nullptr);
    CHECK(receiver->callback == dx::Name("AlarmHeard"));
    CHECK(receiver->scoreCallback.none());
    CHECK(receiver->checkVisibility == 1);
    CHECK(receiver->checkDirection == 1);
    CHECK(receiver->checkCylinder == 0);
    CHECK(receiver->checkLineOfSight == 1);

    callNative(scene.nearSender, "AISendEvent", 1, [](dx::UFunction* function, uint8_t* locals) {
        setParam<uint32_t>(function, locals, 0, TestEvent.index());
    });
    dx::UAISenderEvent* visual = senderOf(scene.manager, TestEvent, scene.nearSender);
    REQUIRE(visual != nullptr);
    const dx::AIValues& visualSlot = visual->slots[scene.manager->curSlot];
    CHECK(visualSlot.visual == 1.0f);
    CHECK(visualSlot.volume == 0.0f);
    CHECK(visualSlot.radius == 0.0f);

    callNative(scene.farSender, "AIStartEvent", 2, [](dx::UFunction* function, uint8_t* locals) {
        setParam<uint32_t>(function, locals, 0, TestEvent.index());
        setParam<uint8_t>(function, locals, 1, 1);
    });
    dx::UAISenderEvent* audio = senderOf(scene.manager, TestEvent, scene.farSender);
    REQUIRE(audio != nullptr);
    CHECK(audio->slots[scene.manager->curSlot].volume == 1.0f);
    CHECK(audio->slots[scene.manager->curSlot].radius == 800.0f);
    CHECK(audio->persist.volume == 1.0f);
    CHECK(audio->persist.radius == 800.0f);
    CHECK(audio->persist.visual == 0.0f);
    releaseScene(scene);
}

TEST_CASE("a pulse AI event is delivered on the next tick and only once", "[data]")
{
    Scene scene = loadScene();
    setCallback(scene.turret, dx::Name("AlarmHeard"));
    dx::UAIReceiverEvent* receiver = receiverOf(scene.manager, TestEvent, scene.turret);
    REQUIRE(receiver != nullptr);
    sendAudio(scene.nearSender, "AISendEvent");
    CHECK(receiver->lastBestActor == nullptr);
    CHECK(receiver->state == 0);
    tick(*scene.world, 1);
    CHECK(receiver->state == 2);
    CHECK(receiver->pending == 0);
    CHECK(receiver->active == 0);
    CHECK(receiver->lastBestActor == scene.nearSender);
    CHECK(receiver->params.bestActor == scene.nearSender);
    CHECK(receiver->params.volume == 1.0f);
    CHECK(receiver->lastScore == 10001.0f);
    CHECK(turretActive(scene.turret) == "False");
    tick(*scene.world, 1);
    CHECK(receiver->lastBestActor == nullptr);
    CHECK(receiver->state == 2);
    CHECK(receiver->active == 0);
    releaseScene(scene);
}

TEST_CASE("a started AI event begins on the next tick and ends after AIEndEvent", "[data]")
{
    Scene scene = loadScene();
    setCallback(scene.turret, dx::Name("AlarmHeard"));
    dx::UAIReceiverEvent* receiver = receiverOf(scene.manager, TestEvent, scene.turret);
    REQUIRE(receiver != nullptr);
    REQUIRE(turretActive(scene.turret) == "False");
    sendAudio(scene.nearSender, "AIStartEvent");
    CHECK(turretActive(scene.turret) == "False");
    tick(*scene.world, 1);
    CHECK(turretActive(scene.turret) == "True");
    CHECK(receiver->state == 0);
    CHECK(receiver->active == 1);
    tick(*scene.world, 5);
    CHECK(receiver->state == 0);
    CHECK(receiver->active == 1);
    endAudio(scene.nearSender);
    tick(*scene.world, 1);
    CHECK(receiver->active == 1);
    CHECK(turretActive(scene.turret) == "True");
    tick(*scene.world, 1);
    CHECK(receiver->state == 1);
    CHECK(receiver->active == 0);
    CHECK(receiver->lastBestActor == nullptr);
    CHECK(turretActive(scene.turret) == "False");
    releaseScene(scene);
}

TEST_CASE("a closer AI event sender becomes the best one through ChangeBest", "[data]")
{
    Scene scene = loadScene();
    setCallback(scene.turret, dx::Name("AlarmHeard"));
    dx::UAIReceiverEvent* receiver = receiverOf(scene.manager, TestEvent, scene.turret);
    REQUIRE(receiver != nullptr);
    sendAudio(scene.farSender, "AIStartEvent");
    tick(*scene.world, 1);
    CHECK(receiver->state == 0);
    CHECK(receiver->lastBestActor == scene.farSender);
    CHECK(receiver->lastScore == 90001.0f);
    sendAudio(scene.nearSender, "AIStartEvent");
    tick(*scene.world, 1);
    CHECK(receiver->state == 3);
    CHECK(receiver->active == 1);
    CHECK(receiver->lastBestActor == scene.nearSender);
    CHECK(receiver->lastScore == 10001.0f);
    CHECK(turretActive(scene.turret) == "True");
    releaseScene(scene);
}

TEST_CASE("a pulse AI event sender expires once all 16 slots are empty", "[data]")
{
    Scene scene = loadScene();
    sendAudio(scene.nearSender, "AISendEvent");
    dx::UAISenderEvent* sender = senderOf(scene.manager, TestEvent, scene.nearSender);
    REQUIRE(sender != nullptr);
    tick(*scene.world, 15);
    CHECK(senderOf(scene.manager, TestEvent, scene.nearSender) == sender);
    CHECK(sender->deleted == 0);
    CHECK_FALSE(sender->destroyed);
    tick(*scene.world, 1);
    CHECK(senderOf(scene.manager, TestEvent, scene.nearSender) == nullptr);
    CHECK(sender->destroyed);
    CHECK(scene.manager->deleteCount == 0);
    releaseScene(scene);
}

TEST_CASE("destroying an AI event sender drops it from the manager at actor cleanup", "[data]")
{
    Scene scene = loadScene();
    setCallback(scene.turret, dx::Name("AlarmHeard"));
    dx::UClass* boxClass = dx::objects::findClass("DeusEx.BoxSmall");
    dx::objects::flushLoads();
    REQUIRE(boxClass != nullptr);
    dx::UObject* box = scene.world->spawn(boxClass, nullptr, dx::Name(), dx::actor::Location(scene.nearSender), {}, nullptr, true);
    REQUIRE(box != nullptr);
    sendAudio(box, "AIStartEvent");
    dx::UAIReceiverEvent* receiver = receiverOf(scene.manager, TestEvent, scene.turret);
    dx::UAISenderEvent* sender = senderOf(scene.manager, TestEvent, box);
    REQUIRE(receiver != nullptr);
    REQUIRE(sender != nullptr);
    tick(*scene.world, 1);
    REQUIRE(receiver->lastBestActor == box);
    REQUIRE(turretActive(scene.turret) == "True");
    REQUIRE(scene.world->destroy(box));
    tick(*scene.world, 1);
    CHECK(receiver->lastBestActor == nullptr);
    CHECK(sender->deleted == 1);
    CHECK(turretActive(scene.turret) == "True");
    tick(*scene.world, 1);
    CHECK(receiver->state == 1);
    CHECK(turretActive(scene.turret) == "False");
    CHECK(sender->destroyed);
    releaseScene(scene);
}

TEST_CASE("AI events are not processed while the level is paused or players only", "[data]")
{
    Scene scene = loadScene();
    setCallback(scene.turret, dx::Name("AlarmHeard"));
    sendAudio(scene.nearSender, "AIStartEvent");
    int32_t slot = scene.manager->curSlot;
    dx::levelinfo::bPlayersOnly(scene.world->levelInfo) = 1;
    tick(*scene.world, 3);
    CHECK(scene.manager->curSlot == slot);
    CHECK(turretActive(scene.turret) == "False");
    dx::levelinfo::bPlayersOnly(scene.world->levelInfo) = 0;
    tick(*scene.world, 1);
    CHECK(scene.manager->curSlot == ((slot + 1) & 15));
    CHECK(turretActive(scene.turret) == "True");
    releaseScene(scene);
}

TEST_CASE("an AIReceiverEvent serializes to the original byte layout and loads back", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* transient = dx::objects::packageObject(dx::Name("Transient"));
    dx::UClass* lightClass = dx::objects::findClass("Engine.Light");
    dx::objects::flushLoads();
    REQUIRE(lightClass != nullptr);
    auto* manager = dx::cast<dx::UEventManager>(dx::objects::newObject(dx::objects::intrinsicClass(dx::Name("EventManager")), transient, dx::Name("RoundTripManager"), 0));
    auto* type = dx::cast<dx::UAIEventType>(dx::objects::newObject(dx::objects::intrinsicClass(dx::Name("AIEventType")), manager, dx::Name("RoundTripType"), 0));
    auto* receiver = dx::cast<dx::UAIReceiverEvent>(dx::objects::newObject(dx::objects::intrinsicClass(dx::Name("AIReceiverEvent")), manager, dx::Name("RoundTripReceiver"), 0));
    dx::UObject* light = dx::objects::newObject(lightClass, transient, dx::Name("RoundTripLight"), 0);
    REQUIRE(manager != nullptr);
    REQUIRE(type != nullptr);
    REQUIRE(receiver != nullptr);
    type->eventName = dx::Name("RoundTripEvent");
    type->hash = dx::aiEventHash(type->eventName);
    type->receivers = receiver;
    manager->hash[type->hash & 0xff] = type;
    manager->processHead = receiver;
    receiver->type = type;
    receiver->actor = light;
    receiver->callback = dx::Name("RoundTripCallback");
    receiver->scoreCallback = dx::Name("RoundTripScore");
    receiver->pending = 1;
    receiver->state = 3;
    receiver->checkVisibility = 1;
    receiver->checkDirection = 0;
    receiver->checkCylinder = 1;
    receiver->checkLineOfSight = 0;
    receiver->active = 1;
    receiver->lastScore = 2.5f;
    receiver->lastBestActor = light;
    receiver->lastSlot = 7;
    receiver->procNext = receiver;
    receiver->procPrev = receiver;

    dx::PackageWriter writer(dx::Name("AIRoundTrip"));
    writer.addExport(manager, nullptr);
    writer.addExport(type, manager);
    writer.addExport(receiver, manager);
    writer.addExport(light, nullptr);
    writer.setFilter([](dx::UObject* object) {
        return object->linker != nullptr || dx::cast<dx::UClass>(object) != nullptr || !object->outer;
    });
    writer.setNativeSerializer([](dx::PackageWriter& out, dx::UObject* object, dx::Writer& data) {
        dx::aiWriteNative(out, object, data);
    });
    std::filesystem::path file = std::filesystem::temp_directory_path() / "dx-ai-event-round-trip.dxs";
    std::string error;
    REQUIRE(writer.save(file, &error));

    dx::Linker* linker = dx::objects::loadPackageFile(dx::Name("AIRoundTripPackage"), file, true);
    REQUIRE(linker != nullptr);
    const dx::ExportEntry& entry = linker->file().exportAt(3);
    std::vector<uint8_t> body(linker->file().data().begin() + entry.offset, linker->file().data().begin() + entry.offset + entry.size);
    const std::vector<uint8_t> expected = {
        0x00,
        0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x02, 0x03,
        0x01, 0x00, 0x00, 0x00,
        0x03,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x20, 0x40,
        0x04,
        0x07, 0x00, 0x00, 0x00,
        0x03, 0x03
    };
    CHECK(body == expected);

    auto* loaded = dx::cast<dx::UAIReceiverEvent>(linker->object(3));
    REQUIRE(loaded != nullptr);
    dx::objects::flushLoads();
    CHECK(loaded != receiver);
    CHECK(loaded->type == linker->object(2));
    CHECK(loaded->actor == linker->object(4));
    CHECK(loaded->deleted == 0);
    CHECK(loaded->next == nullptr);
    CHECK(loaded->callback == dx::Name("RoundTripCallback"));
    CHECK(loaded->scoreCallback == dx::Name("RoundTripScore"));
    CHECK(loaded->pending == 1);
    CHECK(loaded->state == 3);
    CHECK(loaded->checkVisibility == 1);
    CHECK(loaded->checkDirection == 0);
    CHECK(loaded->checkCylinder == 1);
    CHECK(loaded->checkLineOfSight == 0);
    CHECK(loaded->active == 1);
    CHECK(loaded->lastScore == 2.5f);
    CHECK(loaded->lastBestActor == linker->object(4));
    CHECK(loaded->lastSlot == 7);
    CHECK(loaded->procNext == loaded);
    CHECK(loaded->procPrev == loaded);
    auto* loadedManager = dx::cast<dx::UEventManager>(linker->object(1));
    REQUIRE(loadedManager != nullptr);
    CHECK(loadedManager->processHead == loaded);
    CHECK(loadedManager->hash[type->hash & 0xff] == linker->object(2));
    dx::objects::unloadPackage(dx::Name("AIRoundTripPackage"));
    std::filesystem::remove(file);
}

TEST_CASE("a saved level restores its AI event manager instead of clearing it", "[data]")
{
    Scene scene = loadScene();
    setCallback(scene.turret, dx::Name("AlarmHeard"));
    sendAudio(scene.nearSender, "AIStartEvent");
    tick(*scene.world, 2);
    size_t receivers = 0;
    for (dx::UAIEventType* bucket : scene.manager->hash) {
        for (dx::UAIEventType* type = bucket; type; type = type->nextType) {
            for (dx::UAIEvent* event = type->receivers; event; event = event->next) {
                receivers++;
            }
        }
    }
    REQUIRE(receivers > 0);
    std::filesystem::path root = std::filesystem::temp_directory_path() / "dx-ai-event-save";
    std::filesystem::remove_all(root);
    dx::savegame::setSaveRoot(root);
    std::string error;
    REQUIRE(dx::savegame::saveLevel(*scene.world, dx::savegame::currentDirectory() / (std::string(MapName) + ".dxs"), true, &error));
    int32_t slot = scene.manager->curSlot;
    releaseScene(scene);

    REQUIRE(dx::savegame::selectLevel(MapName));
    dx::World world;
    REQUIRE(world.loadMap(MapName));
    dx::UEventManager* manager = dx::aiEventManager();
    REQUIRE(manager != nullptr);
    CHECK(manager->linker != nullptr);
    CHECK(manager->level == world.level);
    CHECK(manager->curSlot == slot);
    size_t restored = 0;
    for (dx::UAIEventType* bucket : manager->hash) {
        for (dx::UAIEventType* type = bucket; type; type = type->nextType) {
            for (dx::UAIEvent* event = type->receivers; event; event = event->next) {
                restored++;
            }
        }
    }
    CHECK(restored == receivers);
    dx::UObject* turret = findActor(world, "AutoTurret0");
    REQUIRE(turret != nullptr);
    dx::UAIReceiverEvent* receiver = receiverOf(manager, TestEvent, turret);
    REQUIRE(receiver != nullptr);
    CHECK(receiver->callback == dx::Name("AlarmHeard"));
    CHECK(receiver->active == 1);
    CHECK(receiver->lastBestActor != nullptr);
    dx::objects::unloadPackage(dx::Name(std::string("SaveGame_") + MapName));
    dx::objects::unloadPackage(dx::Name(MapName));
    std::filesystem::remove_all(root);
}
