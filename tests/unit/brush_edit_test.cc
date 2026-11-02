#include "core/objects.h"
#include "engine/actor.h"
#include "engine/brush_edit.h"
#include "engine/bsp.h"
#include "engine/csg.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
    template <typename T>
    T* make(const char* className, dx::UObject* outer)
    {
        dx::UObject* object = dx::objects::newObject(dx::csg::nativeClass(className), outer, dx::Name(), 0);
        object->loaded = true;
        return dx::cast<T>(object);
    }

    dx::UObject* makeBrush(const char* packageName, float width, float depth, float height)
    {
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        dx::UObject* brush = make<dx::UObject>("Brush", package);
        dx::UModel* model = make<dx::UModel>("Model", package);
        model->polys = make<dx::UPolys>("Polys", package);
        dx::actor::Brush(brush) = model;
        dx::csg::buildCube(*model->polys, width, depth, height, nullptr);
        dx::csg::updateBounds(*model);
        return brush;
    }

    dx::UModel& modelOf(dx::UObject* brush)
    {
        return *dx::cast<dx::UModel>(dx::actor::Brush(brush));
    }

    dx::Vector windingNormal(const dx::Poly& poly)
    {
        dx::Vector sum;
        for (size_t i = 0; i < poly.vertices.size(); i++) {
            const dx::Vector& a = poly.vertices[i];
            const dx::Vector& b = poly.vertices[(i + 1) % poly.vertices.size()];
            sum += dx::Vector { (a.y - b.y) * (a.z + b.z), (a.z - b.z) * (a.x + b.x), (a.x - b.x) * (a.y + b.y) };
        }
        return sum.normal();
    }

    dx::Vector centerOf(const std::vector<dx::Vector>& points)
    {
        dx::Vector sum;
        for (const dx::Vector& point : points) {
            sum += point;
        }
        return sum / static_cast<float>(points.size());
    }

    void requireOutward(const std::vector<dx::Poly>& polys, const dx::Vector& inside)
    {
        for (const dx::Poly& poly : polys) {
            dx::Vector winding = windingNormal(poly);
            CHECK((centerOf(poly.vertices) - inside).dot(winding) > 0.0f);
            CHECK(winding.dot(poly.normal) == Catch::Approx(1.0f).margin(0.001));
        }
    }

    void requireBounds(const std::vector<dx::Poly>& polys, const dx::Vector& min, const dx::Vector& max)
    {
        dx::Vector low { 1e9f, 1e9f, 1e9f };
        dx::Vector high { -1e9f, -1e9f, -1e9f };
        for (const dx::Poly& poly : polys) {
            for (const dx::Vector& v : poly.vertices) {
                low = { std::min(low.x, v.x), std::min(low.y, v.y), std::min(low.z, v.z) };
                high = { std::max(high.x, v.x), std::max(high.y, v.y), std::max(high.z, v.z) };
            }
        }
        CHECK(low.x == Catch::Approx(min.x).margin(0.01));
        CHECK(low.y == Catch::Approx(min.y).margin(0.01));
        CHECK(low.z == Catch::Approx(min.z).margin(0.01));
        CHECK(high.x == Catch::Approx(max.x).margin(0.01));
        CHECK(high.y == Catch::Approx(max.y).margin(0.01));
        CHECK(high.z == Catch::Approx(max.z).margin(0.01));
    }

    float volumeOf(const std::vector<dx::Poly>& polys)
    {
        dx::Vector reference { 17.0f, 29.0f, -53.0f };
        float sum = 0.0f;
        for (const dx::Poly& poly : polys) {
            for (size_t i = 1; i + 1 < poly.vertices.size(); i++) {
                sum += (poly.vertices[0] - reference).dot((poly.vertices[i] - reference).cross(poly.vertices[i + 1] - reference));
            }
        }
        return std::fabs(sum) / 6.0f;
    }

    std::vector<dx::Vector> topFace()
    {
        return { { -128.0f, -128.0f, 128.0f }, { 128.0f, -128.0f, 128.0f }, { 128.0f, 128.0f, 128.0f }, { -128.0f, 128.0f, 128.0f } };
    }

    void shift(dx::UObject* brush, const dx::Vector& offset)
    {
        for (dx::Poly& poly : modelOf(brush).polys->polys) {
            for (dx::Vector& v : poly.vertices) {
                v += offset;
            }
            poly.base += offset;
        }
    }
}

