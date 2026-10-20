#include "core/objects.h"
#include "core/package_writer.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/props_deusex.h"
#include "engine/props_engine.h"
#include "engine/props_extension.h"
#include "engine/resources.h"
#include "engine/savegame.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {
    const char* MapName = "01_NYC_UNATCOIsland";

    std::filesystem::path freshSaveRoot(const char* name)
    {
        std::filesystem::path root = std::filesystem::temp_directory_path() / name;
        std::filesystem::remove_all(root);
        dx::savegame::setSaveRoot(root);
        return root;
    }

    dx::UObject* findActor(dx::World& world, const char* name)
    {
        for (dx::UObject* a : world.actors) {
            if (a && !a->destroyed && a->name == dx::Name(name)) {
                return a;
            }
        }
        return nullptr;
    }

    dx::UObject* findInventory(dx::UObject* owner, const char* className)
    {
        int guard = 0;
        for (dx::UObject* item = dx::actor::Inventory(owner); item && guard < 64; item = dx::actor::Inventory(item), guard++) {
            if (item->isA(dx::Name(className))) {
                return item;
            }
        }
        return nullptr;
    }

    bool flagValue(dx::UObject* player, const char* flag)
    {
        dx::UObject* flags = *reinterpret_cast<dx::UObject**>(player->propertyAddress(dx::Name("FlagBase")));
        dx::vm::Call call(flags, dx::Name("GetBool"));
        call.n(dx::Name(flag));
        call.run();
        return *reinterpret_cast<uint32_t*>(call.returnValue()) != 0;
    }

    void unloadMaps()
    {
        dx::objects::unloadPackage(dx::Name(MapName));
        dx::objects::unloadPackage(dx::Name(std::string("SaveGame_") + MapName));
    }
}

TEST_CASE("saved game restores actor, flag, inventory, reference and state values", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::filesystem::path root = freshSaveRoot("dx-savegame-test-restore");
    unloadMaps();
    {
        dx::World world;
        REQUIRE(world.loadMap(MapName));
        dx::UObject* player = world.player();
        REQUIRE(player != nullptr);
        dx::UObject* terrorist = findActor(world, "Terrorist1");
        REQUIRE(terrorist != nullptr);
        dx::actor::Location(terrorist) = { -1234.0f, 567.0f, -99.0f };
        dx::pawn::Health(terrorist) = 42;
        REQUIRE(dx::vm::gotoState(terrorist, dx::Name("Wandering")));
        dx::pawn::Enemy(terrorist) = player;

        dx::UObject* flags = *reinterpret_cast<dx::UObject**>(player->propertyAddress(dx::Name("FlagBase")));
        REQUIRE(flags != nullptr);
        dx::vm::Call setFlag(flags, dx::Name("SetBool"));
        setFlag.n(dx::Name("SaveTestFlag")).b(true).b(true).i(3);
        setFlag.run();
        REQUIRE(flagValue(player, "SaveTestFlag"));

        dx::UClass* pistolClass = dx::objects::findClass("DeusEx.WeaponPistol");
        dx::objects::flushLoads();
        dx::UObject* pistol = world.spawn(pistolClass, nullptr, dx::Name(), dx::actor::Location(player), {}, nullptr, true);
        REQUIRE(pistol != nullptr);
        dx::vm::Call give(pistol, dx::Name("GiveTo"));
        give.obj(player);
        give.run();
        REQUIRE(findInventory(player, "WeaponPistol") != nullptr);

        REQUIRE(dx::savegame::saveGame(world, 7, "unit test"));
        dx::pawn::Health(terrorist) = 5;
        dx::actor::Location(terrorist) = { 0.0f, 0.0f, 0.0f };
        unloadMaps();
    }
    CHECK(std::filesystem::is_regular_file(root / "Save0007" / "SaveInfo.dxs"));
    CHECK(std::filesystem::is_regular_file(root / "Save0007" / "01_NYC_UNATCOIsland.dxs"));

    std::string map;
    REQUIRE(dx::savegame::prepareLoad(7, map));
    CHECK(map == MapName);
    std::string url = map + "?loadgame=7";
    REQUIRE(dx::savegame::selectLevel(url));
    dx::World loaded;
    REQUIRE(loaded.loadMap(url));
    CHECK(loaded.level->outermost()->name == dx::Name("SaveGame_01_NYC_UNATCOIsland"));
    dx::UObject* player = loaded.player();
    REQUIRE(player != nullptr);
    dx::UObject* terrorist = findActor(loaded, "Terrorist1");
    REQUIRE(terrorist != nullptr);
    dx::Vector location = dx::actor::Location(terrorist);
    CHECK(location.x == -1234.0f);
    CHECK(location.y == 567.0f);
    CHECK(location.z == -99.0f);
    CHECK(dx::pawn::Health(terrorist) == 42);
    CHECK(dx::pawn::Enemy(terrorist) == player);
    CHECK(dx::vm::stateName(terrorist) == "Wandering");
    CHECK(flagValue(player, "SaveTestFlag"));
    CHECK_FALSE(flagValue(player, "SomeOtherFlag"));
    dx::UObject* restoredPistol = findInventory(player, "WeaponPistol");
    REQUIRE(restoredPistol != nullptr);
    CHECK(dx::actor::Owner(restoredPistol) == player);
    CHECK(dx::deusexplayer::saveCount(player) == 1);
    unloadMaps();
    std::filesystem::remove_all(root);
}

