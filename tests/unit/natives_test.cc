#include "audio/audio.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
    struct IslandWorld {
        dx::World world;

        IslandWorld()
        {
            dx::engine::initialize(DEUSEX_GAME_DATA);
            dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
            REQUIRE(world.loadMap("01_NYC_UNATCOIsland"));
        }

        ~IslandWorld()
        {
            dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
        }
    };

    dx::UObject* spawnClass(dx::World& world, const char* path, const dx::Vector& location)
    {
        dx::UClass* cls = dx::objects::findClass(path);
        dx::objects::flushLoads();
        REQUIRE(cls != nullptr);
        dx::UObject* spawned = world.spawn(cls, nullptr, dx::Name(), location, {}, nullptr, true);
        REQUIRE(spawned != nullptr);
        dx::actor::Location(spawned) = location;
        return spawned;
    }

    std::vector<dx::audio::VoiceStatus> voicesOf(const dx::UObject* actor)
    {
        std::vector<dx::audio::VoiceStatus> result;
        for (const dx::audio::VoiceStatus& voice : dx::audio::voices()) {
            if (voice.actor == actor) {
                result.push_back(voice);
            }
        }
        return result;
    }

    struct Bytecode {
        std::vector<uint8_t> code;

        Bytecode& byte(uint8_t value)
        {
            code.push_back(value);
            return *this;
        }

        Bytecode& word(uint32_t value, int bytes)
        {
            for (int i = 0; i < bytes; i++) {
                code.push_back(static_cast<uint8_t>(value >> (8 * i)));
            }
            return *this;
        }

        Bytecode& native(int32_t index)
        {
            if (index < 256) {
                return byte(static_cast<uint8_t>(index));
            }
            return byte(static_cast<uint8_t>(0x60 + (index >> 8))).byte(static_cast<uint8_t>(index & 255));
        }

        Bytecode& object(const dx::UObject* value)
        {
            return byte(0x20).word(value ? value->handle : 0, 4);
        }

        Bytecode& f(float value)
        {
            uint32_t bits;
            std::memcpy(&bits, &value, 4);
            return byte(0x1E).word(bits, 4);
        }

        Bytecode& v(const dx::Vector& value)
        {
            byte(0x23);
            for (float component : { value.x, value.y, value.z }) {
                uint32_t bits;
                std::memcpy(&bits, &component, 4);
                word(bits, 4);
            }
            return *this;
        }

        Bytecode& member(const dx::UObject* self, const char* name)
        {
            dx::UProperty* property = self->cls->findProperty(dx::Name(name));
            REQUIRE(property != nullptr);
            return byte(0x01).word(property->handle, 4);
        }

        Bytecode& omit()
        {
            return byte(0x0B);
        }

        Bytecode& end()
        {
            return byte(0x16);
        }
    };

    std::vector<dx::UObject*> recorded;
    dx::UObject* recordSpawnClass = nullptr;

    void recordTarget(dx::UObject* self, dx::Frame& stack, void*)
    {
        dx::vm::finish(stack);
        recorded.push_back(dx::actor::Target(self));
        if (recordSpawnClass) {
            dx::UClass* cls = dx::cast<dx::UClass>(recordSpawnClass);
            recordSpawnClass = nullptr;
            dx::World::current->spawn(cls, nullptr, dx::Name(), dx::actor::Location(self), {}, nullptr, true);
        }
    }

    template <typename T>
    T runNative(dx::UObject* self, Bytecode& call)
    {
        dx::UStruct node;
        node.script = call.code;
        node.script.push_back(0x0B);
        dx::Frame frame;
        frame.object = self;
        frame.node = &node;
        frame.code = node.script.data();
        T result {};
        frame.step(self, &result);
        return result;
    }

    std::vector<dx::UObject*> runIterator(dx::UObject* self, Bytecode& call)
    {
        dx::vm::registerNative(4095, recordTarget);
        recorded.clear();
        Bytecode script;
        script.byte(0x2F);
        script.code.insert(script.code.end(), call.code.begin(), call.code.end());
        size_t endAt = script.code.size() + 2 + 2 + 1 + 1;
        script.word(static_cast<uint32_t>(endAt), 2);
        script.native(4095).end().byte(0x31).byte(0x30);
        runNative<uint32_t>(self, script);
        return recorded;
    }

    bool contains(const std::vector<dx::UObject*>& list, const dx::UObject* item)
    {
        return std::find(list.begin(), list.end(), item) != list.end();
    }

    void frob(dx::UObject* target, dx::UObject* frobber)
    {
        dx::vm::Call call(target, dx::Name("Frob"));
        call.obj(frobber).obj(nullptr).run();
    }
}

