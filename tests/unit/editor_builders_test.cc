#include "core/objects.h"
#include "engine/actor.h"
#include "engine/bsp.h"
#include "engine/csg.h"
#include "engine/extra_builders.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <utility>

namespace {
    template <typename T>
    T* make(const char* className, dx::UObject* outer)
    {
        dx::UObject* object = dx::objects::newObject(dx::csg::nativeClass(className), outer, dx::Name(), 0);
        object->loaded = true;
        return dx::cast<T>(object);
    }

    dx::UObject* makeBuilderBrush(const char* packageName)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        dx::UObject* brush = make<dx::UObject>("Brush", package);
        dx::UModel* model = make<dx::UModel>("Model", package);
        model->polys = make<dx::UPolys>("Polys", package);
        dx::actor::Brush(brush) = model;
        return brush;
    }

    std::vector<dx::Poly>& polysOf(dx::UObject* brush)
    {
        return dx::cast<dx::UModel>(dx::actor::Brush(brush))->polys->polys;
    }

    dx::brushbuilder::ExtraBuilder builderNamed(const char* name, std::initializer_list<std::pair<const char*, double>> overrides)
    {
        for (dx::brushbuilder::ExtraBuilder& builder : dx::brushbuilder::extraBuilders()) {
            if (builder.name != name) {
                continue;
            }
            for (const std::pair<const char*, double>& item : overrides) {
                bool found = false;
                for (dx::brushbuilder::ExtraParameter& parameter : builder.parameters) {
                    if (parameter.name == item.first) {
                        parameter.value = item.second;
                        found = true;
                    }
                }
                REQUIRE(found);
            }
            return builder;
        }
        FAIL("missing builder " << name);
        return {};
    }

    double defaultOf(const char* builderName, const char* parameterName)
    {
        dx::brushbuilder::ExtraBuilder builder = builderNamed(builderName, {});
        for (const dx::brushbuilder::ExtraParameter& parameter : builder.parameters) {
            if (parameter.name == parameterName) {
                return parameter.value;
            }
        }
        FAIL("missing parameter " << parameterName);
        return 0;
    }

    void checkVertex(const dx::Poly& poly, size_t index, float x, float y, float z)
    {
        REQUIRE(poly.vertices.size() > index);
        CHECK(poly.vertices[index].x == Catch::Approx(x).margin(0.001));
        CHECK(poly.vertices[index].y == Catch::Approx(y).margin(0.001));
        CHECK(poly.vertices[index].z == Catch::Approx(z).margin(0.001));
    }

    size_t countItem(const std::vector<dx::Poly>& polys, const char* item)
    {
        size_t count = 0;
        for (const dx::Poly& poly : polys) {
            if (poly.itemName == dx::Name(item)) {
                count++;
            }
        }
        return count;
    }
}

TEST_CASE("extra builders expose the VB form defaults", "[editor_builders]")
{
    std::vector<dx::brushbuilder::ExtraBuilder> builders = dx::brushbuilder::extraBuilders();
    REQUIRE(builders.size() == 5);
    CHECK(builders[0].name == "LinearStair");
    CHECK(builders[1].name == "CurvedStair");
    CHECK(builders[2].name == "SpiralStair");
    CHECK(builders[3].name == "Sphere");
    CHECK(builders[4].name == "Volumetric");
    CHECK(defaultOf("LinearStair", "StepLength") == 32);
    CHECK(defaultOf("LinearStair", "NumSteps") == 8);
    CHECK(defaultOf("CurvedStair", "InnerRad") == 240);
    CHECK(defaultOf("CurvedStair", "CurveAngle") == 90);
    CHECK(defaultOf("CurvedStair", "AlignSide") == 1);
    CHECK(defaultOf("SpiralStair", "OuterSRad") == 240);
    CHECK(defaultOf("SpiralStair", "StepThickness") == 32);
    CHECK(defaultOf("SpiralStair", "Clockwise") == 1);
    CHECK(defaultOf("Sphere", "InnerRadius") == 496);
    CHECK(defaultOf("Sphere", "VerticalStripes") == 4);
}

