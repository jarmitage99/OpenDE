#include "engine/shape_editor.h"

#include "engine/bsp.h"
#include "engine/builder_mesh.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <numeric>

namespace dx {
    namespace shapeeditor {
        namespace {
            using brushbuilder::failWith;
            using brushbuilder::Mesh;

            constexpr const char* Group = "2DLoft";
            constexpr double Epsilon = 1e-6;

            struct Profile {
                std::vector<Point> points;
                std::vector<std::array<size_t, 3>> triangles;
            };

            double cross(const Point& o, const Point& a, const Point& b)
            {
                return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
            }

            double signedArea(const std::vector<Point>& polygon)
            {
                double area = 0;
                for (size_t i = 0; i < polygon.size(); i++) {
                    const Point& a = polygon[i];
                    const Point& b = polygon[(i + 1) % polygon.size()];
                    area += a.x * b.y - b.x * a.y;
                }
                return area / 2;
            }

            bool onSegment(const Point& a, const Point& b, const Point& p)
            {
                return std::min(a.x, b.x) - Epsilon <= p.x && p.x <= std::max(a.x, b.x) + Epsilon && std::min(a.y, b.y) - Epsilon <= p.y && p.y <= std::max(a.y, b.y) + Epsilon;
            }

            int sign(double value)
            {
                if (value > Epsilon) {
                    return 1;
                }
                if (value < -Epsilon) {
                    return -1;
                }
                return 0;
            }

            bool segmentsTouch(const Point& a, const Point& b, const Point& c, const Point& d)
            {
                int d1 = sign(cross(c, d, a));
                int d2 = sign(cross(c, d, b));
                int d3 = sign(cross(a, b, c));
                int d4 = sign(cross(a, b, d));
                if (d1 * d2 < 0 && d3 * d4 < 0) {
                    return true;
                }
                return (d1 == 0 && onSegment(c, d, a)) || (d2 == 0 && onSegment(c, d, b)) || (d3 == 0 && onSegment(a, b, c)) || (d4 == 0 && onSegment(a, b, d));
            }

            bool insideTriangle(const Point& a, const Point& b, const Point& c, const Point& p)
            {
                return cross(a, b, p) >= -Epsilon && cross(b, c, p) >= -Epsilon && cross(c, a, p) >= -Epsilon;
            }

            bool samePoint(const Point& a, const Point& b)
            {
                return std::abs(a.x - b.x) < Epsilon && std::abs(a.y - b.y) < Epsilon;
            }

            bool makeProfiles(const Document& document, std::vector<Profile>& profiles, std::string* error)
            {
                if (document.shapes.empty()) {
                    return failWith(error, "There are no shapes to process");
                }
                for (const Shape& shape : document.shapes) {
                    Profile profile;
                    for (const Point& vertex : shape.vertices) {
                        profile.points.push_back({ vertex.x - document.origin.x, -(vertex.y - document.origin.y) });
                    }
                    if (signedArea(profile.points) < 0) {
                        std::reverse(profile.points.begin(), profile.points.end());
                    }
                    if (!triangulate(profile.points, profile.triangles, error)) {
                        return false;
                    }
                    profiles.push_back(profile);
                }
                return true;
            }

            void emit(Mesh& mesh, const std::string& item, const std::vector<Vector>& points, int32_t flags = 0)
            {
                std::vector<Vector> unique;
                for (const Vector& point : points) {
                    if (unique.empty() || (unique.back() - point).size() > 0.01f) {
                        unique.push_back(point);
                    }
                }
                while (unique.size() > 1 && (unique.front() - unique.back()).size() <= 0.01f) {
                    unique.pop_back();
                }
                if (unique.size() < 3) {
                    return;
                }
                mesh.begin(item.c_str(), static_cast<int>(unique.size()), flags);
                for (size_t i = 0; i < unique.size(); i++) {
                    mesh.put(static_cast<int>(i) + 1, unique[i].x, unique[i].y, unique[i].z);
                }
            }