TEST_CASE("PlaySound defaults to SLOT_Misc and an unset sound radius of 800 is heard within 800 over root 1.3", "[data]")
{
    IslandWorld island;
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector origin = dx::actor::Location(player);
    dx::Vector near = origin + dx::Vector { 690, 0, 0 };
    dx::Vector far = origin + dx::Vector { 712, 0, 0 };
    REQUIRE(island.world.fastLineCheck(far, origin));
    dx::UObject* nearSwitch = spawnClass(island.world, "DeusEx.Switch1", near);
    dx::UObject* farSwitch = spawnClass(island.world, "DeusEx.Switch1", far);
    REQUIRE(dx::actor::TransientSoundRadius(nearSwitch) == 0.0f);
    dx::audio::update(origin, {}, 0.0f);
    frob(nearSwitch, player);
    frob(farSwitch, player);
    std::vector<dx::audio::VoiceStatus> heard = voicesOf(nearSwitch);
    REQUIRE(heard.size() == 1);
    CHECK(heard[0].slot == 1);
    CHECK(voicesOf(farSwitch).empty());
    dx::audio::shutdown();
}

TEST_CASE("an obstructed PlaySound is heard within 0.6 of the hearing range at 0.35 volume", "[data]")
{
    IslandWorld island;
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector origin = dx::actor::Location(player);
    dx::Vector inside = origin + dx::Vector { 0, 0, -530 };
    dx::Vector outside = origin + dx::Vector { 0, 0, -560 };
    REQUIRE_FALSE(island.world.fastLineCheck(inside, origin));
    REQUIRE_FALSE(island.world.fastLineCheck(outside, origin));
    dx::UObject* insideSwitch = spawnClass(island.world, "DeusEx.Switch1", inside);
    dx::UObject* outsideSwitch = spawnClass(island.world, "DeusEx.Switch1", outside);
    dx::UObject* reference = spawnClass(island.world, "DeusEx.Switch1", inside);
    dx::audio::update(origin, {}, 0.0f);
    frob(insideSwitch, player);
    frob(outsideSwitch, player);
    CHECK(voicesOf(outsideSwitch).empty());
    std::vector<dx::audio::VoiceStatus> heard = voicesOf(insideSwitch);
    REQUIRE(heard.size() == 1);
    dx::UObject* sound = dx::objects::loadObject("DeusExSounds.Generic.Switch4ClickOn", nullptr);
    dx::objects::flushLoads();
    REQUIRE(sound != nullptr);
    REQUIRE(dx::audio::playSound(dx::cast<dx::USound>(sound), reference, 1, inside, 0.35f, 800.0f, 1.0f, true) != 0);
    std::vector<dx::audio::VoiceStatus> expected = voicesOf(reference);
    REQUIRE(expected.size() == 1);
    CHECK(heard[0].volume == expected[0].volume);
    CHECK(heard[0].volume > 0);
    dx::audio::shutdown();
}

TEST_CASE("ClientHearSound scales volume and pitch by a hundredth and takes slot and no override from the id", "[data]")
{
    IslandWorld island;
    REQUIRE(dx::audio::initializeOffline(44100));
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector origin = dx::actor::Location(player);
    dx::Vector location = origin + dx::Vector { 300, 0, 0 };
    dx::UObject* source = spawnClass(island.world, "DeusEx.Switch1", location);
    dx::UObject* reference = spawnClass(island.world, "DeusEx.Switch1", location);
    dx::UObject* sound = dx::objects::loadObject("DeusExSounds.Generic.Switch4ClickOn", nullptr);
    dx::objects::flushLoads();
    REQUIRE(sound != nullptr);
    dx::audio::update(origin, {}, 0.0f);
    const int32_t id = 7 * 16 + 3 * 2;
    {
        dx::vm::Call call(player, dx::Name("ClientHearSound"));
        call.obj(source).i(id).obj(sound).v(location).v({ 50, 500, 150 }).run();
    }
    REQUIRE(dx::audio::playSound(dx::cast<dx::USound>(sound), reference, 3, location, 0.5f, 500.0f, 1.5f, true) != 0);
    std::vector<dx::audio::VoiceStatus> heard = voicesOf(source);
    std::vector<dx::audio::VoiceStatus> expected = voicesOf(reference);
    REQUIRE(heard.size() == 1);
    REQUIRE(expected.size() == 1);
    CHECK(heard[0].slot == 3);
    CHECK(heard[0].volume == expected[0].volume);
    CHECK(heard[0].step == expected[0].step);
    {
        dx::vm::Call call(player, dx::Name("ClientHearSound"));
        call.obj(source).i(id + 1).obj(sound).v(location).v({ 50, 500, 150 }).run();
    }
    CHECK(voicesOf(source).size() == 1);
    dx::audio::shutdown();
}

