#include "core/objects.h"
#include "editor/log_capture.h"
#include "engine/actor.h"
#include "engine/actor_search.h"
#include "engine/csg.h"
#include "engine/map_check.h"
#include "engine/property_edit.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace {
    struct CheckRoom {
        dx::UObject* package = nullptr;
        dx::ULevel* level = nullptr;
        dx::UObject* builder = nullptr;
    };

    dx::UObject* spawn(CheckRoom& room, const char* className, const dx::Vector& location)
    {
        dx::UObject* actor = dx::objects::newObject(dx::objects::findClass(className), room.package, dx::Name(), 0);
        actor->loaded = true;
        dx::actor::Location(actor) = location;
        room.level->actors.push_back(actor);
        return actor;
    }

    CheckRoom makeCheckRoom(const char* packageName)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        CheckRoom room;
        room.package = dx::objects::packageObject(dx::Name(packageName));
        room.level = dx::cast<dx::ULevel>(dx::objects::newObject(dx::csg::nativeClass("Level"), room.package, dx::Name(), 0));
        room.level->model = dx::cast<dx::UModel>(dx::objects::newObject(dx::csg::nativeClass("Model"), room.package, dx::Name(), 0));
        spawn(room, "Engine.LevelInfo", {});
        room.builder = spawn(room, "Engine.Brush", {});
        dx::UModel* builderModel = dx::cast<dx::UModel>(dx::objects::newObject(dx::csg::nativeClass("Model"), room.package, dx::Name(), 0));
        builderModel->polys = dx::cast<dx::UPolys>(dx::objects::newObject(dx::csg::nativeClass("Polys"), room.package, dx::Name(), 0));
        dx::actor::Brush(room.builder) = builderModel;
        dx::brush::CsgOper(room.builder) = dx::csg::CsgActive;
        dx::csg::buildCube(*builderModel->polys, 512.0f, 512.0f, 256.0f, nullptr);
        dx::csg::addBrush(room.level, room.builder, dx::csg::CsgSubtract);
        spawn(room, "DeusEx.DeusExLevelInfo", {});
        return room;
    }

    std::vector<dx::mapcheck::Issue> issuesFor(const std::vector<dx::mapcheck::Issue>& issues, dx::UObject* actor)
    {
        std::vector<dx::mapcheck::Issue> result;
        for (const dx::mapcheck::Issue& issue : issues) {
            if (issue.actor == actor) {
                result.push_back(issue);
            }
        }
        return result;
    }

    bool mentions(const std::vector<dx::mapcheck::Issue>& issues, dx::UObject* actor, const std::string& text)
    {
        for (const dx::mapcheck::Issue& issue : issuesFor(issues, actor)) {
            if (issue.message.find(text) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
}

TEST_CASE("map check reports a level without a PlayerStart as an error", "[data]")
{
    CheckRoom room = makeCheckRoom("MapCheckStartTest");
    dx::csg::rebuild(room.level);
    std::vector<dx::mapcheck::Issue> issues = dx::mapcheck::check(room.level);
    REQUIRE(mentions(issues, nullptr, "No PlayerStart"));
    CHECK(issuesFor(issues, nullptr).front().severity == dx::mapcheck::Severity::Error);
    spawn(room, "Engine.PlayerStart", { 0.0f, 0.0f, 0.0f });
    CHECK(dx::mapcheck::check(room.level).empty());
}

TEST_CASE("map check reports an Event that matches no Tag until an actor carries that Tag", "[data]")
{
    CheckRoom room = makeCheckRoom("MapCheckEventTest");
    spawn(room, "Engine.PlayerStart", {});
    dx::UObject* trigger = spawn(room, "Engine.Trigger", { 64.0f, 0.0f, 0.0f });
    dx::actor::Event(trigger) = dx::Name("OpenVault");
    dx::csg::rebuild(room.level);
    std::vector<dx::mapcheck::Issue> issues = dx::mapcheck::check(room.level);
    REQUIRE(issuesFor(issues, trigger).size() == 1);
    CHECK(issuesFor(issues, trigger)[0].message == "Event OpenVault matches the Tag of no actor");
    CHECK(issuesFor(issues, trigger)[0].severity == dx::mapcheck::Severity::Warning);
    dx::UObject* target = spawn(room, "Engine.Light", { -64.0f, 0.0f, 0.0f });
    dx::actor::Tag(target) = dx::Name("OpenVault");
    CHECK(issuesFor(dx::mapcheck::check(room.level), trigger).empty());
}

TEST_CASE("map check finds actors in solid space, empty brushes, dead lights, zone infos sharing a zone and untouchable triggers", "[data]")
{
    CheckRoom room = makeCheckRoom("MapCheckActorsTest");
    spawn(room, "Engine.PlayerStart", {});
    dx::UObject* inside = spawn(room, "Engine.Light", { 0.0f, 0.0f, 32.0f });
    dx::actor::LightType(inside) = 1;
    dx::actor::LightBrightness(inside) = 64;
    dx::actor::LightRadius(inside) = 16;
    dx::UObject* outside = spawn(room, "Engine.PlayerStart", { 4000.0f, 0.0f, 0.0f });
    dx::UObject* dark = spawn(room, "Engine.Light", { 32.0f, 0.0f, 0.0f });
    dx::actor::LightType(dark) = 1;
    dx::actor::LightBrightness(dark) = 0;
    dx::actor::LightRadius(dark) = 16;
    dx::UObject* zone = spawn(room, "Engine.ZoneInfo", { 0.0f, 32.0f, 0.0f });
    dx::UObject* sameZone = spawn(room, "Engine.ZoneInfo", { 0.0f, 64.0f, 0.0f });
    dx::UObject* trigger = spawn(room, "Engine.Trigger", { 0.0f, -32.0f, 0.0f });
    dx::actor::bCollideActors(trigger) = 0;
    dx::actor::Tag(trigger) = dx::Name("Relay");
    dx::csg::rebuild(room.level);
    dx::UObject* empty = spawn(room, "Engine.Brush", {});
    dx::UModel* emptyModel = dx::cast<dx::UModel>(dx::objects::newObject(dx::csg::nativeClass("Model"), room.package, dx::Name(), 0));
    emptyModel->polys = dx::cast<dx::UPolys>(dx::objects::newObject(dx::csg::nativeClass("Polys"), room.package, dx::Name(), 0));
    dx::actor::Brush(empty) = emptyModel;
    dx::brush::CsgOper(empty) = dx::csg::CsgAdd;
    std::vector<dx::mapcheck::Issue> issues = dx::mapcheck::check(room.level);
    CHECK(issuesFor(issues, inside).empty());
    CHECK(mentions(issues, outside, "outside the world"));
    CHECK(mentions(issues, dark, "LightBrightness is 0"));
    CHECK(issuesFor(issues, zone).empty());
    CHECK(mentions(issues, sameZone, "ZoneInfo has no effect"));
    CHECK(mentions(issues, trigger, "can never fire"));
    CHECK(mentions(issues, empty, "Brush has no polygons"));
    CHECK(issuesFor(issues, empty)[0].severity == dx::mapcheck::Severity::Error);
    CHECK(issuesFor(issues, room.builder).empty());
    CHECK(issuesFor(issues, room.level->actors[0]).empty());
    dx::UObject* relay = spawn(room, "Engine.Light", { 0.0f, 0.0f, -32.0f });
    dx::actor::Event(relay) = dx::Name("Relay");
    CHECK(issuesFor(dx::mapcheck::check(room.level), trigger).empty());
}

TEST_CASE("map check reports duplicate actor names and abstract actor classes", "[data]")
{
    CheckRoom room = makeCheckRoom("MapCheckNamesTest");
    spawn(room, "Engine.PlayerStart", {});
    dx::UObject* first = spawn(room, "Engine.Light", {});
    dx::UObject* second = spawn(room, "Engine.Light", { 16.0f, 0.0f, 0.0f });
    second->name = first->name;
    dx::UClass* info = dx::objects::findClass("Engine.Info");
    REQUIRE((info->classFlags & dx::CLASS_Abstract) != 0);
    dx::UObject* abstract = spawn(room, "Engine.Info", { 0.0f, 16.0f, 0.0f });
    dx::csg::rebuild(room.level);
    std::vector<dx::mapcheck::Issue> issues = dx::mapcheck::check(room.level);
    CHECK(issuesFor(issues, first).empty());
    CHECK(mentions(issues, second, "is used by more than one actor"));
    CHECK(mentions(issues, abstract, "Info is abstract"));
}

TEST_CASE("actor search matches name, class, tag and event and find next cycles through matches", "[data]")
{
    CheckRoom room = makeCheckRoom("ActorSearchTest");
    dx::UObject* lamp = spawn(room, "Engine.Light", {});
    dx::UObject* door = spawn(room, "Engine.Trigger", {});
    dx::UObject* bell = spawn(room, "Engine.Light", {});
    dx::actor::Tag(lamp) = dx::Name("HallLamp");
    dx::actor::Event(door) = dx::Name("OpenHall");
    dx::actor::Tag(bell) = dx::Name("Bell");
    std::vector<dx::UObject*> actors = { lamp, door, bell };
    CHECK(dx::actorsearch::find(actors, { "", "light", "", "" }) == std::vector<dx::UObject*> { lamp, bell });
    CHECK(dx::actorsearch::find(actors, { "", "", "hall", "" }) == std::vector<dx::UObject*> { lamp });
    CHECK(dx::actorsearch::find(actors, { "", "", "", "HALL" }) == std::vector<dx::UObject*> { door });
    CHECK(dx::actorsearch::find(actors, { door->name.str(), "", "", "" }) == std::vector<dx::UObject*> { door });
    CHECK(dx::actorsearch::find(actors, { "", "Light", "Bell", "" }) == std::vector<dx::UObject*> { bell });
    CHECK(dx::actorsearch::find(actors, { "", "Trigger", "Bell", "" }).empty());
    dx::actorsearch::Query lights { "", "Light", "", "" };
    CHECK(dx::actorsearch::findNext(actors, lights, nullptr) == lamp);
    CHECK(dx::actorsearch::findNext(actors, lights, lamp) == bell);
    CHECK(dx::actorsearch::findNext(actors, lights, bell) == lamp);
    CHECK(dx::actorsearch::findNext(actors, lights, door) == bell);
    CHECK(dx::actorsearch::findNext(actors, { "", "Mover", "", "" }, lamp) == nullptr);
}

TEST_CASE("object fields accept only matching selections and color structs convert to and from floats", "[data]")
{
    CheckRoom room = makeCheckRoom("PropertyEditTest");
    dx::UObject* info = room.level->actors[0];
    dx::UProperty* gameType = info->cls->findProperty(dx::Name("DefaultGameType"));
    dx::UProperty* texture = info->cls->findProperty(dx::Name("Texture"));
    REQUIRE(gameType);
    REQUIRE(texture);
    dx::UObject* defaultTexture = dx::objects::loadObject("Engine.DefaultTexture", nullptr);
    dx::objects::flushLoads();
    REQUIRE(defaultTexture);
    dx::UClass* gameInfo = dx::objects::findClass("DeusEx.DeusExGameInfo");
    dx::UClass* light = dx::objects::findClass("Engine.Light");
    REQUIRE(gameInfo);
    CHECK(dx::propedit::pickCurrent(texture, { light, info, defaultTexture }) == defaultTexture);
    CHECK(dx::propedit::pickCurrent(gameType, { defaultTexture, light, gameInfo }) == gameInfo);
    CHECK(dx::propedit::pickCurrent(gameType, { defaultTexture, light }) == nullptr);
    uint8_t* textureMemory = info->data + texture->offset;
    CHECK_FALSE(dx::propedit::assignObject(texture, textureMemory, info));
    REQUIRE(dx::propedit::assignObject(texture, textureMemory, defaultTexture));
    CHECK(*reinterpret_cast<dx::UObject**>(textureMemory) == defaultTexture);
    dx::UProperty* drawColor = dx::objects::findClass("Engine.Canvas")->findProperty(dx::Name("DrawColor"));
    REQUIRE(drawColor);
    CHECK(dx::propedit::isColor(drawColor));
    CHECK_FALSE(dx::propedit::isColor(gameType));
    std::vector<uint8_t> color(drawColor->structType->size);
    dx::propedit::setColor(drawColor, color.data(), { 1.0f, 0.5f, 0.0f, 0.25f });
    CHECK(drawColor->toText(color.data()) == "(R=255,G=128,B=0,A=64)");
    REQUIRE(drawColor->fromText(color.data(), "(R=51,G=102,B=204,A=255)"));
    std::array<float, 4> read = dx::propedit::colorValue(drawColor, color.data());
    CHECK(read[0] == Catch::Approx(0.2f));
    CHECK(read[1] == Catch::Approx(0.4f));
    CHECK(read[2] == Catch::Approx(0.8f));
    CHECK(read[3] == Catch::Approx(1.0f));
}

TEST_CASE("level properties edit the LevelInfo and the DeusExLevelInfo", "[data]")
{
    CheckRoom room = makeCheckRoom("LevelPropertiesTest");
    std::vector<dx::UObject*> objects = dx::propedit::levelPropertyObjects(room.level);
    REQUIRE(objects.size() == 2);
    CHECK(objects[0]->cls->name == dx::Name("LevelInfo"));
    CHECK(objects[1]->cls->name == dx::Name("DeusExLevelInfo"));
    CHECK(dx::propedit::levelPropertyObjects(nullptr).empty());
}

TEST_CASE("the log capture records lines written to stderr until it stops")
{
    dx::editor::LogCapture capture;
    REQUIRE(capture.start());
    std::fprintf(stderr, "log capture marker one\npartial");
    std::fprintf(stderr, " line\n");
    std::vector<std::string> lines;
    for (int attempt = 0; attempt < 200; attempt++) {
        lines = capture.lines();
        if (lines.size() >= 2) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK(lines == std::vector<std::string> { "log capture marker one", "partial line" });
    capture.stop();
    std::fprintf(stderr, "after the capture\n");
    capture.add("> command");
    CHECK(capture.lines() == std::vector<std::string> { "log capture marker one", "partial line", "> command" });
    capture.clear();
    CHECK(capture.lines().empty());
}