TEST_CASE("game directory enumerates save slots with their save info", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::filesystem::path root = freshSaveRoot("dx-savegame-test-directory");
    unloadMaps();
    {
        dx::World world;
        REQUIRE(world.loadMap(MapName));
        REQUIRE(dx::savegame::saveGame(world, 3, "first slot"));
        REQUIRE(dx::savegame::saveGame(world, 0, "second slot"));
        unloadMaps();
    }
    dx::UClass* directoryClass = dx::objects::findClass("DeusEx.GameDirectory");
    dx::objects::flushLoads();
    REQUIRE(directoryClass != nullptr);
    dx::UObject* directory = dx::objects::newObject(directoryClass, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
    {
        dx::vm::Call call(directory, dx::Name("SetDirType"));
        call.byte(1);
        call.run();
    }
    dx::vm::Call(directory, dx::Name("GetGameDirectory")).run();
    dx::vm::Call count(directory, dx::Name("GetDirCount"));
    count.run();
    REQUIRE(*reinterpret_cast<int32_t*>(count.returnValue()) == 2);
    dx::vm::Call filename(directory, dx::Name("GetDirFilename"));
    filename.i(1);
    filename.run();
    CHECK(*reinterpret_cast<std::string*>(filename.returnValue()) == "Save0004");

    dx::vm::Call info(directory, dx::Name("GetSaveInfoFromDirectoryIndex"));
    info.i(0);
    info.run();
    dx::UObject* saveInfo = *reinterpret_cast<dx::UObject**>(info.returnValue());
    REQUIRE(saveInfo != nullptr);
    CHECK(dx::deusexsaveinfo::DirectoryIndex(saveInfo) == 3);
    CHECK(dx::deusexsaveinfo::Description(saveInfo) == "first slot");
    CHECK(dx::deusexsaveinfo::MapName(saveInfo) == "01_NYC_UNATCOIsland");
    CHECK(dx::deusexsaveinfo::MissionLocation(saveInfo) == "NYC - Liberty Island");

    dx::vm::Call quick(directory, dx::Name("GetSaveInfo"));
    quick.i(-1);
    quick.run();
    CHECK(*reinterpret_cast<dx::UObject**>(quick.returnValue()) == nullptr);
    std::filesystem::remove_all(root);
}

TEST_CASE("package writer output is read back by the linker", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UClass* infoClass = dx::objects::findClass("DeusEx.DeusExSaveInfo");
    dx::UClass* flagClass = dx::objects::findClass("Extension.FlagBool");
    dx::UObject* texture = dx::objects::loadObject("CoreTexBrick.Brick.rough_greywall", nullptr);
    dx::objects::flushLoads();
    REQUIRE(infoClass != nullptr);
    REQUIRE(flagClass != nullptr);
    REQUIRE(texture != nullptr);
    dx::UObject* transient = dx::objects::packageObject(dx::Name("Transient"));
    dx::UObject* info = dx::objects::newObject(infoClass, transient, dx::Name("RoundTripInfo"), 0);
    dx::UObject* first = dx::objects::newObject(flagClass, transient, dx::Name("RoundTripFirst"), 0);
    dx::UObject* second = dx::objects::newObject(flagClass, transient, dx::Name("RoundTripSecond"), 0);
    dx::deusexsaveinfo::Description(info) = "round trip";
    dx::deusexsaveinfo::saveCount(info) = 31;
    dx::deusexsaveinfo::bCheatsEnabled(info) = 1;
    dx::deusexsaveinfo::Snapshot(info) = texture;
    dx::flag::FlagName(first) = dx::Name("FirstFlag");
    dx::flag::expiration(first) = 12;
    dx::flag::nextFlag(first) = second;
    dx::flag::FlagName(second) = dx::Name("SecondFlag");

    dx::PackageWriter writer(dx::Name("RoundTrip"));
    writer.addExport(info, nullptr);
    writer.addExport(first, nullptr);
    writer.addExport(second, first);
    writer.setFilter([](dx::UObject* object) {
        return object->linker != nullptr || dx::cast<dx::UClass>(object) != nullptr || !object->outer;
    });
    std::filesystem::path file = std::filesystem::temp_directory_path() / "dx-package-writer-round-trip.dxs";
    std::string error;
    REQUIRE(writer.save(file, &error));

    dx::Linker* linker = dx::objects::loadPackageFile(dx::Name("RoundTripPackage"), file, true);
    REQUIRE(linker != nullptr);
    int32_t infoRef = linker->findExport(dx::Name("RoundTripInfo"), dx::Name("DeusExSaveInfo"), "");
    int32_t firstRef = linker->findExport(dx::Name("RoundTripFirst"), dx::Name("FlagBool"), "");
    int32_t secondRef = linker->findExport(dx::Name("RoundTripSecond"), dx::Name("FlagBool"), "RoundTripFirst");
    REQUIRE(infoRef == 1);
    REQUIRE(firstRef == 2);
    REQUIRE(secondRef == 3);
    dx::UObject* loadedInfo = linker->object(infoRef);
    dx::UObject* loadedFirst = linker->object(firstRef);
    dx::objects::flushLoads();
    REQUIRE(loadedInfo != info);
    CHECK(dx::deusexsaveinfo::Description(loadedInfo) == "round trip");
    CHECK(dx::deusexsaveinfo::saveCount(loadedInfo) == 31);
    CHECK(dx::deusexsaveinfo::bCheatsEnabled(loadedInfo) == 1);
    CHECK(dx::deusexsaveinfo::Snapshot(loadedInfo) == texture);
    CHECK(dx::flag::FlagName(loadedFirst) == dx::Name("FirstFlag"));
    CHECK(dx::flag::expiration(loadedFirst) == 12);
    dx::UObject* loadedSecond = dx::flag::nextFlag(loadedFirst);
    REQUIRE(loadedSecond != nullptr);
    CHECK(loadedSecond->name == dx::Name("RoundTripSecond"));
    CHECK(loadedSecond->outer == loadedFirst);
    CHECK(dx::flag::FlagName(loadedSecond) == dx::Name("SecondFlag"));
    dx::objects::unloadPackage(dx::Name("RoundTripPackage"));
    std::filesystem::remove(file);
}

TEST_CASE("a loaded saved game keeps the touching lists of its actors", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::filesystem::path root = freshSaveRoot("dx-savegame-test-touching");
    unloadMaps();
    std::string toucherName;
    std::string touchedName;
    {
        dx::World world;
        REQUIRE(world.loadMap(MapName));
        for (int i = 0; i < 60; i++) {
            world.tick(1.0f / 30.0f);
        }
        dx::UObject* player = world.player();
        for (dx::UObject* a : world.actors) {
            if (!toucherName.empty() || !a || a->destroyed || a == player) {
                continue;
            }
            for (int i = 0; i < 4; i++) {
                dx::UObject* other = dx::actor::Touching(a, i);
                if (other && other != player && toucherName.empty()) {
                    toucherName = a->name.str();
                    touchedName = other->name.str();
                }
            }
        }
        REQUIRE_FALSE(toucherName.empty());
        REQUIRE(dx::savegame::saveGame(world, 5, "touching"));
        unloadMaps();
    }
    std::string map;
    REQUIRE(dx::savegame::prepareLoad(5, map));
    std::string url = map + "?loadgame=5";
    REQUIRE(dx::savegame::selectLevel(url));
    dx::World loaded;
    REQUIRE(loaded.loadMap(url));
    dx::UObject* toucher = findActor(loaded, toucherName.c_str());
    REQUIRE(toucher != nullptr);
    bool kept = false;
    for (int i = 0; i < 4; i++) {
        dx::UObject* other = dx::actor::Touching(toucher, i);
        kept = kept || (other && other->name.str() == touchedName);
    }
    CHECK(kept);
    unloadMaps();
    std::filesystem::remove_all(root);
}

TEST_CASE("a loaded saved level keeps the options of its saved URL", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::filesystem::path root = freshSaveRoot("dx-savegame-test-url");
    unloadMaps();
    {
        dx::World world;
        REQUIRE(world.loadMap(MapName));
        REQUIRE(world.url == MapName);
        REQUIRE(dx::savegame::saveGame(world, 6, "url"));
        unloadMaps();
    }
    std::string map;
    REQUIRE(dx::savegame::prepareLoad(6, map));
    std::string url = map + "?loadgame=6";
    REQUIRE(dx::savegame::selectLevel(url));
    dx::World loaded;
    REQUIRE(loaded.loadMap(url));
    CHECK(loaded.level->url == "01_NYC_UNATCOIsland?Name=Player?Class=DeusEx.JCDentonMale");
    unloadMaps();
    std::filesystem::remove_all(root);
}

TEST_CASE("zzprobe pitch", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    std::filesystem::path root = freshSaveRoot("dx-savegame-test-pitch");
    unloadMaps();
    for (int pitch : { 64000, 1500 }) {
        {
            dx::World world;
            REQUIRE(world.loadMap(MapName));
            for (int i = 0; i < 10; i++) {
                world.tick(1.0f / 30.0f);
            }
            dx::UObject* player = world.player();
            dx::pawn::ViewRotation(player).pitch = pitch;
            for (int i = 0; i < 10; i++) {
                world.tick(1.0f / 30.0f);
            }
            std::printf("before %d rot %d\n", dx::pawn::ViewRotation(player).pitch, dx::actor::Rotation(player).pitch);
            REQUIRE(dx::savegame::saveGame(world, 8, "pitch"));
            unloadMaps();
        }
        std::string map;
        REQUIRE(dx::savegame::prepareLoad(8, map));
        std::string url = map + "?loadgame=8";
        REQUIRE(dx::savegame::selectLevel(url));
        dx::World loaded;
        REQUIRE(loaded.loadMap(url));
        dx::UObject* player = loaded.player();
        std::printf("loaded %d rot %d\n", dx::pawn::ViewRotation(player).pitch, dx::actor::Rotation(player).pitch);
        for (int i = 0; i < 5; i++) {
            loaded.tick(1.0f / 30.0f);
            std::printf("tick %d %d rot %d\n", i, dx::pawn::ViewRotation(player).pitch, dx::actor::Rotation(player).pitch);
        }
        unloadMaps();
    }
    std::filesystem::remove_all(root);
}

TEST_CASE("plane struct values are written and read with their own member W before the inherited X Y Z", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UClass* actorClass = dx::objects::findClass("Engine.Actor");
    REQUIRE(actorClass != nullptr);
    dx::UProperty* simAnim = actorClass->findProperty(dx::Name("SimAnim"));
    REQUIRE(simAnim != nullptr);
    REQUIRE(simAnim->structType->name == dx::Name("Plane"));
    dx::UObject* transient = dx::objects::packageObject(dx::Name("Transient"));
    dx::UObject* actor = dx::objects::newObject(actorClass, transient, dx::Name("PlaneOrderActor"), 0);
    const float values[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    for (int i = 0; i < 4; i++) {
        dx::UProperty* member = simAnim->structType->findProperty(dx::Name(std::string(1, "XYZW"[i])));
        REQUIRE(member != nullptr);
        *reinterpret_cast<float*>(actor->data + simAnim->offset + member->offset) = values[i];
    }

    dx::PackageWriter writer(dx::Name("PlaneOrder"));
    dx::Writer out;
    writer.writeTaggedProperties(out, actorClass, actor->data, actorClass->defaultObject->data);
    const std::vector<uint8_t>& bytes = out.data();
    dx::Writer tag;
    tag.index(writer.nameIndex(dx::Name("SimAnim")));
    tag.u8(0x4a);
    tag.index(writer.nameIndex(dx::Name("Plane")));
    auto at = std::search(bytes.begin(), bytes.end(), tag.data().begin(), tag.data().end());
    REQUIRE(at != bytes.end());
    size_t payload = static_cast<size_t>(at - bytes.begin()) + tag.size();
    REQUIRE(payload + 16 <= bytes.size());
    float written[4] = {};
    std::memcpy(written, bytes.data() + payload, sizeof(written));
    CHECK(written[0] == 4.0f);
    CHECK(written[1] == 1.0f);
    CHECK(written[2] == 2.0f);
    CHECK(written[3] == 3.0f);

    dx::PackageWriter package(dx::Name("PlaneOrder"));
    package.addExport(actor, nullptr);
    package.setFilter([](dx::UObject* object) {
        return object->linker != nullptr || dx::cast<dx::UClass>(object) != nullptr || !object->outer;
    });
    std::filesystem::path file = std::filesystem::temp_directory_path() / "dx-plane-order.dxs";
    std::string error;
    REQUIRE(package.save(file, &error));
    dx::Linker* linker = dx::objects::loadPackageFile(dx::Name("PlaneOrderPackage"), file, true);
    REQUIRE(linker != nullptr);
    dx::UObject* loaded = linker->object(linker->findExport(dx::Name("PlaneOrderActor"), dx::Name("Actor"), ""));
    dx::objects::flushLoads();
    REQUIRE(loaded != nullptr);
    for (int i = 0; i < 4; i++) {
        dx::UProperty* member = simAnim->structType->findProperty(dx::Name(std::string(1, "XYZW"[i])));
        CHECK(*reinterpret_cast<const float*>(loaded->data + simAnim->offset + member->offset) == values[i]);
    }
    dx::objects::unloadPackage(dx::Name("PlaneOrderPackage"));
    std::filesystem::remove(file);
}
