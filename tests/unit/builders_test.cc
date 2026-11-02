#include "core/objects.h"
#include "engine/actor.h"
#include "engine/bsp.h"
#include "engine/csg.h"
#include "engine/extra_builders.h"
#include "engine/floor_lofter.h"
#include "engine/props_engine.h"
#include "engine/resources.h"
#include "engine/shape_editor.h"
#include "engine/special_brush.h"
#include "engine/world.h"

#include <catch_amalgamated.hpp>

#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

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

    dx::brushbuilder::ExtraBuilder tube(std::initializer_list<std::pair<const char*, double>> overrides)
    {
        for (dx::brushbuilder::ExtraBuilder& builder : dx::brushbuilder::extraBuilders()) {
            if (builder.name != "Tube") {
                continue;
            }
            for (const std::pair<const char*, double>& item : overrides) {
                for (dx::brushbuilder::ExtraParameter& parameter : builder.parameters) {
                    if (parameter.name == item.first) {
                        parameter.value = item.second;
                    }
                }
            }
            return builder;
        }
        FAIL("missing Tube builder");
        return {};
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

    dx::Vector centroid(const dx::Poly& poly)
    {
        dx::Vector sum;
        for (const dx::Vector& v : poly.vertices) {
            sum += v;
        }
        return sum / static_cast<float>(poly.vertices.size());
    }

    dx::Vector faceNormal(const dx::Poly& poly)
    {
        dx::Vector normal;
        for (size_t i = 2; i < poly.vertices.size(); i++) {
            normal += (poly.vertices[i - 1] - poly.vertices[0]).cross(poly.vertices[i] - poly.vertices[0]);
        }
        return normal;
    }

    bool hasVertex(const dx::Poly& poly, float x, float y, float z)
    {
        for (const dx::Vector& v : poly.vertices) {
            if (std::abs(v.x - x) < 0.01f && std::abs(v.y - y) < 0.01f && std::abs(v.z - z) < 0.01f) {
                return true;
            }
        }
        return false;
    }

    void checkFacesPointAwayFrom(const std::vector<dx::Poly>& polys, const dx::Vector& inside)
    {
        for (const dx::Poly& poly : polys) {
            CHECK(faceNormal(poly).dot(centroid(poly) - inside) > 0);
        }
    }

    void writeBytes(const std::filesystem::path& file, const std::vector<uint8_t>& bytes)
    {
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    void put16(std::vector<uint8_t>& bytes, uint32_t value)
    {
        bytes.push_back(static_cast<uint8_t>(value & 0xff));
        bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    }

    void put32(std::vector<uint8_t>& bytes, uint32_t value)
    {
        put16(bytes, value & 0xffff);
        put16(bytes, value >> 16);
    }

    void putFloat(std::vector<uint8_t>& bytes, float value)
    {
        uint32_t raw = 0;
        std::memcpy(&raw, &value, 4);
        put32(bytes, raw);
    }

    std::filesystem::path scratch(const char* name)
    {
        std::filesystem::path directory = std::filesystem::temp_directory_path() / "opende-builders-test";
        std::filesystem::create_directories(directory);
        return directory / name;
    }

    double polygonArea(const std::vector<dx::shapeeditor::Point>& points, const std::vector<std::array<size_t, 3>>& triangles)
    {
        double total = 0;
        for (const std::array<size_t, 3>& t : triangles) {
            const dx::shapeeditor::Point& a = points[t[0]];
            const dx::shapeeditor::Point& b = points[t[1]];
            const dx::shapeeditor::Point& c = points[t[2]];
            total += ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) / 2;
        }
        return total;
    }

    dx::shapeeditor::Document lShape()
    {
        dx::shapeeditor::Document document;
        document.shapes.push_back({ { { 0, 0 }, { 0, 128 }, { 64, 128 }, { 64, 64 }, { 128, 64 }, { 128, 0 } } });
        return document;
    }
}

TEST_CASE("tube builder is listed with the PsTube form defaults", "[builders]")
{
    dx::brushbuilder::ExtraBuilder builder = tube({});
    REQUIRE(builder.parameters.size() == 6);
    CHECK(builder.parameters[0].name == "OuterRadius");
    CHECK(builder.parameters[0].value == 512);
    CHECK(builder.parameters[1].value == 496);
    CHECK(builder.parameters[2].value == 256);
    CHECK(builder.parameters[3].value == 8);
    CHECK(builder.parameters[4].value == 0);
    CHECK(builder.parameters[5].value == 1);
}

