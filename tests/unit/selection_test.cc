#include "core/objects.h"
#include "engine/actor.h"
#include "engine/actor_selection.h"
#include "engine/csg.h"
#include "engine/editor_history.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/t3d.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
    dx::UObject* spawn(const char* className, dx::ULevel* level, const dx::Vector& location)
    {
        dx::UObject* actor = dx::objects::newObject(dx::objects::findClass(className), level->outermost(), dx::Name(), dx::RF_Transactional);
        actor->loaded = true;
        dx::actor::Location(actor) = location;
        level->actors.push_back(actor);
        return actor;
    }

    dx::ULevel* emptyLevel(const char* packageName)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        dx::ULevel* level = dx::cast<dx::ULevel>(dx::objects::newObject(dx::csg::nativeClass("Level"), package, dx::Name("MyLevel"), 0));
        level->loaded = true;
        level->model = dx::cast<dx::UModel>(dx::objects::newObject(dx::csg::nativeClass("Model"), package, dx::Name(), 0));
        level->model->loaded = true;
        return level;
    }

    dx::UObject* brushActor(dx::ULevel* level, const dx::Vector& location)
    {
        dx::UObject* brush = spawn("Engine.Brush", level, location);
        dx::UModel* model = dx::cast<dx::UModel>(dx::objects::newObject(dx::csg::nativeClass("Model"), level->outermost(), dx::Name(), 0));
        model->loaded = true;
        model->polys = dx::cast<dx::UPolys>(dx::objects::newObject(dx::csg::nativeClass("Polys"), level->outermost(), dx::Name(), 0));
        model->polys->loaded = true;
        dx::csg::buildCube(*model->polys, 128.0f, 64.0f, 32.0f, nullptr);
        dx::csg::updateBounds(*model);
        dx::actor::Brush(brush) = model;
        dx::brush::CsgOper(brush) = dx::csg::CsgSubtract;
        return brush;
    }

    size_t countOf(const std::string& text, const std::string& needle)
    {
        size_t count = 0;
        for (size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) {
            count++;
        }
        return count;
    }
}

TEST_CASE("actors export as UnrealEd T3D text with changed properties and brush polygons", "[data]")
{
    dx::ULevel* level = emptyLevel("T3DExportTest");
    dx::UObject* light = spawn("Engine.Light", level, { 16.0f, -32.0f, 48.0f });
    dx::actor::LightBrightness(light) = 200;
    dx::actor::Tag(light) = dx::Name("Lamp");
    dx::UObject* brush = brushActor(level, { 0.0f, 0.0f, 0.0f });
    std::string text = dx::t3d::exportActors({ light, brush });
    CHECK(text.rfind("Begin Map\r\nBegin Actor Class=Light Name=" + light->name.str() + "\r\n", 0) == 0);
    CHECK(text.find("    LightBrightness=200\r\n") != std::string::npos);
    CHECK(text.find("    Tag=Lamp\r\n") != std::string::npos);
    CHECK(text.find("    Location=(X=16.000000,Y=-32.000000,Z=48.000000)\r\n") != std::string::npos);
    CHECK(text.find("LightRadius=") == std::string::npos);
    CHECK(text.find("Begin Actor Class=Brush Name=" + brush->name.str() + "\r\n") != std::string::npos);
    CHECK(text.find("    CsgOper=CSG_Subtract\r\n") != std::string::npos);
    CHECK(text.find("    Begin Brush Name=" + dx::actor::Brush(brush)->name.str() + "\r\n       Begin PolyList\r\n") != std::string::npos);
    CHECK(countOf(text, "Begin Polygon") == 6);
    CHECK(countOf(text, "End Actor\r\n") == 2);
    CHECK(text.size() > 10);
    CHECK(text.substr(text.size() - 9) == "End Map\r\n");
}