TEST_CASE("mirroring a brush and transforming it permanently flips vertices and keeps outward winding", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditMirrorTest", 256.0f, 128.0f, 64.0f);
    shift(brush, { 100.0f, 0.0f, 0.0f });
    dx::brushedit::mirror(brush, 0);
    CHECK(dx::brushedit::mainScale(brush).x == -1.0f);
    std::vector<dx::Poly> world = dx::brushedit::worldPolys(brush);
    requireBounds(world, { -228.0f, -64.0f, -32.0f }, { 28.0f, 64.0f, 32.0f });
    requireOutward(world, { -100.0f, 0.0f, 0.0f });

    dx::brushedit::applyTransform(brush);
    CHECK(dx::brushedit::mainScale(brush).x == 1.0f);
    const std::vector<dx::Poly>& local = modelOf(brush).polys->polys;
    REQUIRE(local.size() == 6);
    requireBounds(local, { -228.0f, -64.0f, -32.0f }, { 28.0f, 64.0f, 32.0f });
    requireOutward(local, { -100.0f, 0.0f, 0.0f });
    CHECK(modelOf(brush).bounds.min.x == Catch::Approx(-228.0f).margin(0.01));
}

TEST_CASE("mirroring twice on two axes keeps the winding of every polygon outward", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditMirrorTwiceTest", 128.0f, 128.0f, 128.0f);
    shift(brush, { 0.0f, 50.0f, 20.0f });
    dx::brushedit::mirror(brush, 1);
    dx::brushedit::mirror(brush, 2);
    dx::brushedit::applyTransform(brush);
    const std::vector<dx::Poly>& local = modelOf(brush).polys->polys;
    requireBounds(local, { -64.0f, -114.0f, -84.0f }, { 64.0f, 14.0f, 44.0f });
    requireOutward(local, { 0.0f, -50.0f, -20.0f });
}

TEST_CASE("brush scale factors and rotation are baked into vertices and then reset", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditScaleTest", 256.0f, 128.0f, 64.0f);
    dx::brushedit::setMainScale(brush, { 2.0f, 1.0f, 0.5f });
    requireBounds(dx::brushedit::worldPolys(brush), { -256.0f, -64.0f, -16.0f }, { 256.0f, 64.0f, 16.0f });
    dx::actor::Rotation(brush) = { 0, 16384, 0 };
    dx::actor::Location(brush) = { 1000.0f, 0.0f, 0.0f };
    requireBounds(dx::brushedit::worldPolys(brush), { 936.0f, -256.0f, -16.0f }, { 1064.0f, 256.0f, 16.0f });

    dx::brushedit::applyTransform(brush);
    CHECK(dx::actor::Rotation(brush).yaw == 0);
    CHECK(dx::brushedit::mainScale(brush).x == 1.0f);
    CHECK(dx::brushedit::mainScale(brush).z == 1.0f);
    requireBounds(modelOf(brush).polys->polys, { -64.0f, -256.0f, -16.0f }, { 64.0f, 256.0f, 16.0f });
    requireBounds(dx::brushedit::worldPolys(brush), { 936.0f, -256.0f, -16.0f }, { 1064.0f, 256.0f, 16.0f });
    requireOutward(modelOf(brush).polys->polys, {});

    dx::brushedit::setMainScale(brush, { 3.0f, 3.0f, 3.0f });
    dx::brushedit::resetAll(brush);
    CHECK(dx::brushedit::mainScale(brush).y == 1.0f);
    CHECK(dx::actor::Location(brush).x == 0.0f);
}