TEST_CASE("solid tube default matches PsTube", "[builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("TubeSolidDefault");
    std::string error;
    REQUIRE(dx::brushbuilder::buildExtra(tube({}), brush, &error));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 10);
    CHECK(dx::actor::Group(brush) == dx::Name("TUBE"));
    CHECK(countItem(polys, "Outer") == 8);
    CHECK(countItem(polys, "Base") == 1);
    CHECK(countItem(polys, "Top") == 1);
    checkVertex(polys[0], 0, 212.0773f, 512, -128);
    checkVertex(polys[0], 1, 212.0773f, 512, 128);
    checkVertex(polys[0], 2, 512, 212.0773f, 128);
    checkVertex(polys[0], 3, 512, 212.0773f, -128);
    REQUIRE(polys[8].vertices.size() == 8);
    checkVertex(polys[8], 0, 512, -212.0773f, -128);
    checkVertex(polys[8], 7, 512, 212.0773f, -128);
    checkVertex(polys[9], 0, 512, 212.0773f, 128);
    checkFacesPointAwayFrom(polys, {});
}

TEST_CASE("hollow tube has inner walls facing the axis and ring caps", "[builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("TubeHollow");
    std::string error;
    REQUIRE(dx::brushbuilder::buildExtra(tube({ { "Hollow", 1 } }), brush, &error));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 32);
    CHECK(countItem(polys, "Outer") == 8);
    CHECK(countItem(polys, "Inner") == 8);
    CHECK(countItem(polys, "Bottom") == 8);
    CHECK(countItem(polys, "Top") == 8);
    REQUIRE(polys[16].itemName == dx::Name("Bottom"));
    checkVertex(polys[16], 0, 512, 212.0773f, -128);
    checkVertex(polys[16], 1, 496, 205.45f, -128);
    for (const dx::Poly& poly : polys) {
        dx::Vector c = centroid(poly);
        dx::Vector radial { c.x, c.y, 0 };
        dx::Vector n = faceNormal(poly);
        if (poly.itemName == dx::Name("Inner")) {
            CHECK(n.dot(radial) < 0);
        } else if (poly.itemName == dx::Name("Outer")) {
            CHECK(n.dot(radial) > 0);
        } else if (poly.itemName == dx::Name("Top")) {
            CHECK(n.z > 0);
        } else {
            CHECK(n.z < 0);
        }
    }
    CHECK_FALSE(dx::brushbuilder::buildExtra(tube({ { "Hollow", 1 }, { "InnerRadius", 600 } }), brush, &error));
    CHECK(error == "The Inner Radius must be smaller than the Outer Radius");
}

TEST_CASE("solid tube with many sides splits its caps into fans around the centre", "[builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("TubeFans");
    std::string error;
    REQUIRE(dx::brushbuilder::buildExtra(tube({ { "Sides", 16 }, { "AlignSide", 0 } }), brush, &error));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 24);
    CHECK(countItem(polys, "Base") == 8);
    for (size_t i = 16; i < 24; i++) {
        CHECK(hasVertex(polys[i], 0, 0, i < 20 ? -128.0f : 128.0f));
    }
    CHECK(polys[16].vertices.size() == 7);
    CHECK(polys[19].vertices.size() == 3);
    checkFacesPointAwayFrom(polys, {});
}

TEST_CASE("floor lofter builds a closed slab from a flat grid", "[builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("FloorFlat");
    dx::brushbuilder::FloorGrid grid;
    dx::brushbuilder::resizeFloor(grid);
    std::string error;
    REQUIRE(dx::brushbuilder::buildFloor(grid, brush, &error));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 33);
    CHECK(dx::actor::Group(brush) == dx::Name("Floor"));
    CHECK(countItem(polys, "FloorTOP") == 16);
    CHECK(countItem(polys, "FloorXSIDE1") == 4);
    CHECK(countItem(polys, "FloorYSIDE2") == 4);
    checkVertex(polys[0], 0, -128, -128, 0);
    checkVertex(polys[0], 1, -64, -128, 0);
    checkVertex(polys[0], 2, -64, -64, 0);
    checkVertex(polys[0], 3, -128, -64, 0);
    REQUIRE(polys[32].itemName == dx::Name("FloorBOTTOM"));
    checkVertex(polys[32], 0, -128, 128, -64);
    checkVertex(polys[32], 2, 128, -128, -64);
    checkFacesPointAwayFrom(polys, { 0, 0, -32 });
}