            Vector at(const Point& point, double z, double scale = 1)
            {
                return { static_cast<float>(point.x * scale), static_cast<float>(point.y * scale), static_cast<float>(z) };
            }

            Vector rotated(const Point& point, double angle)
            {
                return { static_cast<float>(point.x * std::cos(angle)), static_cast<float>(point.y), static_cast<float>(point.x * std::sin(angle)) };
            }

            std::string item(const char* suffix)
            {
                return std::string(Group) + suffix;
            }

            void caps(Mesh& mesh, const Profile& profile, double topZ, double topScale, double bottomZ, double bottomScale)
            {
                for (const std::array<size_t, 3>& triangle : profile.triangles) {
                    emit(mesh, item("TOP"), { at(profile.points[triangle[0]], topZ, topScale), at(profile.points[triangle[1]], topZ, topScale), at(profile.points[triangle[2]], topZ, topScale) });
                    emit(mesh, item("END"), { at(profile.points[triangle[2]], bottomZ, bottomScale), at(profile.points[triangle[1]], bottomZ, bottomScale), at(profile.points[triangle[0]], bottomZ, bottomScale) });
                }
            }

            void sides(Mesh& mesh, const Profile& profile, double topZ, double topScale, double bottomZ, double bottomScale)
            {
                for (size_t i = 0; i < profile.points.size(); i++) {
                    const Point& a = profile.points[i];
                    const Point& b = profile.points[(i + 1) % profile.points.size()];
                    emit(mesh, item("SIDE"), { at(a, bottomZ, bottomScale), at(b, bottomZ, bottomScale), at(b, topZ, topScale), at(a, topZ, topScale) });
                }
            }

            bool send(Mesh& mesh, UObject* brushActor, std::string* error)
            {
                return brushbuilder::sendMesh(mesh, brushActor, Group, true, error);
            }

            void putFloat(std::vector<uint8_t>& bytes, float value)
            {
                uint8_t raw[4];
                std::memcpy(raw, &value, 4);
                bytes.insert(bytes.end(), raw, raw + 4);
            }

            void putInt(std::vector<uint8_t>& bytes, int32_t value)
            {
                uint8_t raw[4];
                std::memcpy(raw, &value, 4);
                bytes.insert(bytes.end(), raw, raw + 4);
            }

            struct Segment {
                Point vertex[2];
                Point control[2];
                int32_t type = 0;
                int32_t detail = 10;
            };
        }

        Shape defaultShape()
        {
            return { { { -128, -128 }, { -128, 128 }, { 128, 128 }, { 128, -128 } } };
        }

        Document newDocument()
        {
            Document document;
            document.shapes.push_back(defaultShape());
            return document;
        }

        Point snap(Point point, double grid)
        {
            if (grid <= 0) {
                return point;
            }
            return { std::round(point.x / grid) * grid, std::round(point.y / grid) * grid };
        }

        void splitSide(Shape& shape, size_t index, double grid)
        {
            if (index >= shape.vertices.size()) {
                return;
            }
            const Point& a = shape.vertices[index];
            const Point& b = shape.vertices[(index + 1) % shape.vertices.size()];
            Point half = snap({ (a.x + b.x) / 2, (a.y + b.y) / 2 }, grid);
            shape.vertices.insert(shape.vertices.begin() + static_cast<std::ptrdiff_t>(index + 1), half);
        }

        bool deleteVertex(Shape& shape, size_t index)
        {
            if (shape.vertices.size() <= 3 || index >= shape.vertices.size()) {
                return false;
            }
            shape.vertices.erase(shape.vertices.begin() + static_cast<std::ptrdiff_t>(index));
            return true;
        }

        void flip(Document& document, bool horizontal)
        {
            for (Shape& shape : document.shapes) {
                for (Point& vertex : shape.vertices) {
                    if (horizontal) {
                        vertex.x = 2 * document.origin.x - vertex.x;
                    } else {
                        vertex.y = 2 * document.origin.y - vertex.y;
                    }
                }
                std::reverse(shape.vertices.begin(), shape.vertices.end());
            }
        }