TEST_CASE("setting and resetting the brush pivot keeps the brush in place", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditPivotTest", 256.0f, 128.0f, 64.0f);
    dx::actor::Location(brush) = { 100.0f, 0.0f, 0.0f };
    dx::brushedit::setPivot(brush, { 228.0f, 64.0f, 32.0f });
    CHECK(dx::actor::Location(brush).x == Catch::Approx(228.0f));
    CHECK(dx::actor::PrePivot(brush).x == Catch::Approx(128.0f));
    CHECK(dx::actor::PrePivot(brush).y == Catch::Approx(64.0f));
    CHECK(dx::actor::PrePivot(brush).z == Catch::Approx(32.0f));
    requireBounds(dx::brushedit::worldPolys(brush), { -28.0f, -64.0f, -32.0f }, { 228.0f, 64.0f, 32.0f });

    dx::actor::Rotation(brush) = { 0, 32768, 0 };
    requireBounds(dx::brushedit::worldPolys(brush), { 228.0f, 64.0f, -32.0f }, { 484.0f, 192.0f, 32.0f });

    dx::brushedit::resetPivot(brush);
    CHECK(dx::actor::PrePivot(brush).x == 0.0f);
    CHECK(dx::actor::Location(brush).x == Catch::Approx(356.0f).margin(0.01));
    CHECK(dx::actor::Location(brush).y == Catch::Approx(128.0f).margin(0.01));
    requireBounds(dx::brushedit::worldPolys(brush), { 228.0f, 64.0f, -32.0f }, { 484.0f, 192.0f, 32.0f });
}

TEST_CASE("snapping brush vertices moves them to the world grid", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditSnapTest", 250.0f, 250.0f, 100.0f);
    dx::actor::Location(brush) = { 4.0f, 0.0f, 0.0f };
    dx::brushedit::snapVertices(brush, { 16.0f, 16.0f, 16.0f });
    requireBounds(dx::brushedit::worldPolys(brush), { -128.0f, -128.0f, -48.0f }, { 128.0f, 128.0f, 48.0f });
    requireBounds(modelOf(brush).polys->polys, { -132.0f, -128.0f, -48.0f }, { 124.0f, 128.0f, 48.0f });
    CHECK(modelOf(brush).polys->polys.size() == 6);
}

TEST_CASE("dragging a brush vertex rebuilds non planar polygons as triangles", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditVertexTest", 256.0f, 256.0f, 256.0f);
    dx::UModel& model = modelOf(brush);
    CHECK(dx::brushedit::localVertices(model).size() == 8);
    CHECK(dx::brushedit::moveVertices(model, { { 128.0f, 128.0f, 128.0f } }, { 0.0f, 0.0f, 64.0f }) == 3);
    dx::brushedit::validate(model);
    const std::vector<dx::Poly>& polys = model.polys->polys;
    CHECK(polys.size() == 7);
    for (const dx::Poly& poly : polys) {
        for (const dx::Vector& v : poly.vertices) {
            CHECK(std::fabs((v - poly.vertices[0]).dot(poly.normal)) < 0.01f);
        }
    }
    requireOutward(polys, {});
    CHECK(model.bounds.max.z == Catch::Approx(192.0f));
    CHECK(dx::brushedit::localVertices(model).size() == 8);

    dx::UModel& edge = modelOf(makeBrush("BrushEditVertexEdgeTest", 256.0f, 256.0f, 256.0f));
    CHECK(dx::brushedit::moveVertices(edge, { { -128.0f, -128.0f, -128.0f }, { -128.0f, -128.0f, 128.0f } }, { 0.0f, 0.0f, 256.0f }) == 6);
    dx::brushedit::validate(edge);
    CHECK(edge.bounds.min.z == Catch::Approx(-128.0f));
    CHECK(edge.bounds.max.z == Catch::Approx(384.0f));
    requireOutward(edge.polys->polys, { 0.0f, 0.0f, 64.0f });
}