TEST_CASE("floor lofter splits raised cells along the chosen diagonal", "[builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("FloorRaised");
    dx::brushbuilder::FloorGrid grid;
    grid.sizeX = 2;
    grid.sizeY = 1;
    dx::brushbuilder::resizeFloor(grid);
    dx::brushbuilder::floorHeight(grid, 1, 1) = 32;
    grid.reversed[1] = 1;
    std::string error;
    REQUIRE(dx::brushbuilder::buildFloor(grid, brush, &error));
    std::vector<dx::Poly>& polys = polysOf(brush);
    CHECK(countItem(polys, "FloorTOP") == 4);
    REQUIRE(polys.size() == 4 + 2 + 2 + 2 + 1);
    checkVertex(polys[0], 0, -64, -32, 0);
    checkVertex(polys[0], 1, 0, -32, 0);
    checkVertex(polys[0], 2, 0, 32, 32);
    checkVertex(polys[1], 2, -64, 32, 0);
    checkVertex(polys[2], 0, 64, -32, 0);
    checkVertex(polys[2], 1, 64, 32, 0);
    checkVertex(polys[2], 2, 0, 32, 32);
    checkVertex(polys[3], 2, 0, -32, 0);
    for (size_t i = 0; i < 4; i++) {
        CHECK(faceNormal(polys[i]).z > 0);
    }
    checkFacesPointAwayFrom(std::vector<dx::Poly>(polys.begin() + 4, polys.end()), { 0, 0, -32 });
    dx::brushbuilder::floorHeight(grid, 0, 0) = -64;
    CHECK_FALSE(dx::brushbuilder::buildFloor(grid, brush, &error));
}

TEST_CASE("height maps load from greyscale BMP and PCX files", "[builders]")
{
    std::vector<uint8_t> bmp { 'B', 'M' };
    put32(bmp, 54 + 1024 + 8);
    put32(bmp, 0);
    put32(bmp, 54 + 1024);
    put32(bmp, 40);
    put32(bmp, 3);
    put32(bmp, 2);
    put16(bmp, 1);
    put16(bmp, 8);
    put32(bmp, 0);
    put32(bmp, 8);
    put32(bmp, 0);
    put32(bmp, 0);
    put32(bmp, 256);
    put32(bmp, 256);
    for (uint32_t i = 0; i < 256; i++) {
        uint8_t grey = static_cast<uint8_t>(255 - i);
        bmp.insert(bmp.end(), { grey, grey, grey, 0 });
    }
    bmp.insert(bmp.end(), { 255, 254, 253, 0, 0, 1, 2, 0 });
    std::filesystem::path bmpFile = scratch("height.bmp");
    writeBytes(bmpFile, bmp);
    dx::brushbuilder::GreyImage image;
    std::string error;
    REQUIRE(dx::brushbuilder::loadGreyImage(bmpFile, image, &error));
    CHECK(image.width == 3);
    CHECK(image.height == 2);
    CHECK(image.pixels == std::vector<uint8_t> { 255, 254, 253, 0, 1, 2 });

    std::vector<uint8_t> pcx { 10, 5, 1, 8 };
    put16(pcx, 0);
    put16(pcx, 0);
    put16(pcx, 2);
    put16(pcx, 1);
    pcx.resize(65, 0);
    pcx.push_back(1);
    put16(pcx, 4);
    pcx.resize(128, 0);
    pcx.insert(pcx.end(), { 0xc3, 40, 0, 0xc4, 200 });
    pcx.push_back(12);
    for (int i = 0; i < 256; i++) {
        uint8_t grey = static_cast<uint8_t>(i);
        pcx.insert(pcx.end(), { grey, grey, grey });
    }
    std::filesystem::path pcxFile = scratch("height.pcx");
    writeBytes(pcxFile, pcx);
    REQUIRE(dx::brushbuilder::loadGreyImage(pcxFile, image, &error));
    CHECK(image.width == 3);
    CHECK(image.height == 2);
    CHECK(image.pixels == std::vector<uint8_t> { 40, 40, 40, 200, 200, 200 });
}