TEST_CASE("RandomBiasedRotation keeps the central rotation at full bias and spreads pitch over a quarter turn without masking", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    auto biased = [&](int32_t yaw, float yawSpread, int32_t pitch, float pitchSpread) {
        dx::vm::Call call(player, dx::Name("RandomBiasedRotation"));
        call.i(yaw).f(yawSpread).i(pitch).f(pitchSpread).run();
        return *reinterpret_cast<dx::Rotator*>(call.returnValue());
    };
    for (int i = 0; i < 20; i++) {
        dx::Rotator exact = biased(70000, 1.0f, -3000, 1.0f);
        CHECK(exact.yaw == 70000);
        CHECK(exact.pitch == -3000);
        CHECK(exact.roll == 0);
    }
    int32_t widestPitch = 0;
    int32_t widestYaw = 0;
    for (int i = 0; i < 400; i++) {
        dx::Rotator spread = biased(0, 0.0f, 0, 0.0f);
        CHECK(spread.pitch >= -16384);
        CHECK(spread.pitch <= 16383);
        CHECK(spread.yaw >= -32768);
        CHECK(spread.yaw <= 32767);
        widestPitch = std::max(widestPitch, std::abs(spread.pitch));
        widestYaw = std::max(widestYaw, std::abs(spread.yaw));
    }
    CHECK(widestPitch > 12000);
    CHECK(widestYaw > 24000);
}

TEST_CASE("RadiusActors adds the collision radius and ChildActors walks the whole owner chain including itself", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector origin = dx::actor::Location(player) + dx::Vector { 0, 0, 2000 };
    dx::UObject* center = spawnClass(island.world, "DeusEx.Switch1", origin);
    dx::UObject* inside = spawnClass(island.world, "DeusEx.Switch1", origin + dx::Vector { 125, 0, 0 });
    dx::UObject* outside = spawnClass(island.world, "DeusEx.Switch1", origin + dx::Vector { 0, 135, 0 });
    dx::actor::CollisionRadius(inside) = 30;
    dx::actor::CollisionRadius(outside) = 30;
    dx::UClass* switchClass = center->cls;
    Bytecode radius;
    radius.native(310).object(switchClass).member(center, "Target").f(100.0f).end();
    std::vector<dx::UObject*> found = runIterator(center, radius);
    CHECK(contains(found, center));
    CHECK(contains(found, inside));
    CHECK_FALSE(contains(found, outside));

    dx::actor::Owner(inside) = center;
    dx::actor::Owner(outside) = inside;
    Bytecode children;
    children.native(305).object(switchClass).member(center, "Target").end();
    std::vector<dx::UObject*> owned = runIterator(center, children);
    REQUIRE(owned.size() == 3);
    CHECK(contains(owned, center));
    CHECK(contains(owned, inside));
    CHECK(contains(owned, outside));
}

TEST_CASE("VisibleActors includes the caller and VisibleCollidingActors limits an unset radius to 1000", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector origin = dx::actor::Location(player);
    dx::Vector nearLocation = origin + dx::Vector { 900, 0, 0 };
    dx::Vector farLocation = origin + dx::Vector { 1100, 0, 0 };
    REQUIRE(island.world.fastLineCheck(farLocation, origin));
    dx::UObject* center = spawnClass(island.world, "DeusEx.Switch1", origin);
    dx::UObject* nearSwitch = spawnClass(island.world, "DeusEx.Switch1", nearLocation);
    dx::UObject* farSwitch = spawnClass(island.world, "DeusEx.Switch1", farLocation);
    for (dx::UObject* a : { center, nearSwitch, farSwitch }) {
        REQUIRE(dx::actor::bCollideActors(a) != 0);
        island.world.collisionHash.update(a);
    }
    Bytecode visible;
    visible.native(311).object(center->cls).member(center, "Target").f(100.0f).end();
    CHECK(contains(runIterator(center, visible), center));

    Bytecode colliding;
    colliding.native(312).object(center->cls).member(center, "Target").end();
    std::vector<dx::UObject*> found = runIterator(center, colliding);
    CHECK(contains(found, center));
    CHECK(contains(found, nearSwitch));
    CHECK_FALSE(contains(found, farSwitch));
}

TEST_CASE("AllActors visits actors spawned inside the loop and CycleActors wraps to the start of the actor list", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::UObject* first = spawnClass(island.world, "DeusEx.Switch1", dx::actor::Location(player) + dx::Vector { 0, 0, 2000 });
    size_t before = 0;
    for (dx::UObject* a : island.world.actors) {
        if (a && !a->destroyed && a->cls == first->cls) {
            before++;
        }
    }
    recordSpawnClass = first->cls;
    Bytecode all;
    all.native(304).object(first->cls).member(first, "Target").end();
    CHECK(runIterator(first, all).size() == before + 1);

    dx::UObject* levelInfoClass = dx::engineClasses().levelInfo;
    dx::UProperty* hitPoints = first->cls->findProperty(dx::Name("HitPoints"));
    REQUIRE(hitPoints != nullptr);
    int32_t& index = *reinterpret_cast<int32_t*>(first->data + hitPoints->offset);
    const int32_t last = static_cast<int32_t>(island.world.actors.size()) - 1;
    index = last;
    Bytecode cycle;
    cycle.native(1002).object(levelInfoClass).member(first, "Target").member(first, "HitPoints").end();
    std::vector<dx::UObject*> cycled = runIterator(first, cycle);
    REQUIRE(cycled.size() == 1);
    CHECK(cycled[0] == island.world.levelInfo);
    CHECK(index == last);
}