TEST_CASE("pasting T3D text creates new actors with the copied properties and brush", "[data]")
{
    dx::ULevel* level = emptyLevel("T3DPasteTest");
    dx::UObject* info = spawn("Engine.LevelInfo", level, {});
    dx::UObject* light = spawn("Engine.Light", level, { 16.0f, -32.0f, 48.0f });
    dx::actor::LightBrightness(light) = 200;
    dx::actor::Tag(light) = dx::Name("Lamp");
    dx::UObject* brush = brushActor(level, { 64.0f, 0.0f, 0.0f });
    dx::UModel* source = dx::cast<dx::UModel>(dx::actor::Brush(brush));
    std::string text = dx::t3d::exportActors({ info, light, brush });
    std::vector<dx::UObject*> pasted = dx::t3d::importActors(level, text, false);
    REQUIRE(pasted.size() == 2);
    CHECK(level->actors.size() == 5);
    dx::UObject* lightCopy = pasted[0];
    dx::UObject* brushCopy = pasted[1];
    CHECK(lightCopy != light);
    CHECK(lightCopy->name != light->name);
    CHECK(lightCopy->cls == light->cls);
    CHECK(dx::actor::LightBrightness(lightCopy) == 200);
    CHECK(dx::actor::Tag(lightCopy) == dx::Name("Lamp"));
    CHECK(dx::actor::Location(lightCopy).x == 16.0f);
    CHECK(dx::actor::Location(lightCopy).y == -32.0f);
    CHECK(dx::actor::Location(lightCopy).z == 48.0f);
    CHECK(dx::actor::Level(lightCopy) == info);
    CHECK(dx::brush::CsgOper(brushCopy) == dx::csg::CsgSubtract);
    dx::UModel* model = dx::cast<dx::UModel>(dx::actor::Brush(brushCopy));
    REQUIRE(model != nullptr);
    CHECK(model != source);
    REQUIRE(model->polys != nullptr);
    REQUIRE(model->polys->polys.size() == 6);
    for (size_t i = 0; i < 6; i++) {
        const dx::Poly& a = source->polys->polys[i];
        const dx::Poly& b = model->polys->polys[i];
        REQUIRE(a.vertices.size() == b.vertices.size());
        for (size_t v = 0; v < a.vertices.size(); v++) {
            CHECK(a.vertices[v].x == b.vertices[v].x);
            CHECK(a.vertices[v].y == b.vertices[v].y);
            CHECK(a.vertices[v].z == b.vertices[v].z);
        }
    }
    CHECK(model->bounds.max.x == 64.0f);
}

TEST_CASE("pasted actor references point at pasted copies or at the existing actors", "[data]")
{
    dx::ULevel* level = emptyLevel("T3DReferenceTest");
    spawn("Engine.LevelInfo", level, {});
    dx::UObject* owner = spawn("Engine.Light", level, { 0.0f, 0.0f, 0.0f });
    dx::UObject* owned = spawn("Engine.Light", level, { 32.0f, 0.0f, 0.0f });
    dx::actor::Owner(owned) = owner;
    std::vector<dx::UObject*> both = dx::t3d::importActors(level, dx::t3d::exportActors({ owner, owned }), false);
    REQUIRE(both.size() == 2);
    CHECK(dx::actor::Owner(both[1]) == both[0]);
    std::vector<dx::UObject*> single = dx::t3d::importActors(level, dx::t3d::exportActors({ owned }), false);
    REQUIRE(single.size() == 1);
    CHECK(dx::actor::Owner(single[0]) == owner);
}