TEST_CASE("height map builder raises grid vertices by the scaled pixel values", "[builders][data]")
{
    dx::brushbuilder::GreyImage image;
    image.width = 3;
    image.height = 2;
    image.pixels = { 0, 10, 0, 0, 0, 5 };
    dx::brushbuilder::FloorGrid grid = dx::brushbuilder::heightMapGrid(image, 32, 2, 16);
    CHECK(grid.sizeX == 2);
    CHECK(grid.sizeY == 1);
    dx::UObject* brush = makeBuilderBrush("HeightMapBuild");
    std::string error;
    REQUIRE(dx::brushbuilder::buildFloor(grid, brush, &error));
    std::vector<dx::Poly>& polys = polysOf(brush);
    bool raised = false;
    bool lower = false;
    for (const dx::Poly& poly : polys) {
        raised = raised || hasVertex(poly, 0, -16, 20);
        lower = lower || hasVertex(poly, 32, 16, 10);
        CHECK_FALSE(hasVertex(poly, 0, -16, 0));
    }
    CHECK(raised);
    CHECK(lower);
    CHECK(hasVertex(polys.back(), -32, -16, -16));
    CHECK(countItem(polys, "FloorTOP") == 4);
}

TEST_CASE("shape triangulation covers concave shapes and rejects crossing sides", "[builders]")
{
    std::vector<std::array<size_t, 3>> triangles;
    std::string error;
    std::vector<dx::shapeeditor::Point> square = dx::shapeeditor::defaultShape().vertices;
    REQUIRE(dx::shapeeditor::triangulate(square, triangles, &error));
    CHECK(triangles.size() == 2);
    CHECK(std::abs(polygonArea(square, triangles)) == Catch::Approx(256.0 * 256.0));
    std::vector<dx::shapeeditor::Point> l = lShape().shapes[0].vertices;
    REQUIRE(dx::shapeeditor::triangulate(l, triangles, &error));
    CHECK(triangles.size() == 4);
    CHECK(std::abs(polygonArea(l, triangles)) == Catch::Approx(128.0 * 128.0 - 64.0 * 64.0));
    std::vector<dx::shapeeditor::Point> bowTie { { 0, 0 }, { 64, 64 }, { 64, 0 }, { 0, 64 } };
    CHECK_FALSE(dx::shapeeditor::triangulate(bowTie, triangles, &error));
    CHECK(error == "The sides of a shape cross each other");
}

TEST_CASE("shape vertices can be split, deleted and snapped", "[builders]")
{
    dx::shapeeditor::Shape shape = dx::shapeeditor::defaultShape();
    dx::shapeeditor::splitSide(shape, 0, 16);
    REQUIRE(shape.vertices.size() == 5);
    CHECK(shape.vertices[1].x == -128);
    CHECK(shape.vertices[1].y == 0);
    dx::shapeeditor::Point snapped = dx::shapeeditor::snap({ 21, -9 }, 16);
    CHECK(snapped.x == 16);
    CHECK(snapped.y == -16);
    CHECK(dx::shapeeditor::deleteVertex(shape, 1));
    CHECK(dx::shapeeditor::deleteVertex(shape, 0));
    CHECK(shape.vertices.size() == 3);
    CHECK_FALSE(dx::shapeeditor::deleteVertex(shape, 0));
}