TEST_CASE("clipping a brush keeps the side behind the clip plane and caps it", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditClipTest", 256.0f, 256.0f, 256.0f);
    dx::UModel& model = modelOf(brush);
    REQUIRE(dx::brushedit::clipBrush(model, { 64.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }));
    CHECK(model.polys->polys.size() == 6);
    requireBounds(model.polys->polys, { -128.0f, -128.0f, -128.0f }, { 64.0f, 128.0f, 128.0f });
    requireOutward(model.polys->polys, { -32.0f, 0.0f, 0.0f });

    dx::UObject* other = makeBrush("BrushEditClipDiagonalTest", 256.0f, 256.0f, 256.0f);
    std::vector<dx::Poly> original = modelOf(other).polys->polys;
    dx::Vector diagonal = dx::Vector { 1.0f, 1.0f, 0.0f }.normal();
    std::vector<dx::Poly> back = dx::brushedit::clipPolys(original, {}, diagonal);
    std::vector<dx::Poly> front = dx::brushedit::clipPolys(original, {}, diagonal * -1.0f);
    CHECK(back.size() == 5);
    CHECK(front.size() == 5);
    requireOutward(back, { -64.0f, -64.0f, 0.0f });
    requireOutward(front, { 64.0f, 64.0f, 0.0f });
    requireBounds(back, { -128.0f, -128.0f, -128.0f }, { 128.0f, 128.0f, 128.0f });
    CHECK_FALSE(dx::brushedit::clipBrush(modelOf(other), { 200.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }));
    CHECK(modelOf(other).polys->polys.size() == 6);
}

TEST_CASE("clipping a concave brush only caps the solid part of the cut", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditClipConcaveTest", 256.0f, 256.0f, 256.0f);
    dx::UModel& model = modelOf(brush);
    std::vector<dx::Vector> profile = { { 0, 0, 0 }, { 256, 0, 0 }, { 256, 64, 0 }, { 64, 64, 0 }, { 64, 256, 0 }, { 0, 256, 0 } };
    std::vector<dx::Poly> polys;
    dx::Poly bottom;
    dx::Poly top;
    for (size_t i = 0; i < profile.size(); i++) {
        bottom.vertices.push_back(profile[profile.size() - 1 - i]);
        top.vertices.push_back(profile[i] + dx::Vector { 0, 0, 64 });
        dx::Poly side;
        const dx::Vector& a = profile[i];
        const dx::Vector& b = profile[(i + 1) % profile.size()];
        side.vertices = { a, b, b + dx::Vector { 0, 0, 64 }, a + dx::Vector { 0, 0, 64 } };
        polys.push_back(side);
    }
    polys.push_back(bottom);
    polys.push_back(top);
    for (dx::Poly& poly : polys) {
        dx::bsp::calcNormal(poly);
        poly.base = poly.vertices[0];
    }
    model.polys->polys = polys;
    dx::Vector diagonal = dx::Vector { 1.0f, 1.0f, 0.0f }.normal();
    REQUIRE(dx::brushedit::clipBrush(model, { 100.0f, 100.0f, 0.0f }, diagonal));
    requireBounds(model.polys->polys, { 0.0f, 0.0f, 0.0f }, { 200.0f, 200.0f, 64.0f });
    float capArea = 0.0f;
    for (const dx::Poly& poly : model.polys->polys) {
        if (poly.normal.dot(diagonal) > 0.99f) {
            for (size_t i = 1; i + 1 < poly.vertices.size(); i++) {
                capArea += (poly.vertices[i] - poly.vertices[0]).cross(poly.vertices[i + 1] - poly.vertices[0]).size() * 0.5f;
            }
        }
    }
    CHECK(capArea == Catch::Approx(2.0f * 64.0f * std::sqrt(2.0f) * 64.0f).margin(1.0));
}