TEST_CASE("linear stair default matches PsLStair", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraLinearDefault");
    std::string error = "stale";
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("LinearStair", {}), brush, &error));
    CHECK(error.empty());
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 34);
    CHECK(dx::actor::Group(brush) == dx::Name("GROUP"));
    CHECK(countItem(polys, "Rise") == 8);
    CHECK(countItem(polys, "Step") == 8);
    CHECK(countItem(polys, "Side") == 16);
    CHECK(polys[0].itemName == dx::Name("Rise"));
    checkVertex(polys[0], 0, 128, -128, -64);
    checkVertex(polys[0], 1, 128, -128, -48);
    checkVertex(polys[0], 2, -128, -128, -48);
    checkVertex(polys[0], 3, -128, -128, -64);
    CHECK(polys[1].itemName == dx::Name("Step"));
    checkVertex(polys[1], 0, 128, -128, -48);
    checkVertex(polys[1], 1, 128, -96, -48);
    checkVertex(polys[1], 2, -128, -96, -48);
    checkVertex(polys[1], 3, -128, -128, -48);
    checkVertex(polys[2], 0, 128, -96, -48);
    checkVertex(polys[2], 1, 128, -96, -32);
    CHECK(polys[16].itemName == dx::Name("Back"));
    checkVertex(polys[16], 0, -128, 128, -64);
    checkVertex(polys[16], 1, -128, 128, 64);
    checkVertex(polys[16], 2, 128, 128, 64);
    CHECK(polys[17].itemName == dx::Name("Base"));
    checkVertex(polys[17], 2, 128, -128, -64);
    CHECK(polys[18].itemName == dx::Name("Side"));
    checkVertex(polys[18], 0, -128, -128, -64);
    checkVertex(polys[18], 2, -128, 128, -48);
    checkVertex(polys[19], 0, -128, -96, -48);
    checkVertex(polys[26], 0, 128, 128, -64);
    checkVertex(polys[26], 3, 128, -128, -64);
}

TEST_CASE("linear stair truncates odd widths and splits FirstStep like the VB form", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraLinearOdd");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("LinearStair", { { "StepWidth", 255 }, { "FirstStep", 8 } }), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 34);
    checkVertex(polys[0], 0, 127, -128, -68);
    checkVertex(polys[0], 1, 127, -128, -44);
    checkVertex(polys[0], 3, -128, -128, -68);
    checkVertex(polys[2], 0, 127, -96, -44);
    checkVertex(polys[26], 0, 127, 128, -68);
}

TEST_CASE("linear stair rejects the same inputs as the VB form", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraLinearErrors");
    std::string error;
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("LinearStair", { { "StepWidth", 0 } }), brush, &error));
    CHECK(error == "You must use positive, non-zero values for Length, Width, Height, and Number of Steps.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("LinearStair", { { "NumSteps", 1 } }), brush, &error));
    CHECK(error == "A 1-step Staircase is a cube. Use the cube brush.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("LinearStair", { { "NumSteps", 46 } }), brush, &error));
    CHECK(error == "Sorry, you must have 45 or fewer steps.");
    CHECK(polysOf(brush).empty());
}

TEST_CASE("curved stair default matches PsCStair", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraCurvedDefault");
    std::string error;
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("CurvedStair", {}), brush, &error));
    CHECK(error.empty());
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 21);
    CHECK(dx::actor::Group(brush) == dx::Name());
    CHECK(polys[0].itemName == dx::Name("Step"));
    checkVertex(polys[0], 0, 506, 0, -16);
    checkVertex(polys[0], 1, 467.4830f, 193.6378f, -16);
    checkVertex(polys[0], 2, 226.3504f, 93.7574f, -16);
    checkVertex(polys[0], 3, 245, 0, -16);
    CHECK(polys[1].itemName == dx::Name("Base"));
    checkVertex(polys[1], 0, 245, 0, -32);
    CHECK(polys[2].itemName == dx::Name("Rise"));
    checkVertex(polys[2], 1, 245, 0, -32);
    CHECK(polys[5].itemName == dx::Name("Step"));
    checkVertex(polys[5], 3, 226.3504f, 93.7574f, 0);
    CHECK(polys[20].itemName == dx::Name("Back"));
    checkVertex(polys[20], 0, 0, 245, 32);
    checkVertex(polys[20], 1, 0, 506, 32);
    checkVertex(polys[20], 2, 0, 506, -32);
    checkVertex(polys[20], 3, 0, 245, -32);
}