TEST_CASE("extruding the default shape makes a closed box around the origin", "[builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ShapeExtrude");
    std::string error;
    REQUIRE(dx::shapeeditor::extrude(dx::shapeeditor::newDocument(), 64, brush, &error));
    std::vector<dx::Poly>& polys = polysOf(brush);
    REQUIRE(polys.size() == 6);
    CHECK(dx::actor::Group(brush) == dx::Name("2DLoft"));
    CHECK(countItem(polys, "2DLoftSIDE") == 4);
    checkFacesPointAwayFrom(polys, {});
    for (const dx::Poly& poly : polys) {
        for (const dx::Vector& v : poly.vertices) {
            CHECK(std::abs(v.x) == 128);
            CHECK(std::abs(v.y) == 128);
            CHECK(std::abs(v.z) == 32);
        }
    }
    dx::shapeeditor::Document moved = dx::shapeeditor::newDocument();
    moved.origin = { 128, 128 };
    REQUIRE(dx::shapeeditor::extrude(moved, 64, brush, &error));
    for (const dx::Poly& poly : polysOf(brush)) {
        for (const dx::Vector& v : poly.vertices) {
            CHECK(v.x <= 0);
            CHECK(v.x >= -256);
            CHECK(v.y >= 0);
            CHECK(v.y <= 256);
        }
    }
}

TEST_CASE("extruding a concave shape keeps every face convex and outward", "[builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ShapeExtrudeL");
    std::string error;
    dx::shapeeditor::Document document = lShape();
    REQUIRE(dx::shapeeditor::extrude(document, 32, brush, &error));
    std::vector<dx::Poly>& polys = polysOf(brush);
    CHECK(countItem(polys, "2DLoftSIDE") == 6);
    double topArea = 0;
    for (const dx::Poly& poly : polys) {
        dx::Vector n = faceNormal(poly);
        if (poly.itemName == dx::Name("2DLoftTOP")) {
            CHECK(n.z > 0);
            topArea += n.size() / 2;
        } else if (poly.itemName == dx::Name("2DLoftEND")) {
            CHECK(n.z < 0);
        }
        for (size_t i = 0; i < poly.vertices.size(); i++) {
            const dx::Vector& a = poly.vertices[i];
            const dx::Vector& b = poly.vertices[(i + 1) % poly.vertices.size()];
            const dx::Vector& c = poly.vertices[(i + 2) % poly.vertices.size()];
            CHECK((b - a).cross(c - b).dot(n) >= -0.001f);
        }
    }
    CHECK(topArea == Catch::Approx(128.0 * 128.0 - 64.0 * 64.0));
    for (const dx::Poly& poly : polys) {
        if (poly.itemName != dx::Name("2DLoftSIDE")) {
            continue;
        }
        dx::Vector n = faceNormal(poly);
        dx::Vector c = centroid(poly);
        bool outerX = std::abs(c.x) < 0.01f && n.x < 0;
        bool outerY = std::abs(c.y) < 0.01f && n.y > 0;
        bool farX = std::abs(c.x - 128) < 0.01f && n.x > 0;
        bool farY = std::abs(c.y + 128) < 0.01f && n.y < 0;
        bool notchX = std::abs(c.x - 64) < 0.01f && n.x > 0;
        bool notchY = std::abs(c.y + 64) < 0.01f && n.y < 0;
        CHECK((outerX || outerY || farX || farY || notchX || notchY));
    }
}

TEST_CASE("extrude to point and to bevel taper the shape upwards", "[builders][data]")
{
    dx::UObject* brush = makeBuilderBrush("ShapeTaper");
    std::string error;
    REQUIRE(dx::shapeeditor::extrudeToPoint(dx::shapeeditor::newDocument(), 256, brush, &error));
    std::vector<dx::Poly>& pyramid = polysOf(brush);
    REQUIRE(pyramid.size() == 5);
    CHECK(countItem(pyramid, "2DLoftSIDE") == 4);
    for (const dx::Poly& poly : pyramid) {
        if (poly.itemName == dx::Name("2DLoftSIDE")) {
            CHECK(poly.vertices.size() == 3);
            CHECK(hasVertex(poly, 0, 0, 256));
        }
    }
    checkFacesPointAwayFrom(pyramid, { 0, 0, 64 });
    REQUIRE(dx::shapeeditor::extrudeToBevel(dx::shapeeditor::newDocument(), 128, 32, brush, &error));
    std::vector<dx::Poly>& bevel = polysOf(brush);
    REQUIRE(bevel.size() == 6);
    bool top = false;
    for (const dx::Poly& poly : bevel) {
        if (poly.itemName == dx::Name("2DLoftTOP")) {
            top = true;
            CHECK(hasVertex(poly, 96, 96, 32));
            CHECK(hasVertex(poly, -96, -96, 32));
        }
    }
    CHECK(top);
    checkFacesPointAwayFrom(bevel, { 0, 0, 16 });
    CHECK_FALSE(dx::shapeeditor::extrudeToBevel(dx::shapeeditor::newDocument(), 128, 128, brush, &error));
}

