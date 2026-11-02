#pragma once

#include "engine/brush_builder.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace dx {
    namespace brushbuilder {
        inline float quantize(float value)
        {
            float scaled = static_cast<float>(std::floor(static_cast<double>(value) * 10000.0) / 10000.0);
            char text[32];
            std::snprintf(text, sizeof(text), "%.7g", static_cast<double>(scaled));
            return std::strtof(text, nullptr);
        }

        struct Mesh {
            std::vector<Vector> vertices;
            std::vector<BuilderPolygon> polys;
            std::vector<Vector> pending;
            Name item;
            int32_t flags = 0;

            void flush()
            {
                if (pending.empty()) {
                    return;
                }
                BuilderPolygon poly;
                poly.item = item;
                poly.flags = flags;
                for (const Vector& v : pending) {
                    poly.indices.push_back(static_cast<int32_t>(vertices.size()));
                    vertices.push_back(v);
                }
                polys.push_back(poly);
                pending.clear();
            }

            void begin(const char* name, int count, int32_t polyFlags = 0)
            {
                flush();
                pending.assign(static_cast<size_t>(count), Vector {});
                item = Name(name);
                flags = polyFlags;
            }

            void put(int index, double x, double y, double z)
            {
                pending[static_cast<size_t>(index - 1)] = { quantize(static_cast<float>(x)), quantize(static_cast<float>(y)), quantize(static_cast<float>(z)) };
            }

            void ring(int index, double radius, double angle, double z)
            {
                put(index, radius * std::cos(angle), radius * std::sin(angle), z);
            }
        };

        inline bool failWith(std::string* error, const std::string& message)
        {
            if (error) {
                *error = message;
            }
            return false;
        }

        inline bool sendMesh(Mesh& mesh, UObject* brushActor, const char* group, bool mergeCoplanars, std::string* error)
        {
            mesh.flush();
            if (!finishBrush(brushActor, mesh.vertices, mesh.polys, Name(group), mergeCoplanars)) {
                return failWith(error, "The builder brush has no model");
            }
            return true;
        }
    }
}