        bool triangulate(const std::vector<Point>& polygon, std::vector<std::array<size_t, 3>>& triangles, std::string* error)
        {
            triangles.clear();
            size_t n = polygon.size();
            if (n < 3) {
                return failWith(error, "A shape needs at least 3 vertices");
            }
            for (size_t i = 0; i < n; i++) {
                if (samePoint(polygon[i], polygon[(i + 1) % n])) {
                    return failWith(error, "Two neighbouring vertices of a shape are at the same place");
                }
                for (size_t j = i + 2; j < n; j++) {
                    if (i == 0 && j == n - 1) {
                        continue;
                    }
                    if (segmentsTouch(polygon[i], polygon[(i + 1) % n], polygon[j], polygon[(j + 1) % n])) {
                        return failWith(error, "The sides of a shape cross each other");
                    }
                }
            }
            double area = signedArea(polygon);
            if (std::abs(area) < Epsilon) {
                return failWith(error, "A shape has no area");
            }
            std::vector<size_t> ring(n);
            std::iota(ring.begin(), ring.end(), 0);
            if (area < 0) {
                std::reverse(ring.begin(), ring.end());
            }
            while (ring.size() > 3) {
                size_t m = ring.size();
                bool clipped = false;
                for (size_t k = 0; k < m && !clipped; k++) {
                    size_t a = ring[(k + m - 1) % m];
                    size_t b = ring[k];
                    size_t c = ring[(k + 1) % m];
                    double turn = cross(polygon[a], polygon[b], polygon[c]);
                    if (std::abs(turn) < Epsilon) {
                        ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(k));
                        clipped = true;
                        continue;
                    }
                    if (turn < 0) {
                        continue;
                    }
                    bool empty = true;
                    for (size_t other : ring) {
                        if (other != a && other != b && other != c && insideTriangle(polygon[a], polygon[b], polygon[c], polygon[other])) {
                            empty = false;
                            break;
                        }
                    }
                    if (empty) {
                        triangles.push_back({ a, b, c });
                        ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(k));
                        clipped = true;
                    }
                }
                if (!clipped) {
                    return failWith(error, "A shape cannot be broken into triangles");
                }
            }
            if (std::abs(cross(polygon[ring[0]], polygon[ring[1]], polygon[ring[2]])) >= Epsilon) {
                triangles.push_back({ ring[0], ring[1], ring[2] });
            }
            return true;
        }

        bool sheet(const Document& document, UObject* brushActor, std::string* error)
        {
            std::vector<Profile> profiles;
            if (!makeProfiles(document, profiles, error)) {
                return false;
            }
            Mesh mesh;
            for (const Profile& profile : profiles) {
                for (const std::array<size_t, 3>& triangle : profile.triangles) {
                    emit(mesh, item("SHEET"), { at(profile.points[triangle[0]], 0), at(profile.points[triangle[1]], 0), at(profile.points[triangle[2]], 0) }, static_cast<int32_t>(bsp::PF_NotSolid));
                }
            }
            return send(mesh, brushActor, error);
        }

        bool extrude(const Document& document, double depth, UObject* brushActor, std::string* error)
        {
            if (depth <= 0) {
                return failWith(error, "The depth must be greater than 0");
            }
            std::vector<Profile> profiles;
            if (!makeProfiles(document, profiles, error)) {
                return false;
            }
            Mesh mesh;
            for (const Profile& profile : profiles) {
                caps(mesh, profile, depth / 2, 1, -depth / 2, 1);
                sides(mesh, profile, depth / 2, 1, -depth / 2, 1);
            }
            return send(mesh, brushActor, error);
        }

        bool extrudeToPoint(const Document& document, double depth, UObject* brushActor, std::string* error)
        {
            if (depth <= 0) {
                return failWith(error, "The depth must be greater than 0");
            }
            std::vector<Profile> profiles;
            if (!makeProfiles(document, profiles, error)) {
                return false;
            }
            Mesh mesh;
            Vector apex { 0, 0, static_cast<float>(depth) };
            for (const Profile& profile : profiles) {
                for (const std::array<size_t, 3>& triangle : profile.triangles) {
                    emit(mesh, item("END"), { at(profile.points[triangle[2]], 0), at(profile.points[triangle[1]], 0), at(profile.points[triangle[0]], 0) });
                }
                for (size_t i = 0; i < profile.points.size(); i++) {
                    emit(mesh, item("SIDE"), { at(profile.points[i], 0), at(profile.points[(i + 1) % profile.points.size()], 0), apex });
                }
            }
            return send(mesh, brushActor, error);
        }

        bool extrudeToBevel(const Document& document, double depth, double capHeight, UObject* brushActor, std::string* error)
        {
            if (depth <= 0) {
                return failWith(error, "The depth must be greater than 0");
            }
            if (capHeight <= 0 || capHeight >= depth) {
                return failWith(error, "The cap height must be greater than 0 and less than the depth");
            }
            std::vector<Profile> profiles;
            if (!makeProfiles(document, profiles, error)) {
                return false;
            }
            double scale = 1.0 - capHeight / depth;
            Mesh mesh;
            for (const Profile& profile : profiles) {
                caps(mesh, profile, capHeight, scale, 0, 1);
                sides(mesh, profile, capHeight, scale, 0, 1);
            }
            return send(mesh, brushActor, error);
        }

        bool revolve(const Document& document, int totalSides, int sides, UObject* brushActor, std::string* error)
        {
            if (totalSides < 3) {
                return failWith(error, "There must be at least 3 sides per 360 degrees");
            }
            if (sides < 1 || sides > totalSides) {
                return failWith(error, "The number of sides must be between 1 and the sides per 360 degrees");
            }
            std::vector<Profile> profiles;
            if (!makeProfiles(document, profiles, error)) {
                return false;
            }
            bool positive = false;
            bool negative = false;
            for (const Profile& profile : profiles) {
                for (const Point& point : profile.points) {
                    positive = positive || point.x > Epsilon;
                    negative = negative || point.x < -Epsilon;
                }
            }
            if (positive == negative) {
                return failWith(error, "Origin must be completely to the left or right side of the shape.");
            }
            double pi = 4 * std::atan(1.0);
            double step = 2 * pi / totalSides;
            double end = step * sides;
            Mesh mesh;
            for (const Profile& profile : profiles) {
                size_t count = profile.points.size();
                if (sides != totalSides) {
                    for (const std::array<size_t, 3>& triangle : profile.triangles) {
                        std::array<size_t, 3> t = positive ? triangle : std::array<size_t, 3> { triangle[2], triangle[1], triangle[0] };
                        emit(mesh, item("END"), { rotated(profile.points[t[2]], 0), rotated(profile.points[t[1]], 0), rotated(profile.points[t[0]], 0) });
                        emit(mesh, item("TOP"), { rotated(profile.points[t[0]], end), rotated(profile.points[t[1]], end), rotated(profile.points[t[2]], end) });
                    }
                }
                for (int side = 0; side < sides; side++) {
                    double a0 = step * side;
                    double a1 = step * (side + 1);
                    for (size_t i = 0; i < count; i++) {
                        const Point& a = profile.points[positive ? i : (i + 1) % count];
                        const Point& b = profile.points[positive ? (i + 1) % count : i];
                        emit(mesh, item("SIDE"), { rotated(a, a0), rotated(b, a0), rotated(b, a1), rotated(a, a1) });
                    }
                }
            }
            return send(mesh, brushActor, error);
        }

        bool save(const Document& document, const std::filesystem::path& file, std::string* error)
        {
            std::vector<uint8_t> bytes;
            putFloat(bytes, static_cast<float>(document.origin.x));
            putFloat(bytes, static_cast<float>(document.origin.y));
            putInt(bytes, static_cast<int32_t>(document.shapes.size()));
            for (const Shape& shape : document.shapes) {
                putInt(bytes, static_cast<int32_t>(shape.vertices.size()));
                for (size_t i = 0; i < shape.vertices.size(); i++) {
                    const Point& a = shape.vertices[i];
                    const Point& b = shape.vertices[(i + 1) % shape.vertices.size()];
                    for (const Point& point : { a, b, a, b }) {
                        putFloat(bytes, static_cast<float>(point.x));
                        putFloat(bytes, static_cast<float>(point.y));
                        putFloat(bytes, 0.0f);
                    }
                    putInt(bytes, 0);
                    putInt(bytes, 10);
                }
            }
            std::ofstream out(file, std::ios::binary | std::ios::trunc);
            if (!out) {
                return failWith(error, "Cannot write " + file.string());
            }
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            return static_cast<bool>(out) || failWith(error, "Cannot write " + file.string());
        }

        bool load(Document& document, const std::filesystem::path& file, std::string* error)
        {
            std::ifstream in(file, std::ios::binary);
            if (!in) {
                return failWith(error, "Cannot open " + file.string());
            }
            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            size_t at = 0;
            bool truncated = false;
            auto next = [&](void* out) {
                if (at + 4 > bytes.size()) {
                    truncated = true;
                    std::memset(out, 0, 4);
                    return;
                }
                std::memcpy(out, bytes.data() + at, 4);
                at += 4;
            };
            auto nextFloat = [&]() {
                float value = 0;
                next(&value);
                return static_cast<double>(value);
            };
            auto nextInt = [&]() {
                int32_t value = 0;
                next(&value);
                return value;
            };
            Document result;
            result.origin.x = nextFloat();
            result.origin.y = nextFloat();
            int32_t shapeCount = nextInt();
            for (int32_t s = 0; s < shapeCount && !truncated; s++) {
                int32_t segmentCount = nextInt();
                std::vector<Segment> segments;
                for (int32_t i = 0; i < segmentCount && !truncated; i++) {
                    Segment segment;
                    Point* points[4] = { &segment.vertex[0], &segment.vertex[1], &segment.control[0], &segment.control[1] };
                    for (Point* point : points) {
                        point->x = nextFloat();
                        point->y = nextFloat();
                        nextFloat();
                    }
                    segment.type = nextInt();
                    segment.detail = nextInt();
                    segments.push_back(segment);
                }
                if (truncated || segments.size() < 3) {
                    return failWith(error, "Bad 2D shape file " + file.string());
                }
                std::vector<size_t> order { 0 };
                std::vector<bool> used(segments.size(), false);
                used[0] = true;
                while (order.size() < segments.size()) {
                    const Point& tail = segments[order.back()].vertex[1];
                    size_t found = segments.size();
                    for (size_t i = 0; i < segments.size(); i++) {
                        if (!used[i] && samePoint(segments[i].vertex[0], tail)) {
                            found = i;
                            break;
                        }
                    }
                    if (found == segments.size()) {
                        return failWith(error, "The segments of a shape do not form a closed loop");
                    }
                    used[found] = true;
                    order.push_back(found);
                }
                Shape shape;
                for (size_t index : order) {
                    const Segment& segment = segments[index];
                    shape.vertices.push_back(segment.vertex[0]);
                    if (segment.type != 1) {
                        continue;
                    }
                    const Point curve[4] = { segment.vertex[0], segment.control[0], segment.control[1], segment.vertex[1] };
                    for (int n = 1; n <= segment.detail; n++) {
                        double p = static_cast<double>(n) / (segment.detail + 1);
                        double q = 1 - p;
                        double weights[4] = { q * q * q, 3 * q * q * p, 3 * p * p * q, p * p * p };
                        Point point;
                        for (int k = 0; k < 4; k++) {
                            point.x += curve[k].x * weights[k];
                            point.y += curve[k].y * weights[k];
                        }
                        shape.vertices.push_back(point);
                    }
                }
                result.shapes.push_back(shape);
            }
            if (truncated) {
                return failWith(error, "Bad 2D shape file " + file.string());
            }
            document = result;
            return true;
        }
    }
}