TEST_CASE("revolving a shape sweeps its sides around the origin", "[builders][data]")
{
    dx::shapeeditor::Document document;
    document.shapes.push_back({ { { 64, -64 }, { 64, 64 }, { 128, 64 }, { 128, -64 } } });
    dx::UObject* brush = makeBuilderBrush("ShapeRevolve");
    std::string error;
    REQUIRE(dx::shapeeditor::revolve(document, 8, 8, brush, &error));
    std::vector<dx::Poly>& ring = polysOf(brush);
    REQUIRE(ring.size() == 32);
    for (const dx::Poly& poly : ring) {
        dx::Vector c = centroid(poly);
        double angle = std::atan2(c.z, c.x);
        dx::Vector core { static_cast<float>(96 * std::cos(angle)), 0, static_cast<float>(96 * std::sin(angle)) };
        CHECK(faceNormal(poly).dot(c - core) > 0);
        for (const dx::Vector& v : poly.vertices) {
            float radius = std::sqrt(v.x * v.x + v.z * v.z);
            bool onCircle = std::abs(radius - 64) < 0.01f || std::abs(radius - 128) < 0.01f;
            CHECK(onCircle);
        }
    }
    REQUIRE(dx::shapeeditor::revolve(document, 4, 1, brush, &error));
    std::vector<dx::Poly>& quarter = polysOf(brush);
    REQUIRE(quarter.size() == 6);
    CHECK(countItem(quarter, "2DLoftEND") == 1);
    CHECK(countItem(quarter, "2DLoftTOP") == 1);
    for (const dx::Poly& poly : quarter) {
        if (poly.itemName == dx::Name("2DLoftEND")) {
            CHECK(faceNormal(poly).z < 0);
        }
        if (poly.itemName == dx::Name("2DLoftTOP")) {
            CHECK(faceNormal(poly).x < 0);
            CHECK(hasVertex(poly, 0, 64, 128));
        }
    }
    dx::shapeeditor::flip(document, true);
    REQUIRE(dx::shapeeditor::revolve(document, 8, 8, brush, &error));
    for (const dx::Poly& poly : polysOf(brush)) {
        dx::Vector c = centroid(poly);
        double angle = std::atan2(c.z, c.x);
        dx::Vector core { static_cast<float>(96 * std::cos(angle)), 0, static_cast<float>(96 * std::sin(angle)) };
        CHECK(faceNormal(poly).dot(c - core) > 0);
    }
    document.origin = { -96, 0 };
    CHECK_FALSE(dx::shapeeditor::revolve(document, 8, 8, brush, &error));
    CHECK(error == "Origin must be completely to the left or right side of the shape.");
}

TEST_CASE("2D shapes save and load in the UnrealEd .2ds format", "[builders]")
{
    dx::shapeeditor::Document document = lShape();
    document.origin = { 16, 32 };
    std::filesystem::path file = scratch("shape.2ds");
    std::string error;
    REQUIRE(dx::shapeeditor::save(document, file, &error));
    CHECK(std::filesystem::file_size(file) == 4 + 4 + 4 + 4 + 6 * (12 * 4 + 8));
    dx::shapeeditor::Document loaded;
    REQUIRE(dx::shapeeditor::load(loaded, file, &error));
    CHECK(loaded.origin.x == 16);
    CHECK(loaded.origin.y == 32);
    REQUIRE(loaded.shapes.size() == 1);
    REQUIRE(loaded.shapes[0].vertices.size() == 6);
    for (size_t i = 0; i < 6; i++) {
        CHECK(loaded.shapes[0].vertices[i].x == document.shapes[0].vertices[i].x);
        CHECK(loaded.shapes[0].vertices[i].y == document.shapes[0].vertices[i].y);
    }

    std::vector<uint8_t> bytes;
    putFloat(bytes, 0);
    putFloat(bytes, 0);
    put32(bytes, 1);
    put32(bytes, 3);
    float segments[3][4] = { { 0, 0, 0, 64 }, { 64, 0, 0, 0 }, { 0, 64, 64, 0 } };
    for (const auto& segment : segments) {
        for (int k = 0; k < 2; k++) {
            putFloat(bytes, segment[0]);
            putFloat(bytes, segment[1]);
            putFloat(bytes, 0);
            putFloat(bytes, segment[2]);
            putFloat(bytes, segment[3]);
            putFloat(bytes, 0);
        }
        put32(bytes, 0);
        put32(bytes, 10);
    }
    std::filesystem::path unordered = scratch("unordered.2ds");
    writeBytes(unordered, bytes);
    REQUIRE(dx::shapeeditor::load(loaded, unordered, &error));
    REQUIRE(loaded.shapes[0].vertices.size() == 3);
    CHECK(loaded.shapes[0].vertices[0].x == 0);
    CHECK(loaded.shapes[0].vertices[0].y == 0);
    CHECK(loaded.shapes[0].vertices[1].x == 0);
    CHECK(loaded.shapes[0].vertices[1].y == 64);
    CHECK(loaded.shapes[0].vertices[2].x == 64);
    CHECK(loaded.shapes[0].vertices[2].y == 0);
}