TEST_CASE("Trace defaults to tracing actors when the caller collides and a miss reports a zero hit location", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector origin = dx::actor::Location(player);
    dx::Vector end = origin + dx::Vector { 400, 0, 0 };
    REQUIRE(island.world.fastLineCheck(end, origin));
    dx::UObject* tracer = spawnClass(island.world, "DeusEx.Switch1", origin);
    REQUIRE(dx::actor::bCollideActors(tracer) != 0);
    Bytecode miss;
    miss.native(277).member(tracer, "Velocity").member(tracer, "Acceleration").v(end).end();
    dx::actor::Velocity(tracer) = { 5, 5, 5 };
    CHECK(runNative<dx::UObject*>(tracer, miss) == nullptr);
    CHECK(dx::actor::Velocity(tracer).x == 0.0f);
    CHECK(dx::actor::Velocity(tracer).y == 0.0f);
    CHECK(dx::actor::Velocity(tracer).z == 0.0f);

    dx::UObject* crate = spawnClass(island.world, "DeusEx.CrateUnbreakableSmall", origin + dx::Vector { 200, 0, 0 });
    island.world.collisionHash.update(crate);
    Bytecode hit;
    hit.native(277).member(tracer, "Velocity").member(tracer, "Acceleration").v(end).end();
    CHECK(runNative<dx::UObject*>(tracer, hit) == crate);
}

TEST_CASE("TraceActors and TraceTexture ignore the class argument and report level hits", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector origin = dx::actor::Location(player);
    dx::Vector below = origin + dx::Vector { 0, 0, -400 };
    REQUIRE_FALSE(island.world.fastLineCheck(below, origin));
    dx::UObject* tracer = spawnClass(island.world, "DeusEx.Switch1", origin + dx::Vector { 0, 0, 2000 });
    dx::UObject* pawnClass = dx::engineClasses().pawn;
    Bytecode traceActors;
    traceActors.native(309).object(pawnClass).member(tracer, "Target").member(tracer, "Velocity").member(tracer, "Acceleration").v(below).v(origin).end();
    CHECK(contains(runIterator(tracer, traceActors), island.world.levelInfo));
}

TEST_CASE("colour operators wrap each channel to a byte and keep the alpha of the colour operand", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    auto colorOp = [&](const char* name, auto&& fill) {
        dx::vm::Call call(player, dx::Name(name));
        REQUIRE(call.valid());
        fill(call);
        call.run();
        return *reinterpret_cast<dx::Color*>(call.returnValue());
    };
    auto setColor = [](dx::vm::Call& call, size_t parameter, dx::Color value) {
        *reinterpret_cast<dx::Color*>(call.parameter(parameter)) = value;
    };
    dx::Color scaled = colorOp("Multiply_ColorFloat", [&](dx::vm::Call& call) {
        setColor(call, 0, { 200, 100, 50, 77 });
        *reinterpret_cast<float*>(call.parameter(1)) = 2.0f;
    });
    CHECK(scaled.r == 144);
    CHECK(scaled.g == 200);
    CHECK(scaled.b == 100);
    CHECK(scaled.a == 77);
    dx::Color negative = colorOp("Multiply_FloatColor", [&](dx::vm::Call& call) {
        *reinterpret_cast<float*>(call.parameter(0)) = -1.0f;
        setColor(call, 1, { 10, 10, 10, 5 });
    });
    CHECK(negative.r == 246);
    CHECK(negative.a == 5);
    dx::Color sum = colorOp("Add_ColorColor", [&](dx::vm::Call& call) {
        setColor(call, 0, { 200, 10, 0, 100 });
        setColor(call, 1, { 100, 10, 0, 200 });
    });
    CHECK(sum.r == 44);
    CHECK(sum.g == 20);
    CHECK(sum.a == 100);
    dx::Color difference = colorOp("Subtract_ColorColor", [&](dx::vm::Call& call) {
        setColor(call, 0, { 10, 20, 30, 40 });
        setColor(call, 1, { 20, 10, 0, 50 });
    });
    CHECK(difference.r == 246);
    CHECK(difference.g == 10);
    CHECK(difference.b == 30);
    CHECK(difference.a == 40);
}