TEST_CASE("a whole level exports to T3D and imports into a new level with the same actors", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    dx::objects::setEditorMode(true);
    dx::Linker* linker = dx::objects::loadPackage(dx::Name("01_NYC_UNATCOIsland"));
    REQUIRE(linker != nullptr);
    dx::ULevel* source = dx::cast<dx::ULevel>(linker->object(linker->findExport(dx::Name("MyLevel"), dx::Name("Level"), "*")));
    dx::objects::ensureLoaded(source);
    dx::objects::flushLoads();
    std::vector<dx::UObject*> originals;
    for (dx::UObject* actor : source->actors) {
        if (actor) {
            dx::objects::ensureLoaded(actor);
            originals.push_back(actor);
        }
    }
    dx::objects::flushLoads();
    std::string text = dx::t3d::exportActors(originals);
    CHECK(text.find("\n    Name=") == std::string::npos);
    dx::ULevel* target = emptyLevel("T3DLevelImport");
    std::vector<dx::UObject*> imported = dx::t3d::importActors(target, text, true);
    REQUIRE(imported.size() == originals.size());
    REQUIRE(target->actors.size() == originals.size());
    std::unordered_map<dx::UObject*, dx::UObject*> copies;
    for (size_t i = 0; i < originals.size(); i++) {
        copies[originals[i]] = target->actors[i];
    }
    CHECK(target->actors[0]->cls->name == dx::Name("LevelInfo"));
    CHECK(target->actors[1]->cls->name == dx::Name("Brush"));
    size_t brushes = 0;
    size_t mismatches = 0;
    std::string report;
    for (size_t i = 0; i < originals.size(); i++) {
        dx::UObject* original = originals[i];
        dx::UObject* copy = target->actors[i];
        REQUIRE(copy->cls == original->cls);
        for (dx::UProperty* property : original->cls->fieldOrder()) {
            if (property->name == dx::Name("Name") || property->name == dx::Name("Level") || property->name == dx::Name("XLevel") || property->name == dx::Name("Region") || property->name == dx::Name("Brush")) {
                continue;
            }
            if ((property->propertyFlags & (dx::CPF_Transient | dx::CPF_Native)) && property->category.none()) {
                continue;
            }
            if (property->type == dx::PropType::Array || property->type == dx::PropType::Map || property->type == dx::PropType::FixedArray || property->type == dx::PropType::Pointer) {
                continue;
            }
            for (int32_t index = 0; index < property->arrayDim; index++) {
                size_t at = property->offset + static_cast<size_t>(index) * property->elementSize();
                if (property->type == dx::PropType::Object) {
                    dx::UObject* before = *reinterpret_cast<dx::UObject**>(original->data + at);
                    dx::UObject* after = *reinterpret_cast<dx::UObject**>(copy->data + at);
                    auto mapped = copies.find(before);
                    dx::UObject* expected = mapped != copies.end() ? mapped->second : before;
                    if (expected != after) {
                        mismatches++;
                        report += original->name.str() + "." + property->name.str() + "\n";
                    }
                    continue;
                }
                if (property->toText(original->data + at) != property->toText(copy->data + at)) {
                    mismatches++;
                    report += original->name.str() + "." + property->name.str() + " " + property->toText(original->data + at) + " vs " + property->toText(copy->data + at) + "\n";
                }
            }
        }
        dx::UModel* originalBrush = dx::cast<dx::UModel>(dx::actor::Brush(original));
        if (originalBrush && originalBrush->polys) {
            dx::UModel* copyBrush = dx::cast<dx::UModel>(dx::actor::Brush(copy));
            REQUIRE(copyBrush != nullptr);
            REQUIRE(copyBrush->polys != nullptr);
            CHECK(copyBrush->polys->polys.size() == originalBrush->polys->polys.size());
            brushes++;
        }
    }
    INFO(report.substr(0, 4000));
    CHECK(mismatches == 0);
    CHECK(brushes > 100);
    dx::objects::unloadPackage(dx::Name("01_NYC_UNATCOIsland"));
    dx::objects::setEditorMode(false);
}

TEST_CASE("selection helpers pick actors by class, box, inversion and hidden state", "[data]")
{
    dx::ULevel* level = emptyLevel("SelectionHelpersTest");
    dx::UObject* builder = brushActor(level, { 0.0f, 0.0f, 0.0f });
    dx::UObject* a = spawn("Engine.Light", level, { 0.0f, 0.0f, 0.0f });
    dx::UObject* b = spawn("Engine.Light", level, { 100.0f, 50.0f, 500.0f });
    dx::UObject* c = spawn("Engine.PlayerStart", level, { 300.0f, 0.0f, 0.0f });
    dx::UObject* d = spawn("Engine.Light", level, { -200.0f, 0.0f, 0.0f });
    dx::actor::bHiddenEd(d) = 1;
    std::vector<dx::UObject*> actors = { builder, a, b, c, d };
    CHECK(dx::actorselect::ofClass(actors, a->cls) == std::vector<dx::UObject*> { a, b });
    CHECK(dx::actorselect::visible(actors) == std::vector<dx::UObject*> { builder, a, b, c });
    CHECK(dx::actorselect::inBox(actors, 0, 1, 150.0f, 60.0f, 50.0f, -10.0f) == std::vector<dx::UObject*> { b });
    CHECK(dx::actorselect::inBox(actors, 0, 2, -300.0f, -10.0f, 400.0f, 10.0f) == std::vector<dx::UObject*> { builder, a, c });
    CHECK(dx::actorselect::inverted(actors, { a }, builder) == std::vector<dx::UObject*> { b, c });
    CHECK(dx::actorselect::toUnhide(actors) == std::vector<dx::UObject*> { d });
    CHECK(dx::actorselect::commonClass({ a, b }) == a->cls);
    CHECK(dx::actorselect::commonClass({ a, c })->name == dx::Name("Actor"));
    CHECK(dx::actorselect::commonClass({ c, b, builder })->name == dx::Name("Actor"));
}