TEST_CASE("special brush presets carry the Add Special dialog flags", "[builders]")
{
    std::vector<dx::specialbrush::Preset> presets = dx::specialbrush::presets();
    REQUIRE(presets.size() == 8);
    auto flagsOf = [&](const char* name) {
        for (const dx::specialbrush::Preset& preset : presets) {
            if (preset.name == name) {
                return preset.flags;
            }
        }
        FAIL("missing preset " << name);
        return 0u;
    };
    CHECK(flagsOf("Regular Brush") == 0u);
    CHECK(flagsOf("Invisible collision hull") == 0x00000021u);
    CHECK(flagsOf("Zone Portal") == 0x04000109u);
    CHECK(flagsOf("Semisolid Pillar") == 0x00000020u);
    CHECK(flagsOf("Transparent Window") == 0x0000000cu);
    CHECK(flagsOf("Masked Decoration") == 0x0000000au);
    CHECK(flagsOf("Masked Wall") == 0x00000022u);
    CHECK(flagsOf("Water") == 0x0400010cu);
    dx::specialbrush::Options custom;
    custom.masked = true;
    custom.twoSided = true;
    custom.solidity = dx::specialbrush::Solidity::NotSolid;
    CHECK(dx::specialbrush::flags(custom) == 0x0000010au);
    std::vector<dx::specialbrush::Preset> zones = dx::specialbrush::advancedZones();
    REQUIRE(zones.size() == 3);
    CHECK(zones[0].name == "Cutaway Zone");
    CHECK(zones[1].name == "No-Terrain Zone");
    CHECK(zones[2].name == "No-Cut Zone");
}