TEST_CASE("GetMeshTexture prefers MultiSkins, then mesh textures above slot zero, then Skin", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    REQUIRE(dx::actor::Mesh(player) != nullptr);
    dx::UObject* skin = nullptr;
    dx::UObject* multi = nullptr;
    for (const dx::BspSurf& surf : island.world.level->model->surfs) {
        if (surf.texture && !skin) {
            skin = surf.texture;
        } else if (surf.texture && surf.texture != skin && !multi) {
            multi = surf.texture;
        }
    }
    REQUIRE(skin != nullptr);
    REQUIRE(multi != nullptr);
    for (int i = 0; i < 8; i++) {
        dx::actor::MultiSkins(player, i) = nullptr;
    }
    dx::actor::Skin(player) = skin;
    dx::actor::MultiSkins(player, 1) = multi;
    auto meshTexture = [&](int32_t index) {
        dx::vm::Call call(player, dx::Name("GetMeshTexture"));
        call.i(index).run();
        return *reinterpret_cast<dx::UObject**>(call.returnValue());
    };
    CHECK(meshTexture(1) == multi);
    CHECK(meshTexture(0) == skin);
}

TEST_CASE("MoveSmooth clears bJustTeleported and Spawn without a tag leaves Tag None", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::actor::bJustTeleported(player) = 1;
    dx::vm::Call move(player, dx::Name("MoveSmooth"));
    move.v({ 0, 0, 0 }).run();
    CHECK(dx::actor::bJustTeleported(player) == 0);

    dx::UClass* switchClass = dx::objects::findClass("DeusEx.Switch1");
    dx::objects::flushLoads();
    Bytecode spawn;
    spawn.native(278).object(switchClass).object(nullptr).omit().v(dx::actor::Location(player) + dx::Vector { 0, 0, 2000 }).end();
    dx::UObject* spawned = runNative<dx::UObject*>(player, spawn);
    REQUIRE(spawned != nullptr);
    CHECK(dx::actor::Tag(spawned).none());
}

TEST_CASE("MakeNoise needs an instigator, remembers the noise on it and sets the stimulus of pawns that hear it", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::UObject* listener = nullptr;
    for (dx::UObject* p = dx::levelinfo::PawnList(island.world.levelInfo); p; p = dx::pawn::nextPawn(p)) {
        if (p != player && !dx::isPlayerPawn(p) && dx::vm::isProbing(p, dx::Name("HearNoise"))) {
            listener = p;
            break;
        }
    }
    REQUIRE(listener != nullptr);
    dx::Vector origin = dx::actor::Location(player);
    dx::Vector listenAt = origin + dx::Vector { 300, 0, 0 };
    REQUIRE(island.world.fastLineCheck(listenAt, origin));
    dx::actor::Location(listener) = listenAt;
    dx::pawn::Alertness(listener) = 0.4f;
    dx::pawn::HearingThreshold(listener) = 0.1f;
    dx::pawn::Stimulus(listener) = -1.0f;
    dx::UObject* silent = spawnClass(island.world, "DeusEx.Switch1", origin);
    dx::actor::Instigator(silent) = nullptr;
    dx::vm::Call(silent, dx::Name("MakeNoise")).f(1.0f).run();
    CHECK(dx::pawn::Stimulus(listener) == -1.0f);

    dx::UObject* noisy = spawnClass(island.world, "DeusEx.Switch1", origin);
    dx::actor::Instigator(noisy) = player;
    dx::pawn::noise1time(player) = -10.0f;
    dx::pawn::noise2time(player) = -10.0f;
    dx::vm::Call(noisy, dx::Name("MakeNoise")).f(1.0f).run();
    CHECK(dx::pawn::Stimulus(listener) == Catch::Approx(2.2f));
    CHECK(dx::pawn::noise1time(player) == island.world.timeSeconds);
    CHECK(dx::pawn::noise1loudness(player) == 1.0f);
    CHECK(dx::pawn::noise1spot(player).x == origin.x);

    dx::pawn::Stimulus(listener) = -1.0f;
    dx::vm::Call(noisy, dx::Name("MakeNoise")).f(1.0f).run();
    CHECK(dx::pawn::Stimulus(listener) == -1.0f);
}

TEST_CASE("FastTrace only tests world geometry and ignores movers", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector start;
    dx::Vector end;
    bool found = false;
    for (dx::UObject* a : island.world.actors) {
        if (found || !a || a->destroyed || !dx::isMover(a)) {
            continue;
        }
        dx::Vector center = dx::actor::Location(a);
        for (const dx::Vector& axis : { dx::Vector { 96, 0, 0 }, dx::Vector { 0, 96, 0 } }) {
            for (float lift : { 0.0f, 32.0f, 64.0f }) {
                dx::Vector from = center - axis + dx::Vector { 0, 0, lift };
                dx::Vector to = center + axis + dx::Vector { 0, 0, lift };
                dx::CheckResult hit;
                if (!found && island.world.fastLineCheck(to, from) && island.world.lineCheck(hit, from, to, {}, nullptr, dx::TRACE_World) && hit.actor == a) {
                    start = from;
                    end = to;
                    found = true;
                }
            }
        }
    }
    REQUIRE(found);
    dx::vm::Call call(player, dx::Name("FastTrace"));
    call.v(end).v(start).run();
    CHECK(*reinterpret_cast<uint32_t*>(call.returnValue()) != 0);
}