TEST_CASE("curved stair counter clockwise reverses winding and direction", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraCurvedCcw");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("CurvedStair", { { "CCWSelect", 1 } }), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 21);
    checkVertex(polys[0], 0, 245, 0, -16);
    checkVertex(polys[0], 1, 226.3504f, -93.7575f, -16);
    checkVertex(polys[0], 3, 506, 0, -16);
}

TEST_CASE("curved stair without side alignment keeps the typed radii", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraCurvedNoAlign");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("CurvedStair", { { "AlignSide", 0 }, { "FirstStep", 8 } }), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 21);
    checkVertex(polys[0], 0, 496, 0, -20);
    checkVertex(polys[0], 3, 240, 0, -20);
    checkVertex(polys[1], 0, 240, 0, -44);
}

TEST_CASE("curved stair rejects the same inputs as the VB form", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraCurvedErrors");
    std::string error;
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("CurvedStair", { { "CurveAngle", 400 } }), brush, &error));
    CHECK(error == "Use Spiral Staircase for 360 degree or greater angles.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("CurvedStair", { { "FirstStep", -1 } }), brush, &error));
    CHECK(error == "FirstStep must be zero(0) or greater.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("CurvedStair", { { "StepWidth", 0 } }), brush, &error));
    CHECK(error == "All Values must be greater than 0.");
}

TEST_CASE("sphere default matches PsSphere", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSphereDefault");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("Sphere", {}), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 32);
    CHECK(dx::actor::Group(brush) == dx::Name("DOME"));
    CHECK(countItem(polys, "OUTSIDE") == 32);
    REQUIRE(polys[0].vertices.size() == 3);
    checkVertex(polys[0], 0, 0, 0, -512);
    checkVertex(polys[0], 1, 313.5355f, 313.5355f, -256);
    checkVertex(polys[0], 2, 443.4050f, 0, -256);
    REQUIRE(polys[1].vertices.size() == 4);
    checkVertex(polys[1], 0, 443.4050f, 0, -256);
    checkVertex(polys[1], 1, 313.5355f, 313.5355f, -256);
    checkVertex(polys[1], 2, 362.0387f, 362.0387f, 0);
    checkVertex(polys[1], 3, 512, 0, 0);
    REQUIRE(polys[3].vertices.size() == 3);
    checkVertex(polys[3], 2, 0, 0, 512);
}

TEST_CASE("hollow sphere adds reversed inside polygons", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSphereHollow");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("Sphere", { { "IsHollow", 1 } }), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 64);
    CHECK(countItem(polys, "INSIDE") == 32);
    CHECK(polys[1].itemName == dx::Name("INSIDE"));
    REQUIRE(polys[1].vertices.size() == 3);
    checkVertex(polys[1], 0, 429.5486f, 0, -248);
    checkVertex(polys[1], 2, 0, 0, -496);
}

TEST_CASE("sphere validation follows the VB form", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSphereErrors");
    std::string error;
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("Sphere", { { "RadialStripes", 0 } }), brush, &error));
    CHECK(error == "You must give all numbers, and they all must be nonzero");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("Sphere", { { "IsHollow", 1 }, { "InnerRadius", 503 } }), brush, &error));
    CHECK(error == "Inner radius must be 10 larger than outer radius");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("Sphere", { { "RadialStripes", 2 } }), brush, &error));
    CHECK(error == "You need at least 3 radial stripes");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("Sphere", { { "VerticalStripes", 1 } }), brush, &error));
    CHECK(error == "You need at least 2 vertical stripes");
    CHECK(dx::brushbuilder::buildExtra(builderNamed("Sphere", { { "OuterRadius", 40 }, { "RadialStripes", 3 }, { "VerticalStripes", 2 } }), brush, &error));
    CHECK(error == "Outer radius must be at least 48, preferably larger");
    CHECK(polysOf(brush).size() == 6);
}