TEST_CASE("editing a property on several actors shows the common value and undoes in one step", "[data]")
{
    dx::ULevel* level = emptyLevel("MultiEditTest");
    dx::UObject* a = spawn("Engine.Light", level, {});
    dx::UObject* b = spawn("Engine.Light", level, {});
    dx::actor::LightBrightness(a) = 10;
    dx::actor::LightBrightness(b) = 20;
    dx::UProperty* property = a->cls->findProperty(dx::Name("LightBrightness"));
    REQUIRE(property != nullptr);
    std::string text = "unset";
    CHECK_FALSE(dx::actorselect::commonText({ a, b }, property, 0, text));
    CHECK(text.empty());
    dx::EditorHistory history;
    history.begin(level, { a, b }, "Set LightBrightness");
    CHECK(dx::actorselect::setText({ a, b }, property, 0, "77"));
    history.commit();
    CHECK(dx::actor::LightBrightness(a) == 77);
    CHECK(dx::actor::LightBrightness(b) == 77);
    CHECK(dx::actorselect::commonText({ a, b }, property, 0, text));
    CHECK(text == "77");
    REQUIRE(history.undo());
    CHECK(dx::actor::LightBrightness(a) == 10);
    CHECK(dx::actor::LightBrightness(b) == 20);
    CHECK(history.undoCount() == 0);
}

TEST_CASE("z clipping keeps the part of a segment inside the slab")
{
    dx::Vector a { 0.0f, 0.0f, -100.0f };
    dx::Vector b { 100.0f, 0.0f, 100.0f };
    REQUIRE(dx::actorselect::clipZ(a, b, 0.0f, 50.0f));
    CHECK(a.x == 50.0f);
    CHECK(a.z == 0.0f);
    CHECK(b.x == 75.0f);
    CHECK(b.z == 50.0f);
    dx::Vector c { 0.0f, 0.0f, 60.0f };
    dx::Vector d { 10.0f, 0.0f, 90.0f };
    CHECK_FALSE(dx::actorselect::clipZ(c, d, 0.0f, 50.0f));
    dx::Vector e { 0.0f, 5.0f, 10.0f };
    dx::Vector f { 0.0f, 9.0f, 10.0f };
    REQUIRE(dx::actorselect::clipZ(e, f, 0.0f, 50.0f));
    CHECK(e.y == 5.0f);
    CHECK(f.y == 9.0f);
}

TEST_CASE("gizmo rotation axes convert back to the actor rotator")
{
    const dx::Rotator rotations[] = { { 0, 0, 0 }, { 4096, 8192, 0 }, { -3000, 30000, 2000 }, { 0, -16384, -8000 }, { 12000, 100, 30000 } };
    for (const dx::Rotator& rotation : rotations) {
        dx::Rotator back = dx::actorselect::rotatorFromAxes(dx::rotationAxes(rotation));
        CHECK(std::abs(back.pitch - rotation.pitch) <= 1);
        CHECK(std::abs(back.yaw - rotation.yaw) <= 1);
        CHECK(std::abs(back.roll - rotation.roll) <= 1);
    }
    CHECK(dx::actorselect::rotationDelta({ 0, 65000, 0 }, { 0, 100, 0 }) == dx::Rotator { 0, 636, 0 });
    CHECK(dx::actorselect::rotationDelta({ 0, 0, 0 }, { 0, 40000, 0 }) == dx::Rotator { 0, -25536, 0 });
    CHECK(dx::actorselect::rotationDelta({ 100, 0, -50 }, { 300, 0, 50 }) == dx::Rotator { 200, 0, 100 });
}