TEST_CASE("travelling twice keeps a single NanoKeyRing in the inventory chain", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    const char* maps[] = { "01_NYC_UNATCOIsland", "01_NYC_UNATCOHQ", "01_NYC_UNATCOIsland" };
    for (const char* map : maps) {
        dx::objects::unloadPackage(dx::Name(map));
    }
    std::vector<dx::TravelActor> travel;
    std::vector<int> keyRings;
    std::vector<int> pistols;
    for (const char* map : maps) {
        dx::World world;
        world.incomingTravel = std::move(travel);
        REQUIRE(world.loadMap(map));
        for (int i = 0; i < 30; i++) {
            world.tick(1.0f / 30.0f);
        }
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        if (keyRings.empty()) {
            dx::UClass* pistolClass = dx::objects::findClass("DeusEx.WeaponPistol");
            dx::objects::flushLoads();
            dx::UObject* pistol = world.spawn(pistolClass, nullptr, dx::Name(), dx::actor::Location(player), {}, nullptr, true);
            REQUIRE(pistol != nullptr);
            dx::vm::Call give(pistol, dx::Name("GiveTo"));
            give.obj(player).run();
        }
        int rings = 0;
        int guns = 0;
        int guard = 0;
        for (dx::UObject* item = dx::actor::Inventory(player); item && guard < 64; item = dx::actor::Inventory(item), guard++) {
            rings += item->isA(dx::Name("NanoKeyRing")) ? 1 : 0;
            guns += item->isA(dx::Name("WeaponPistol")) ? 1 : 0;
        }
        keyRings.push_back(rings);
        pistols.push_back(guns);
        travel = world.collectTravel();
        dx::objects::unloadPackage(dx::Name(map));
    }
    dx::World::releaseTravel(travel);
    CHECK(keyRings == std::vector<int> { 1, 1, 1 });
    CHECK(pistols == std::vector<int> { 1, 1, 1 });
}

TEST_CASE("PlayerCanSeeMe needs the actor within range and inside the player's view cone", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector origin = dx::actor::Location(player);
    REQUIRE(island.world.fastLineCheck(origin + dx::Vector { 1300, 0, 0 }, origin));
    dx::pawn::ViewRotation(player) = {};
    dx::pawn::bBehindView(player) = 0;
    auto seen = [&](const dx::Vector& offset) {
        dx::UObject* target = spawnClass(island.world, "DeusEx.Switch1", origin + offset);
        dx::actor::CollisionRadius(target) = 10;
        dx::vm::Call call(target, dx::Name("PlayerCanSeeMe"));
        call.run();
        return *reinterpret_cast<uint32_t*>(call.returnValue()) != 0;
    };
    CHECK(seen({ 300, 0, 0 }));
    CHECK_FALSE(seen({ 1200, 0, 0 }));
    CHECK_FALSE(seen({ 0, 200, 0 }));
}

TEST_CASE("GetBoundingBox applies the brush PostScale and pads plain actors by one unit", "[data]")
{
    IslandWorld island;
    dx::UObject* mover = nullptr;
    for (dx::UObject* a : island.world.actors) {
        if (!mover && a && !a->destroyed && dx::isMover(a)) {
            mover = a;
        }
    }
    REQUIRE(mover != nullptr);
    auto box = [&](dx::UObject* actor) {
        dx::vm::Call call(actor, dx::Name("GetBoundingBox"));
        call.skip().skip().b(false).v(dx::actor::Location(actor)).r({});
        call.run();
        dx::Vector minimum = *reinterpret_cast<dx::Vector*>(call.parameter(0));
        dx::Vector maximum = *reinterpret_cast<dx::Vector*>(call.parameter(1));
        return std::make_pair(minimum, maximum);
    };
    dx::UProperty* postScale = mover->cls->findProperty(dx::Name("PostScale"));
    REQUIRE(postScale != nullptr);
    dx::Vector& scale = *reinterpret_cast<dx::Vector*>(mover->data + postScale->offset + postScale->structType->findProperty(dx::Name("Scale"))->offset);
    REQUIRE(scale.x == 1.0f);
    auto plain = box(mover);
    scale.x = 2.0f;
    auto stretched = box(mover);
    float plainWidth = plain.second.x - plain.first.x;
    REQUIRE(plainWidth > 1.0f);
    CHECK(stretched.second.x - stretched.first.x == Catch::Approx(plainWidth * 2.0f).epsilon(1e-4));
    CHECK(stretched.second.y - stretched.first.y == Catch::Approx(plain.second.y - plain.first.y).epsilon(1e-4));

    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::UObject* info = spawnClass(island.world, "Engine.Trigger", dx::actor::Location(player) + dx::Vector { 0, 0, 2000 });
    REQUIRE(dx::actor::Mesh(info) == nullptr);
    REQUIRE(dx::actor::Brush(info) == nullptr);
    dx::actor::CollisionRadius(info) = 20;
    dx::actor::CollisionHeight(info) = 30;
    auto padded = box(info);
    CHECK(padded.second.x - padded.first.x == 42.0f);
    CHECK(padded.second.z - padded.first.z == 62.0f);
}

