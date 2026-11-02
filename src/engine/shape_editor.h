#pragma once

#include "engine/resources.h"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace dx {
    namespace shapeeditor {
        struct Point {
            double x = 0;
            double y = 0;
        };

        struct Shape {
            std::vector<Point> vertices;
        };

        struct Document {
            Point origin;
            std::vector<Shape> shapes;
        };

        Shape defaultShape();
        Document newDocument();
        Point snap(Point point, double grid);
        void splitSide(Shape& shape, size_t index, double grid);
        bool deleteVertex(Shape& shape, size_t index);
        void flip(Document& document, bool horizontal);
        bool triangulate(const std::vector<Point>& polygon, std::vector<std::array<size_t, 3>>& triangles, std::string* error);
        bool sheet(const Document& document, UObject* brushActor, std::string* error);
        bool extrude(const Document& document, double depth, UObject* brushActor, std::string* error);
        bool extrudeToPoint(const Document& document, double depth, UObject* brushActor, std::string* error);
        bool extrudeToBevel(const Document& document, double depth, double capHeight, UObject* brushActor, std::string* error);
        bool revolve(const Document& document, int totalSides, int sides, UObject* brushActor, std::string* error);
        bool save(const Document& document, const std::filesystem::path& file, std::string* error);
        bool load(Document& document, const std::filesystem::path& file, std::string* error);
    }
}