TEST_CASE("two clip markers in an orthographic view define the clip plane in brush space", "[data]")
{
    dx::UObject* brush = makeBrush("BrushEditClipPlaneTest", 256.0f, 256.0f, 256.0f);
    dx::Vector base;
    dx::Vector normal;
    std::vector<dx::Vector> markers = { { 0.0f, -100.0f, 0.0f }, { 0.0f, 100.0f, 0.0f } };
    REQUIRE(dx::brushedit::clipPlane(brush, markers, 2, base, normal));
    CHECK(normal.x == Catch::Approx(-1.0f));
    std::vector<dx::Vector> flipped = { markers[1], markers[0] };
    REQUIRE(dx::brushedit::clipPlane(brush, flipped, 2, base, normal));
    CHECK(normal.x == Catch::Approx(1.0f));

    dx::actor::Location(brush) = { 32.0f, 0.0f, 0.0f };
    dx::brushedit::mirror(brush, 0);
    REQUIRE(dx::brushedit::clipPlane(brush, markers, 2, base, normal));
    CHECK(normal.x == Catch::Approx(1.0f));
    CHECK(base.x == Catch::Approx(32.0f));
    CHECK_FALSE(dx::brushedit::clipPlane(brush, { markers[0] }, 2, base, normal));
}

TEST_CASE("mover keyframes store positions relative to the base and show each key", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UClass* moverClass = dx::objects::findClass("Engine.Mover");
    REQUIRE(moverClass);
    dx::UObject* mover = dx::objects::newObject(moverClass, dx::objects::packageObject(dx::Name("BrushEditMoverTest")), dx::Name(), 0);
    mover->loaded = true;
    dx::actor::Location(mover) = { 100.0f, 0.0f, 0.0f };
    dx::actor::Rotation(mover) = { 0, 1024, 0 };
    dx::brushedit::moverMoved(mover);
    CHECK(dx::mover::BasePos(mover).x == 100.0f);
    CHECK(dx::mover::BaseRot(mover).yaw == 1024);

    dx::brushedit::showKey(mover, 1);
    CHECK(dx::mover::KeyNum(mover) == 1);
    CHECK(dx::actor::Location(mover).x == 100.0f);
    dx::actor::Location(mover) = { 100.0f, 32.0f, 64.0f };
    dx::actor::Rotation(mover) = { 0, 1024 + 16384, 0 };
    dx::brushedit::moverMoved(mover);
    CHECK(dx::mover::KeyPos(mover, 1).y == 32.0f);
    CHECK(dx::mover::KeyPos(mover, 1).z == 64.0f);
    CHECK(dx::mover::KeyRot(mover, 1).yaw == 16384);
    CHECK(dx::mover::BasePos(mover).x == 100.0f);

    dx::brushedit::showKey(mover, 0);
    CHECK(dx::actor::Location(mover).y == 0.0f);
    CHECK(dx::actor::Location(mover).z == 0.0f);
    CHECK(dx::actor::Rotation(mover).yaw == 1024);
    CHECK(dx::mover::BasePos(mover).x == 100.0f);

    dx::actor::Location(mover) = { 200.0f, 0.0f, 0.0f };
    dx::brushedit::moverMoved(mover);
    dx::brushedit::showKey(mover, 1);
    CHECK(dx::actor::Location(mover).x == 200.0f);
    CHECK(dx::actor::Location(mover).y == 32.0f);
    CHECK(dx::actor::Rotation(mover).yaw == 1024 + 16384);

    dx::brushedit::showKey(mover, 12);
    CHECK(dx::mover::KeyNum(mover) == 7);
}