TEST_CASE("ParabolicTrace fails back to the start when too fast or out of time and keeps bouncing when asked", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::Vector start = dx::actor::Location(player) + dx::Vector { 0, 0, 300 };
    dx::UObject* thrown = spawnClass(island.world, "DeusEx.Switch1", start);
    dx::actor::bBounce(thrown) = 0;
    dx::actor::Velocity(thrown) = {};
    dx::Vector cylinder { 10, 10, 10 };
    auto trace = [&](const dx::Vector& velocity, bool bounce, float maxTime, float elasticity = 0.9f) {
        Bytecode call;
        call.native(722).member(thrown, "Acceleration").v(velocity).v(start).byte(0x28).v(cylinder).f(maxTime).f(elasticity).byte(bounce ? 0x27 : 0x28).f(60.0f).f(0.05f).end();
        float time = runNative<float>(thrown, call);
        return std::make_pair(time, dx::actor::Acceleration(thrown));
    };
    auto fast = trace({ 40000, 0, 0 }, false, 5.0f);
    CHECK(fast.first == 0.0f);
    CHECK(fast.second.x == start.x);
    CHECK(fast.second.z == start.z);
    auto late = trace({}, false, 0.2f);
    CHECK(late.first == 0.0f);
    CHECK(late.second.z == start.z);
    auto landed = trace({}, false, 5.0f);
    REQUIRE(landed.first > 0.2f);
    CHECK(landed.second.z < start.z - 250.0f);
    auto stopped = trace({ 300, 0, -600 }, false, 5.0f);
    auto damped = trace({ 300, 0, -600 }, true, 5.0f, 0.3f);
    CHECK(stopped.first > 0.0f);
    CHECK(damped.first > stopped.first);
    CHECK(damped.second.x > stopped.second.x);
    CHECK(trace({ 300, 0, -600 }, true, 5.0f).first == 0.0f);
}