TEST_CASE("spiral stair default matches PsSStair", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSpiralDefault");
    std::string error;
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", {}), brush, &error));
    CHECK(error.empty());
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 48);
    CHECK(dx::actor::Group(brush) == dx::Name("SPIRAL"));
    CHECK(countItem(polys, "Step") == 16);
    CHECK(countItem(polys, "Rise") == 16);
    CHECK(countItem(polys, "Inner") == 8);
    CHECK(countItem(polys, "Outer") == 8);
    checkVertex(polys[0], 0, 26.5097f, 64, -64);
    checkVertex(polys[0], 1, 99.4112f, 240, -64);
    checkVertex(polys[0], 2, 240, 99.4112f, -64);
    checkVertex(polys[0], 3, 64, 26.5097f, -64);
    checkVertex(polys[1], 3, 26.5097f, 64, -48);
    checkVertex(polys[8], 0, 64, 26.5097f, -32);
    checkVertex(polys[8], 2, 99.4112f, 240, -32);
    CHECK(polys[16].itemName == dx::Name("Rise"));
    checkVertex(polys[16], 0, 240, 99.4112f, -64);
    checkVertex(polys[16], 1, 240, 99.4112f, -32);
    checkVertex(polys[16], 3, 64, 26.5097f, -64);
    checkVertex(polys[17], 0, 26.5097f, 64, -64);
    checkVertex(polys[17], 1, 26.5097f, 64, -48);
    checkVertex(polys[17], 2, 99.4112f, 240, -48);
    CHECK(polys[18].itemName == dx::Name("Rise"));
    checkVertex(polys[18], 0, 99.4112f, 240, -32);
    checkVertex(polys[18], 3, 26.5097f, 64, -32);
    CHECK(polys[32].itemName == dx::Name("Inner"));
    checkVertex(polys[32], 3, 64, 26.5097f, -64);
    checkVertex(polys[32], 0, 64, 26.5097f, -32);
}

TEST_CASE("spiral stair counter clockwise mirrors the angle and winding", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSpiralCcw");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "Clockwise", 0 } }), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 48);
    checkVertex(polys[0], 0, 64, -26.5097f, -64);
    checkVertex(polys[0], 3, 26.5097f, -64, -64);
}

TEST_CASE("spiral stair interior fill builds the center post and caps", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSpiralFillInt");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "IntFill", 1 } }), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 56);
    CHECK(countItem(polys, "Inner") == 0);
    CHECK(countItem(polys, "CenterPost") == 14);
    CHECK(polys[40].itemName == dx::Name("CenterPost"));
    checkVertex(polys[40], 0, -26.5097f, 64, -64);
    checkVertex(polys[40], 1, -26.5097f, 64, -48);
    checkVertex(polys[40], 2, 26.5097f, 64, -48);
    checkVertex(polys[40], 3, 26.5097f, 64, -64);
    CHECK(polys[47].itemName == dx::Name("CenterPost"));
    checkVertex(polys[47], 0, 26.5097f, 64, -32);
    checkVertex(polys[47], 1, 26.5097f, 64, 80);
    checkVertex(polys[47], 2, 64, 26.5097f, 80);
    checkVertex(polys[47], 3, 64, 26.5097f, -32);
    CHECK(polys[54].itemName == dx::Name("Base"));
    REQUIRE(polys[54].vertices.size() == 8);
    checkVertex(polys[54], 7, 64, 26.5097f, -64);
    checkVertex(polys[54], 6, 26.5097f, 64, -64);
    CHECK(polys[55].itemName == dx::Name("Top"));
    checkVertex(polys[55], 0, 64, 26.5097f, 80);
    checkVertex(polys[55], 1, 26.5097f, 64, 80);
}