TEST_CASE("brush edges are listed once each and faces are found by their vertices in any order", "[data]")
{
    dx::UModel& model = modelOf(makeBrush("BrushEditEdgesTest", 256.0f, 256.0f, 256.0f));
    CHECK(dx::brushedit::localEdges(model).size() == 12);
    int32_t top = dx::brushedit::findPoly(model, topFace());
    REQUIRE(top >= 0);
    CHECK(model.polys->polys[static_cast<size_t>(top)].normal.z == Catch::Approx(1.0f));
    std::vector<dx::Vector> reordered = { topFace()[2], topFace()[0], topFace()[3], topFace()[1] };
    CHECK(dx::brushedit::findPoly(model, reordered) == top);
    CHECK(dx::brushedit::findPoly(model, { topFace()[0], topFace()[1], topFace()[2] }) == -1);
}

TEST_CASE("extruding a face outward adds side faces and grows the brush by the swept volume", "[data]")
{
    dx::UModel& model = modelOf(makeBrush("BrushEditExtrudeTest", 256.0f, 256.0f, 256.0f));
    size_t top = static_cast<size_t>(dx::brushedit::findPoly(model, topFace()));
    REQUIRE(dx::brushedit::extrudeFace(model, top, { 0.0f, 0.0f, 64.0f }));
    requireBounds(model.polys->polys, { -128.0f, -128.0f, -128.0f }, { 128.0f, 128.0f, 192.0f });
    requireOutward(model.polys->polys, {});
    CHECK(volumeOf(model.polys->polys) == Catch::Approx(256.0f * 256.0f * 320.0f).margin(1.0));
    CHECK(model.polys->polys.size() == 6);
    CHECK(dx::brushedit::localVertices(model).size() == 8);
    CHECK(model.bounds.max.z == Catch::Approx(192.0f));

    dx::UModel& sheared = modelOf(makeBrush("BrushEditExtrudeShearTest", 256.0f, 256.0f, 256.0f));
    size_t face = static_cast<size_t>(dx::brushedit::findPoly(sheared, topFace()));
    REQUIRE(dx::brushedit::extrudeFace(sheared, face, { 64.0f, 0.0f, 64.0f }));
    requireBounds(sheared.polys->polys, { -128.0f, -128.0f, -128.0f }, { 192.0f, 128.0f, 192.0f });
    for (const dx::Poly& poly : sheared.polys->polys) {
        CHECK(windingNormal(poly).dot(poly.normal) == Catch::Approx(1.0f).margin(0.001));
    }
    CHECK(volumeOf(sheared.polys->polys) == Catch::Approx(256.0f * 256.0f * 320.0f).margin(1.0));
    std::vector<dx::Vector> shearedVertices = dx::brushedit::localVertices(sheared);
    CHECK(std::any_of(shearedVertices.begin(), shearedVertices.end(), [](const dx::Vector& v) {
        return (v - dx::Vector { 128.0f, -128.0f, 128.0f }).sizeSquared() < 0.01f;
    }));
    CHECK(dx::brushedit::findPoly(sheared, { { -64.0f, -128.0f, 192.0f }, { 192.0f, -128.0f, 192.0f }, { 192.0f, 128.0f, 192.0f }, { -64.0f, 128.0f, 192.0f } }) >= 0);
}

TEST_CASE("extruding a face inward moves it and shrinks the brush", "[data]")
{
    dx::UModel& model = modelOf(makeBrush("BrushEditExtrudeInwardTest", 256.0f, 256.0f, 256.0f));
    size_t top = static_cast<size_t>(dx::brushedit::findPoly(model, topFace()));
    REQUIRE(dx::brushedit::extrudeFace(model, top, { 0.0f, 0.0f, -64.0f }));
    CHECK(model.polys->polys.size() == 6);
    requireBounds(model.polys->polys, { -128.0f, -128.0f, -128.0f }, { 128.0f, 128.0f, 64.0f });
    requireOutward(model.polys->polys, {});
    CHECK(volumeOf(model.polys->polys) == Catch::Approx(256.0f * 256.0f * 192.0f).margin(1.0));
    CHECK_FALSE(dx::brushedit::extrudeFace(model, top, {}));
    CHECK_FALSE(dx::brushedit::extrudeFace(model, 99, { 0.0f, 0.0f, 64.0f }));
}