TEST_CASE("Canvas natives follow the original cursor, size and hiding rules", "[data]")
{
    IslandWorld island;
    dx::UClass* canvasClass = dx::objects::findClass("Engine.Canvas");
    dx::UObject* font = dx::objects::loadObject("Engine.SmallFont", nullptr);
    dx::objects::flushLoads();
    REQUIRE(canvasClass != nullptr);
    REQUIRE(dx::cast<dx::UFont>(font) != nullptr);
    dx::UObject* canvas = dx::objects::newObject(canvasClass, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
    dx::canvas::Font(canvas) = font;
    dx::canvas::ClipX(canvas) = 640;
    dx::canvas::ClipY(canvas) = 480;
    dx::canvas::SpaceX(canvas) = 3;
    dx::UObject* texture = island.world.level->model->surfs[0].texture;
    REQUIRE(texture != nullptr);

    dx::canvas::Style(canvas) = 0;
    dx::canvas::CurX(canvas) = 5;
    dx::canvas::CurYL(canvas) = 0;
    dx::vm::Call(canvas, dx::Name("DrawText")).s("Hello").b(false).run();
    CHECK(dx::canvas::CurX(canvas) == 5.0f);
    CHECK(dx::canvas::CurYL(canvas) == 0.0f);

    dx::canvas::Style(canvas) = 1;
    dx::vm::Call(canvas, dx::Name("DrawTile")).obj(nullptr).f(32).f(16).f(0).f(0).f(32).f(16).run();
    CHECK(dx::canvas::CurX(canvas) == 5.0f);

    dx::canvas::CurX(canvas) = -10;
    dx::canvas::CurY(canvas) = 4;
    dx::vm::Call(canvas, dx::Name("DrawTileClipped")).obj(texture).f(50).f(20).f(0).f(0).f(50).f(20).run();
    CHECK(dx::canvas::CurX(canvas) == 43.0f);
    CHECK(dx::canvas::CurYL(canvas) == 20.0f);

    auto textSize = [&](const char* text) {
        dx::vm::Call call(canvas, dx::Name("TextSize"));
        call.s(text).f(-1).f(-1).run();
        return std::make_pair(*reinterpret_cast<float*>(call.parameter(1)), *reinterpret_cast<float*>(call.parameter(2)));
    };
    auto empty = textSize("");
    CHECK(empty.first == 0.0f);
    CHECK(empty.second == 0.0f);
    const dx::FontCharacter* a = dx::cast<dx::UFont>(font)->glyph('A', nullptr);
    const dx::FontCharacter* b = dx::cast<dx::UFont>(font)->glyph('B', nullptr);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    auto ab = textSize("AB");
    CHECK(ab.first == static_cast<float>(a->width + b->width));

    dx::canvas::CurX(canvas) = 10.7f;
    dx::canvas::CurY(canvas) = 3.9f;
    dx::vm::Call(canvas, dx::Name("StrLen")).s("Hi").f(0).f(0).run();
    CHECK(dx::canvas::CurX(canvas) == 10.0f);
    CHECK(dx::canvas::CurY(canvas) == 3.0f);

    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::actor::bHidden(player) = 0;
    dx::vm::Call(canvas, dx::Name("DrawActor")).obj(player).b(false).b(false).run();
    CHECK(dx::actor::bHidden(player) != 0);
}

TEST_CASE("ZoneActors includes actors in the level pseudo zone", "[data]")
{
    IslandWorld island;
    dx::UObject* zone = nullptr;
    for (dx::UObject* a : island.world.actors) {
        if (!zone && a && !a->destroyed && a != island.world.levelInfo && a->isA(dx::Name("ZoneInfo"))) {
            zone = a;
        }
    }
    REQUIRE(zone != nullptr);
    dx::UObject* player = island.world.player();
    dx::UObject* loose = spawnClass(island.world, "DeusEx.Switch1", dx::actor::Location(player) + dx::Vector { 0, 0, 2000 });
    dx::region(loose).zone = island.world.levelInfo;
    Bytecode zoneActors;
    zoneActors.native(308).object(loose->cls).member(zone, "Target").end();
    CHECK(contains(runIterator(zone, zoneActors), loose));
}

TEST_CASE("UpdateURL edits the current URL, GetAddressURL reports host and port and ClientTravel needs a Player", "[data]")
{
    IslandWorld island;
    dx::UObject* player = island.world.player();
    REQUIRE(player != nullptr);
    dx::vm::Call(player, dx::Name("UpdateURL")).s("Probe").s("One").b(false).run();
    dx::vm::Call(player, dx::Name("UpdateURL")).s("Probe").s("Two").b(false).run();
    const std::string& url = island.world.url;
    CHECK(url.find("?Probe=Two") != std::string::npos);
    CHECK(url.find("Probe=One") == std::string::npos);
    CHECK(url.find("Probe=") == url.rfind("Probe="));

    dx::vm::Call address(island.world.levelInfo, dx::Name("GetAddressURL"));
    address.run();
    CHECK(*reinterpret_cast<std::string*>(address.returnValue()) == ":7790");

    dx::UObject* viewport = dx::playerpawn::Player(player);
    dx::playerpawn::Player(player) = nullptr;
    dx::vm::Call(player, dx::Name("ClientTravel")).s("02_NYC_BatteryPark").byte(0).b(true).run();
    CHECK_FALSE(island.world.travelPending);
    dx::playerpawn::Player(player) = viewport;
}

TEST_CASE("Warp and UnWarp transform location, velocity and rotation by the warp coordinates", "[data]")
{
    IslandWorld island;
    dx::UClass* warpClass = dx::objects::findClass("Engine.WarpZoneInfo");
    dx::objects::flushLoads();
    REQUIRE(warpClass != nullptr);
    dx::UObject* warp = dx::objects::newObject(warpClass, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
    dx::warpzoneinfo::WarpCoords(warp) = { { 100, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, 0, 1 } };
    dx::actor::Location(warp) = { 10, 0, 0 };
    dx::actor::Velocity(warp) = { 1, 0, 0 };
    dx::actor::Rotation(warp) = {};
    Bytecode warpCall;
    warpCall.native(314).member(warp, "Location").member(warp, "Velocity").member(warp, "Rotation").end();
    runNative<uint32_t>(warp, warpCall);
    CHECK(dx::actor::Location(warp).x == Catch::Approx(100.0f));
    CHECK(dx::actor::Location(warp).y == Catch::Approx(10.0f));
    CHECK(dx::actor::Velocity(warp).x == Catch::Approx(0.0f).margin(1e-5));
    CHECK(dx::actor::Velocity(warp).y == Catch::Approx(1.0f));
    CHECK(std::abs(dx::actor::Rotation(warp).yaw - 16384) <= 4);
    Bytecode unwarpCall;
    unwarpCall.native(315).member(warp, "Location").member(warp, "Velocity").member(warp, "Rotation").end();
    runNative<uint32_t>(warp, unwarpCall);
    CHECK(dx::actor::Location(warp).x == Catch::Approx(10.0f));
    CHECK(dx::actor::Location(warp).y == Catch::Approx(0.0f).margin(1e-4));
    CHECK(dx::actor::Velocity(warp).x == Catch::Approx(1.0f));
    CHECK(std::abs(dx::actor::Rotation(warp).yaw) <= 4);
}