TEST_CASE("special brushes keep their flags and solidity through a CSG rebuild", "[builders][data]")
{
    std::vector<dx::specialbrush::Preset> all = dx::specialbrush::presets();
    for (const dx::specialbrush::Preset& zone : dx::specialbrush::advancedZones()) {
        all.push_back(zone);
    }
    for (size_t index = 0; index < all.size(); index++) {
        const dx::specialbrush::Preset& preset = all[index];
        INFO(preset.name);
        dx::engine::initialize(DEUSEX_GAME_DATA);
        dx::UObject* package = dx::objects::packageObject(dx::Name(("SpecialPresetLevel" + std::to_string(index)).c_str()));
        dx::ULevel* level = make<dx::ULevel>("Level", package);
        level->model = make<dx::UModel>("Model", package);
        dx::UObject* builder = make<dx::UObject>("Brush", package);
        dx::UModel* builderModel = make<dx::UModel>("Model", package);
        builderModel->polys = make<dx::UPolys>("Polys", package);
        dx::actor::Brush(builder) = builderModel;
        dx::brush::CsgOper(builder) = dx::csg::CsgActive;
        level->actors.push_back(nullptr);
        level->actors.push_back(builder);
        dx::csg::buildCube(*builderModel->polys, 1024.0f, 1024.0f, 512.0f, nullptr);
        dx::csg::addBrush(level, builder, dx::csg::CsgSubtract);
        dx::csg::buildCube(*builderModel->polys, 128.0f, 128.0f, 128.0f, nullptr);
        dx::actor::Location(builder) = { 200.0f, 0.0f, -256.0f };
        dx::UObject* special = dx::csg::addBrush(level, builder, dx::csg::CsgAdd, preset.flags);
        REQUIRE(special != nullptr);
        dx::csg::rebuild(level);
        const dx::UModel& model = *level->model;
        uint32_t expected = preset.flags;
        if (expected & dx::bsp::PF_Portal) {
            expected = (expected & ~dx::bsp::PF_Semisolid) | dx::bsp::PF_NotSolid;
        }
        CHECK(static_cast<uint32_t>(dx::brush::PolyFlags(special)) == expected);
        int surfaces = 0;
        for (const dx::BspSurf& surf : model.surfs) {
            if (surf.actor == special) {
                surfaces++;
                CHECK((surf.polyFlags & expected) == expected);
                CHECK((surf.polyFlags & (dx::bsp::PF_Semisolid | dx::bsp::PF_NotSolid | dx::bsp::PF_Portal | dx::bsp::PF_Invisible | dx::bsp::PF_Masked | 0x4u | dx::bsp::PF_TwoSided)) == expected);
            }
        }
        CHECK(surfaces > 0);
        bool notSolid = (expected & dx::bsp::PF_NotSolid) != 0;
        CHECK(dx::csg::pointSolid(model, { 200.0f, 0.0f, -230.0f }) == !notSolid);
        CHECK_FALSE(dx::csg::pointSolid(model, { 0.0f, 0.0f, -230.0f }));
        bool cutsFloor = (expected & (dx::bsp::PF_NotSolid | dx::bsp::PF_Semisolid)) == 0;
        bool floorUnder = false;
        for (const dx::BspNode& node : model.nodes) {
            if (node.numVertices < 3 || node.plane.z < 0.99f || std::abs(node.plane.w + 256.0f) > 0.5f) {
                continue;
            }
            bool covers = true;
            for (int32_t i = 0; i < node.numVertices; i++) {
                const dx::Vector& a = model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + i)].vertex)];
                const dx::Vector& b = model.points[static_cast<size_t>(model.verts[static_cast<size_t>(node.vertPool + (i + 1) % node.numVertices)].vertex)];
                covers = covers && (b.x - a.x) * (0.0f - a.y) - (b.y - a.y) * (200.0f - a.x) >= 0;
            }
            floorUnder = floorUnder || (covers && model.surfs[static_cast<size_t>(node.surf)].actor != special);
        }
        CHECK(floorUnder == !cutsFloor);
    }
}

TEST_CASE("an extruded concave shape subtracts a matching room from the world", "[builders][data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* package = dx::objects::packageObject(dx::Name("ShapeCsgLevel"));
    dx::ULevel* level = make<dx::ULevel>("Level", package);
    level->model = make<dx::UModel>("Model", package);
    dx::UObject* builder = make<dx::UObject>("Brush", package);
    dx::UModel* builderModel = make<dx::UModel>("Model", package);
    builderModel->polys = make<dx::UPolys>("Polys", package);
    dx::actor::Brush(builder) = builderModel;
    dx::brush::CsgOper(builder) = dx::csg::CsgActive;
    level->actors.push_back(nullptr);
    level->actors.push_back(builder);
    std::string error;
    REQUIRE(dx::shapeeditor::extrude(lShape(), 64, builder, &error));
    REQUIRE(dx::csg::addBrush(level, builder, dx::csg::CsgSubtract) != nullptr);
    dx::csg::rebuild(level);
    const dx::UModel& model = *level->model;
    CHECK_FALSE(dx::csg::pointSolid(model, { 32, -32, 0 }));
    CHECK_FALSE(dx::csg::pointSolid(model, { 32, -100, 0 }));
    CHECK_FALSE(dx::csg::pointSolid(model, { 100, -32, 0 }));
    CHECK(dx::csg::pointSolid(model, { 100, -100, 0 }));
    CHECK(dx::csg::pointSolid(model, { 32, -32, 40 }));
    CHECK(dx::csg::pointSolid(model, { -10, -32, 0 }));
}