TEST_CASE("transforming selected vertices scales a face around its center into a frustum", "[data]")
{
    dx::UModel& model = modelOf(makeBrush("BrushEditTransformTest", 256.0f, 256.0f, 256.0f));
    dx::Vector center { 0.0f, 0.0f, 128.0f };
    size_t moved = dx::brushedit::transformVertices(model, topFace(), [&](const dx::Vector& vertex) {
        return center + (vertex - center) * 0.5f;
    });
    CHECK(moved == 12);
    dx::brushedit::validate(model);
    requireOutward(model.polys->polys, {});
    std::vector<dx::Vector> vertices = dx::brushedit::localVertices(model);
    CHECK(vertices.size() == 8);
    CHECK(std::any_of(vertices.begin(), vertices.end(), [](const dx::Vector& v) {
        return (v - dx::Vector { 64.0f, 64.0f, 128.0f }).sizeSquared() < 0.01f;
    }));
    CHECK(volumeOf(model.polys->polys) == Catch::Approx(256.0f * (256.0f * 256.0f + 256.0f * 128.0f + 128.0f * 128.0f) / 3.0f).margin(1.0));
}

TEST_CASE("a ray picks the nearest brush polygon from outside and from inside", "[data]")
{
    dx::UModel& model = modelOf(makeBrush("BrushEditRayTest", 256.0f, 256.0f, 256.0f));
    const std::vector<dx::Poly>& polys = model.polys->polys;
    float distance = 0.0f;
    int32_t hit = dx::brushedit::rayPoly(polys, { 10.0f, 20.0f, 1000.0f }, { 0.0f, 0.0f, -1.0f }, distance);
    REQUIRE(hit >= 0);
    CHECK(polys[static_cast<size_t>(hit)].normal.z == Catch::Approx(1.0f));
    CHECK(distance == Catch::Approx(872.0f));

    hit = dx::brushedit::rayPoly(polys, {}, { 1.0f, 0.0f, 0.0f }, distance);
    REQUIRE(hit >= 0);
    CHECK(polys[static_cast<size_t>(hit)].normal.x == Catch::Approx(1.0f));
    CHECK(distance == Catch::Approx(128.0f));

    CHECK(dx::brushedit::rayPoly(polys, { 300.0f, 0.0f, 1000.0f }, { 0.0f, 0.0f, -1.0f }, distance) == -1);
}

TEST_CASE("extruding a face only merges the new side faces and leaves other coplanar faces split", "[data]")
{
    dx::UModel& model = modelOf(makeBrush("BrushEditExtrudeSplitTest", 256.0f, 256.0f, 256.0f));
    std::vector<dx::Poly>& polys = model.polys->polys;
    auto bottom = std::find_if(polys.begin(), polys.end(), [](const dx::Poly& poly) {
        return poly.normal.z < -0.99f;
    });
    REQUIRE(bottom != polys.end());
    dx::Poly half = *bottom;
    half.vertices = { bottom->vertices[0], bottom->vertices[2], bottom->vertices[3] };
    bottom->vertices = { bottom->vertices[0], bottom->vertices[1], bottom->vertices[2] };
    polys.push_back(half);
    REQUIRE(polys.size() == 7);
    size_t top = static_cast<size_t>(dx::brushedit::findPoly(model, topFace()));
    REQUIRE(dx::brushedit::extrudeFace(model, top, { 0.0f, 0.0f, 64.0f }));
    size_t bottoms = static_cast<size_t>(std::count_if(model.polys->polys.begin(), model.polys->polys.end(), [](const dx::Poly& poly) {
        return poly.normal.z < -0.99f;
    }));
    CHECK(bottoms == 2);
    CHECK(model.polys->polys.size() == 7);
    CHECK(volumeOf(model.polys->polys) == Catch::Approx(256.0f * 256.0f * 320.0f).margin(1.0));
    requireOutward(model.polys->polys, {});
}