TEST_CASE("spiral stair exterior fill builds the outer wall ring", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSpiralFillExt");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "ExtFill", 1 } }), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 78);
    CHECK(countItem(polys, "OuterInt") == 14);
    CHECK(countItem(polys, "Outer") == 8);
    CHECK(polys[54].itemName == dx::Name("Outer"));
    checkVertex(polys[54], 3, 255.9146f, 106.0033f, -64);
    checkVertex(polys[54], 2, 255.9146f, 106.0033f, 80);
    checkVertex(polys[54], 1, 106.0033f, 255.9146f, 80);
    CHECK(polys[62].itemName == dx::Name("Base"));
    checkVertex(polys[62], 0, 255.9146f, 106.0033f, -64);
    checkVertex(polys[62], 1, 240, 99.4112f, -64);
    CHECK(polys[70].itemName == dx::Name("Top"));
    checkVertex(polys[70], 3, 255.9146f, 106.0033f, 80);
}

TEST_CASE("spiral stair sloped variants emit the sloped sections", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSpiralSloped");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "CeilingSloped", 1 }, { "SurfaceSloped", 1 } }), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 66);
    CHECK(countItem(polys, "Rise") == 2);
    CHECK(countItem(polys, "Ceiling") == 16);
    CHECK(countItem(polys, "Step") == 16);
    CHECK(polys[2].itemName == dx::Name("Ceiling"));
    REQUIRE(polys[2].vertices.size() == 3);
    checkVertex(polys[2], 0, 26.5097f, 64, -48);
    checkVertex(polys[2], 1, 99.4112f, 240, -48);
    checkVertex(polys[2], 2, 240, 99.4112f, -64);
    CHECK(polys[18].itemName == dx::Name("Step"));
    checkVertex(polys[18], 0, 240, 99.4112f, -32);
    checkVertex(polys[18], 2, 26.5097f, 64, -16);
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "CeilingSloped", 1 } }), brush, nullptr));
    CHECK(polysOf(brush).size() == 49);
    CHECK(countItem(polysOf(brush), "Rise") == 9);
}

TEST_CASE("spiral stair validation follows the VB form", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraSpiralErrors");
    std::string error;
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "CeilingSloped", 1 }, { "StepThickness", 8 } }), brush, &error));
    CHECK(error == "Step Thickness must be Equal to or Greater than the Step Height for Sloped Ceiling.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "SurfaceSloped", 1 } }), brush, &error));
    CHECK(error == "You Cannot have a Sloped Step with a Stepped Ceiling.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "InnerSRad", 240 } }), brush, &error));
    CHECK(error == "The InnerStair Radius must be Greater than the Outer Stair Radius.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "ExtFill", 1 }, { "OuterWallRadius", 240 } }), brush, &error));
    CHECK(error == "The Outer Wall Radius must be Greater than the Outer Stair Radius.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "NumSteps", 0 } }), brush, &error));
    CHECK(error == "You must use Positive non-zero values.");
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "CycleNum", 2 } }), brush, &error));
    CHECK(error == "You must have at least 3 Steps per 360 degrees.");
    CHECK(dx::brushbuilder::buildExtra(builderNamed("SpiralStair", { { "CycleNum", 3 }, { "StepHeight", 15 } }), brush, &error));
    CHECK(error == "Normal People will BUMP their heads on the steps above!");
    CHECK(polysOf(brush).size() == 48);
}

TEST_CASE("volumetric builder emits crossed two sided sheets", "[editor_builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ExtraVolumetric");
    REQUIRE(dx::brushbuilder::buildExtra(builderNamed("Volumetric", {}), brush, nullptr));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 2);
    CHECK(polys[0].itemName == dx::Name("Sheet"));
    CHECK(polys[0].polyFlags == (dx::bsp::PF_TwoSided | dx::bsp::PF_NotSolid));
    CHECK(polys[1].polyFlags == 0x108u);
    checkVertex(polys[0], 0, 64, 0, 64);
    checkVertex(polys[0], 1, 64, 0, -64);
    checkVertex(polys[0], 2, -64, 0, -64);
    checkVertex(polys[0], 3, -64, 0, 64);
    checkVertex(polys[1], 0, 0, 64, 64);
    checkVertex(polys[1], 2, 0, -64, -64);
    std::string error;
    CHECK_FALSE(dx::brushbuilder::buildExtra(builderNamed("Volumetric", { { "NumSheets", 1 } }), brush, &error));
    CHECK(error == "Bad parameters in brush builder");
}
