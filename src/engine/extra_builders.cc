#include "engine/extra_builders.h"

#include "engine/brush_builder.h"
#include "engine/bsp.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace dx {
    namespace brushbuilder {
        namespace {
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

                void put(int index, double x, double y, double z);

                void ring(int index, double radius, double angle, double z)
                {
                    put(index, radius * std::cos(angle), radius * std::sin(angle), z);
                }
            };

            float quantize(float value)
            {
                float scaled = static_cast<float>(std::floor(static_cast<double>(value) * 10000.0) / 10000.0);
                char text[32];
                std::snprintf(text, sizeof(text), "%.7g", static_cast<double>(scaled));
                return std::strtof(text, nullptr);
            }

            void Mesh::put(int index, double x, double y, double z)
            {
                pending[static_cast<size_t>(index - 1)] = { quantize(static_cast<float>(x)), quantize(static_cast<float>(y)), quantize(static_cast<float>(z)) };
            }

            double single(double v)
            {
                return static_cast<float>(v);
            }

            double value(const ExtraBuilder& builder, const char* name)
            {
                for (const ExtraParameter& parameter : builder.parameters) {
                    if (parameter.name == name) {
                        return parameter.value;
                    }
                }
                return 0;
            }

            int cint(double v)
            {
                return static_cast<int>(std::nearbyint(v));
            }

            bool fail(std::string* error, const char* message)
            {
                if (error) {
                    *error = message;
                }
                return false;
            }

            void warn(std::string* error, const char* message)
            {
                if (error) {
                    *error = message;
                }
            }

            bool send(Mesh& mesh, UObject* brushActor, const char* group, std::string* error)
            {
                mesh.flush();
                if (!finishBrush(brushActor, mesh.vertices, mesh.polys, Name(group), false)) {
                    return fail(error, "The builder brush has no model");
                }
                return true;
            }

            bool buildLinearStair(const ExtraBuilder& builder, UObject* brushActor, std::string* error)
            {
                int width = static_cast<int>(std::floor(value(builder, "StepWidth")));
                int length = static_cast<int>(std::floor(value(builder, "StepLength")));
                int height = static_cast<int>(std::floor(value(builder, "StepHeight")));
                double stepsText = value(builder, "NumSteps");
                int steps = static_cast<int>(std::floor(stepsText));
                int first = static_cast<int>(std::floor(value(builder, "FirstStep")));
                double x = single(std::floor(-(width / 2.0)));
                double y = single(std::floor(-((length * steps) / 2.0)));
                double z = single(std::floor(-(((height * steps) - first) / 2.0)));
                double zBase = single(std::floor(-(((height * steps) + first) / 2.0)));
                if (width <= 0 || length <= 0 || height <= 0 || steps <= 0) {
                    return fail(error, "You must use positive, non-zero values for Length, Width, Height, and Number of Steps.");
                }
                if (steps == 1) {
                    return fail(error, "A 1-step Staircase is a cube. Use the cube brush.");
                }
                if (steps > 45) {
                    return fail(error, "Sorry, you must have 45 or fewer steps.");
                }
                Mesh mesh;
                for (int step = 1; step <= steps; step++) {
                    double zLow = step == 1 ? zBase : z;
                    mesh.begin("Rise", 4);
                    mesh.put(4, x, y, zLow);
                    mesh.put(3, x, y, z + height);
                    mesh.put(2, x + width, y, z + height);
                    mesh.put(1, x + width, y, zLow);
                    z = z + height;
                    mesh.begin("Step", 4);
                    mesh.put(4, x, y, z);
                    mesh.put(3, x, y + length, z);
                    mesh.put(2, x + width, y + length, z);
                    mesh.put(1, x + width, y, z);
                    y = y + length;
                }
                mesh.begin("Back", 4);
                z = z - height;
                mesh.put(1, x, y, zBase);
                mesh.put(2, x, y, z + height);
                mesh.put(3, x + width, y, z + height);
                mesh.put(4, x + width, y, zBase);
                mesh.begin("Base", 4);
                mesh.put(1, x, y, zBase);
                mesh.put(2, x + width, y, zBase);
                mesh.put(3, x + width, y - (length * stepsText), zBase);
                mesh.put(4, x, y - (length * stepsText), zBase);
                int sideSteps = cint(stepsText);
                double sideEnd = std::floor((length * stepsText) / 2);
                for (int side = 0; side < 2; side++) {
                    x = single(side == 0 ? std::floor(-(width / 2.0)) : std::floor(width / 2.0));
                    y = single(std::floor(-((length * steps) / 2.0)));
                    z = single(std::floor(-(((height * steps) - first) / 2.0)));
                    for (int step = 1; step <= sideSteps; step++) {
                        double zLow = step == 1 ? zBase : z;
                        int a = side == 0 ? 1 : 4;
                        int b = side == 0 ? 2 : 3;
                        int c = side == 0 ? 3 : 2;
                        int d = side == 0 ? 4 : 1;
                        mesh.begin("Side", 4);
                        mesh.put(a, x, y, zLow);
                        mesh.put(b, x, y, z + height);
                        mesh.put(c, x, sideEnd, z + height);
                        mesh.put(d, x, sideEnd, zLow);
                        z = z + height;
                        y = y + length;
                    }
                }
                return send(mesh, brushActor, "GROUP", error);
            }

            bool buildCurvedStair(const ExtraBuilder& builder, UObject* brushActor, std::string* error)
            {
                int inner = static_cast<int>(std::floor(value(builder, "InnerRad")));
                int outer = static_cast<int>(std::floor(value(builder, "StepWidth"))) + inner;
                int height = static_cast<int>(std::floor(value(builder, "StepHeight")));
                int steps = static_cast<int>(std::floor(value(builder, "NumSteps")));
                double curve = single(value(builder, "CurveAngle"));
                int first = cint(value(builder, "FirstStep"));
                bool ccw = value(builder, "CCWSelect") != 0;
                int vdir[5] = { 0, 1, 2, 3, 4 };
                if (ccw) {
                    vdir[1] = 4;
                    vdir[2] = 3;
                    vdir[3] = 2;
                    vdir[4] = 1;
                }
                if (curve > 360) {
                    return fail(error, "Use Spiral Staircase for 360 degree or greater angles.");
                }
                if (first < 0) {
                    return fail(error, "FirstStep must be zero(0) or greater.");
                }
                if (inner <= 0 || outer <= 0 || height <= 0 || outer <= inner || steps <= 0) {
                    return fail(error, "All Values must be greater than 0.");
                }
                if (curve == 0) {
                    return fail(error, "Division by zero");
                }
                double pi = 4 * std::atan(1.0);
                double turns = single(360.0 / curve);
                double angleInc = 2 * pi / turns / steps;
                if (ccw) {
                    angleInc = -angleInc;
                }
                double halfAngle = angleInc / 2;
                if (value(builder, "AlignSide") != 0) {
                    inner = cint(1 / (std::cos(halfAngle) / inner));
                    outer = cint(1 / (std::cos(halfAngle) / outer));
                }
                double angle = 0;
                double middle = std::floor(-(((height * steps) + first) / 2.0));
                double z = single(middle + height);
                double zBase = single(middle - first);
                Mesh mesh;
                for (int step = 1; step <= steps; step++) {
                    double next = angle + angleInc;
                    mesh.begin("Step", 4);
                    mesh.ring(vdir[4], inner, angle, z);
                    mesh.ring(vdir[3], inner, next, z);
                    mesh.ring(vdir[2], outer, next, z);
                    mesh.ring(vdir[1], outer, angle, z);
                    mesh.begin("Base", 4);
                    mesh.ring(vdir[1], inner, angle, zBase);
                    mesh.ring(vdir[2], inner, next, zBase);
                    mesh.ring(vdir[3], outer, next, zBase);
                    mesh.ring(vdir[4], outer, angle, zBase);
                    double zLow = step == 1 ? zBase : z - height;
                    mesh.begin("Rise", 4);
                    mesh.ring(vdir[1], inner, angle, z);
                    mesh.ring(vdir[2], inner, angle, zLow);
                    mesh.ring(vdir[3], outer, angle, zLow);
                    mesh.ring(vdir[4], outer, angle, z);
                    mesh.begin("Side", 4);
                    mesh.ring(vdir[1], inner, angle, z);
                    mesh.ring(vdir[2], inner, next, z);
                    mesh.ring(vdir[3], inner, next, zBase);
                    mesh.ring(vdir[4], inner, angle, zBase);
                    mesh.begin("Side", 4);
                    mesh.ring(vdir[4], outer, angle, z);
                    mesh.ring(vdir[3], outer, next, z);
                    mesh.ring(vdir[2], outer, next, zBase);
                    mesh.ring(vdir[1], outer, angle, zBase);
                    angle = angle + angleInc;
                    z = z + height;
                }
                mesh.begin("Back", 4);
                mesh.ring(vdir[1], inner, angle, z - height);
                mesh.ring(vdir[2], outer, angle, z - height);
                mesh.ring(vdir[3], outer, angle, zBase);
                mesh.ring(vdir[4], inner, angle, zBase);
                return send(mesh, brushActor, "", error);
            }

            bool buildSpiralStair(const ExtraBuilder& builder, UObject* brushActor, std::string* error)
            {
                double sOuter = std::floor(value(builder, "OuterSRad"));
                double sInner = std::floor(value(builder, "InnerSRad"));
                double sHeight = std::floor(value(builder, "StepHeight"));
                double sThickness = std::floor(value(builder, "StepThickness"));
                int owRadius = static_cast<int>(std::floor(value(builder, "OuterWallRadius")));
                int numSteps = static_cast<int>(std::floor(value(builder, "NumSteps")));
                int cycle = static_cast<int>(std::floor(value(builder, "CycleNum")));
                bool cSloped = value(builder, "CeilingSloped") != 0;
                bool cStepped = !cSloped;
                bool sSloped = value(builder, "SurfaceSloped") != 0;
                bool sStepped = !sSloped;
                bool fillInt = value(builder, "IntFill") != 0;
                bool fillExt = value(builder, "ExtFill") != 0;
                bool clockwise = value(builder, "Clockwise") != 0;
                if (cSloped && sThickness < sHeight) {
                    return fail(error, "Step Thickness must be Equal to or Greater than the Step Height for Sloped Ceiling.");
                }
                if (cStepped && sSloped) {
                    return fail(error, "You Cannot have a Sloped Step with a Stepped Ceiling.");
                }
                if (sInner >= sOuter) {
                    return fail(error, "The InnerStair Radius must be Greater than the Outer Stair Radius.");
                }
                if (fillExt && sOuter >= owRadius) {
                    return fail(error, "The Outer Wall Radius must be Greater than the Outer Stair Radius.");
                }
                if (sOuter <= 0 || sInner <= 0 || sThickness <= 0 || sHeight <= 0 || owRadius <= 0 || numSteps <= 0) {
                    return fail(error, "You must use Positive non-zero values.");
                }
                if (cycle < 3) {
                    return fail(error, "You must have at least 3 Steps per 360 degrees.");
                }
                if (cSloped && sHeight > sThickness) {
                    return fail(error, "The Step Thickness must be Greater than the Step Height for Sloped Ceilings.");
                }
                if (cycle * sHeight < 48 && numSteps >= cycle) {
                    warn(error, "Normal People will BUMP their heads on the steps above!");
                }
                double pi = 4 * std::atan(1.0);
                double angleInc = clockwise ? 2 * pi / cycle : -2 * pi / cycle;
                double startAngle = angleInc / 2;
                double angle = startAngle;
                double halfAngle = angleInc / 2;
                if (value(builder, "AlignSide") != 0) {
                    sInner = 1 / (std::cos(halfAngle) / sInner);
                    sOuter = 1 / (std::cos(halfAngle) / sOuter);
                    owRadius = cint(1 / (std::cos(halfAngle) / owRadius));
                }
                int vc4[5] = { 0, 1, 2, 3, 4 };
                int vc3[4] = { 0, 1, 2, 3 };
                if (!clockwise) {
                    vc4[1] = 4;
                    vc4[2] = 3;
                    vc4[3] = 2;
                    vc4[4] = 1;
                    vc3[1] = 3;
                    vc3[2] = 2;
                    vc3[3] = 1;
                }
                double bottom = -(numSteps * sHeight / 2);
                double top = (numSteps * sHeight / 2) - (sHeight - sThickness);
                double nextAngle = 0;
                double z = single(bottom);
                double zBase = 0;
                double zTop = 0;
                double nextZ = 0;
                int topcount = cycle - 1;
                auto nextOf = [&](int i) {
                    if (i % cycle != 0) {
                        nextAngle = angle + angleInc;
                    } else {
                        nextAngle = startAngle;
                    }
                };
                auto advance = [&](int i) {
                    if (i % cycle != 0) {
                        angle = angle + angleInc;
                    } else {
                        angle = startAngle;
                    }
                };
                auto underBase = [&](int i) {
                    if (i <= cycle) {
                        zBase = single(bottom);
                    } else {
                        zBase = single(bottom + ((i - cycle) * sHeight) - (sHeight - sThickness));
                    }
                    zTop = single(bottom + (sHeight * (i - 1)));
                };
                auto topBase = [&](int i) {
                    zTop = single(top);
                    zBase = single(zTop - ((topcount - i + 1) * sHeight));
                    if (zBase < bottom) {
                        zBase = single(bottom);
                    }
                };
                Mesh mesh;
                if (sStepped && cStepped) {
                    angle = startAngle;
                    for (int i = 1; i <= numSteps; i++) {
                        nextOf(i);
                        mesh.begin("Step", 4);
                        mesh.ring(vc4[4], sInner, angle, z);
                        mesh.ring(vc4[3], sOuter, angle, z);
                        mesh.ring(vc4[2], sOuter, nextAngle, z);
                        mesh.ring(vc4[1], sInner, nextAngle, z);
                        advance(i);
                        z = single(z + sHeight);
                    }
                }
                if (sStepped) {
                    angle = startAngle;
                    z = single(bottom);
                    for (int i = 1; i <= numSteps; i++) {
                        mesh.begin("Step", 4);
                        nextOf(i);
                        mesh.ring(vc4[1], sInner, angle, z + sThickness);
                        mesh.ring(vc4[2], sOuter, angle, z + sThickness);
                        mesh.ring(vc4[3], sOuter, nextAngle, z + sThickness);
                        mesh.ring(vc4[4], sInner, nextAngle, z + sThickness);
                        advance(i);
                        z = single(z + sHeight);
                    }
                }
                if (sStepped && cStepped) {
                    angle = startAngle;
                    z = single(bottom);
                    for (int i = 1; i <= numSteps; i++) {
                        nextOf(i);
                        if (i > 1 && sHeight < sThickness) {
                            zBase = single(z + (sThickness - sHeight));
                        } else {
                            zBase = z;
                        }
                        mesh.begin("Rise", 4);
                        mesh.ring(vc4[4], sInner, angle, zBase);
                        mesh.ring(vc4[3], sInner, angle, z + sThickness);
                        mesh.ring(vc4[2], sOuter, angle, z + sThickness);
                        mesh.ring(vc4[1], sOuter, angle, zBase);
                        if (i < numSteps && sHeight < sThickness) {
                            zTop = single(z + sHeight);
                        } else {
                            zTop = single(z + sThickness);
                        }
                        mesh.begin("Rise", 4);
                        mesh.ring(vc4[1], sInner, nextAngle, z);
                        mesh.ring(vc4[2], sInner, nextAngle, zTop);
                        mesh.ring(vc4[3], sOuter, nextAngle, zTop);
                        mesh.ring(vc4[4], sOuter, nextAngle, z);
                        advance(i);
                        z = single(z + sHeight);
                    }
                }
                if (sSloped && cSloped) {
                    angle = startAngle;
                    z = single(bottom);
                    for (int i = 1; i <= numSteps; i++) {
                        if (i == 1) {
                            mesh.begin("Rise", 4);
                            mesh.ring(vc4[4], sInner, angle, z);
                            mesh.ring(vc4[3], sInner, angle, z + sThickness);
                            mesh.ring(vc4[2], sOuter, angle, z + sThickness);
                            mesh.ring(vc4[1], sOuter, angle, z);
                        }
                        if (i == numSteps) {
                            mesh.begin("Rise", 4);
                            mesh.ring(vc4[1], sInner, angle + angleInc, z);
                            mesh.ring(vc4[2], sInner, angle + angleInc, z + sThickness);
                            mesh.ring(vc4[3], sOuter, angle + angleInc, z + sThickness);
                            mesh.ring(vc4[4], sOuter, angle + angleInc, z);
                        }
                        advance(i);
                        z = single(z + sHeight);
                    }
                }
                if (cSloped && !sSloped) {
                    angle = startAngle;
                    z = single(bottom);
                    for (int i = 1; i <= numSteps; i++) {
                        if (sThickness == sHeight || i == 1) {
                            mesh.begin("Rise", 4);
                            mesh.ring(vc4[4], sInner, angle, z);
                            mesh.ring(vc4[3], sInner, angle, z + sThickness);
                            mesh.ring(vc4[2], sOuter, angle, z + sThickness);
                            mesh.ring(vc4[1], sOuter, angle, z);
                        }
                        if (sThickness > sHeight && i > 1) {
                            mesh.begin("Rise", 4);
                            mesh.ring(vc4[4], sInner, angle, z + (sThickness - sHeight));
                            mesh.ring(vc4[3], sInner, angle, z + sThickness);
                            mesh.ring(vc4[2], sOuter, angle, z + sThickness);
                            mesh.ring(vc4[1], sOuter, angle, z + (sThickness - sHeight));
                        }
                        if (i == numSteps) {
                            mesh.begin("Rise", 4);
                            mesh.ring(vc4[1], sInner, angle + angleInc, z);
                            mesh.ring(vc4[2], sInner, angle + angleInc, z + sThickness);
                            mesh.ring(vc4[3], sOuter, angle + angleInc, z + sThickness);
                            mesh.ring(vc4[4], sOuter, angle + angleInc, z);
                        }
                        advance(i);
                        z = single(z + sHeight);
                    }
                }
                if (cSloped && !sSloped) {
                    angle = startAngle;
                    z = single(bottom);
                    for (int i = 1; i <= numSteps; i++) {
                        if (i < numSteps && sHeight == sThickness && !fillExt) {
                            mesh.begin("Outer", 3);
                            mesh.ring(vc3[1], sOuter, angle, z + sHeight - sThickness);
                            mesh.ring(vc3[2], sOuter, angle + angleInc, z + sHeight);
                            mesh.ring(vc3[3], sOuter, angle, z + sHeight);
                        }
                        if (i < numSteps && sThickness > sHeight && !fillExt) {
                            mesh.begin("Outer", 4);
                            mesh.ring(vc4[1], sOuter, angle, z);
                            mesh.ring(vc4[2], sOuter, angle + angleInc, z + sHeight);
                            mesh.ring(vc4[3], sOuter, angle + angleInc, z + sHeight + (sThickness - sHeight));
                            mesh.ring(vc4[4], sOuter, angle, z + sThickness);
                        }
                        if (i == numSteps && !fillExt) {
                            mesh.begin("Outer", 4);
                            mesh.ring(vc4[1], sOuter, angle, z);
                            mesh.ring(vc4[2], sOuter, angle + angleInc, z);
                            mesh.ring(vc4[3], sOuter, angle + angleInc, z + sThickness);
                            mesh.ring(vc4[4], sOuter, angle, z + sThickness);
                        }
                        if (i < numSteps && sHeight == sThickness && !fillInt) {
                            mesh.begin("Inner", 3);
                            mesh.ring(vc3[3], sInner, angle, z + sHeight - sThickness);
                            mesh.ring(vc3[2], sInner, angle + angleInc, z + sHeight);
                            mesh.ring(vc3[1], sInner, angle, z + sHeight);
                        }
                        if (i < numSteps && sThickness > sHeight && !fillInt) {
                            mesh.begin("Inner", 4);
                            mesh.ring(vc4[4], sInner, angle, z);
                            mesh.ring(vc4[3], sInner, angle + angleInc, z + sHeight);
                            mesh.ring(vc4[2], sInner, angle + angleInc, z + sHeight + (sThickness - sHeight));
                            mesh.ring(vc4[1], sInner, angle, z + sThickness);
                        }
                        if (i == numSteps && !fillInt) {
                            mesh.begin("Inner", 4);
                            mesh.ring(vc4[4], sInner, angle, z);
                            mesh.ring(vc4[3], sInner, angle + angleInc, z);
                            mesh.ring(vc4[2], sInner, angle + angleInc, z + sThickness);
                            mesh.ring(vc4[1], sInner, angle, z + sThickness);
                        }
                        advance(i);
                        z = single(z + sHeight);
                    }
                }
                if (sStepped && cStepped) {
                    angle = startAngle;
                    z = single(bottom);
                    for (int i = 1; i <= numSteps; i++) {
                        nextOf(i);
                        if (!fillInt) {
                            mesh.begin("Inner", 4);
                            mesh.ring(vc4[4], sInner, angle, z);
                            mesh.ring(vc4[3], sInner, nextAngle, z);
                            mesh.ring(vc4[2], sInner, nextAngle, z + sThickness);
                            mesh.ring(vc4[1], sInner, angle, z + sThickness);
                        }
                        if (!fillExt) {
                            mesh.begin("Outer", 4);
                            mesh.ring(vc4[1], sOuter, angle, z);
                            mesh.ring(vc4[2], sOuter, nextAngle, z);
                            mesh.ring(vc4[3], sOuter, nextAngle, z + sThickness);
                            mesh.ring(vc4[4], sOuter, angle, z + sThickness);
                        }
                        advance(i);
                        z = single(z + sHeight);
                    }
                }
                if (cSloped) {
                    angle = startAngle;
                    z = single(bottom);
                    for (int i = 1; i <= numSteps; i++) {
                        if (i < numSteps) {
                            nextZ = single(z + sHeight);
                        } else {
                            nextZ = z;
                        }
                        mesh.begin("Ceiling", 3);
                        mesh.ring(vc3[3], sOuter, angle, z);
                        mesh.ring(vc3[2], sOuter, angle + angleInc, nextZ);
                        mesh.ring(vc3[1], sInner, angle + angleInc, nextZ);
                        mesh.begin("Ceiling", 3);
                        mesh.ring(vc3[3], sOuter, angle, z);
                        mesh.ring(vc3[2], sInner, angle + angleInc, nextZ);
                        mesh.ring(vc3[1], sInner, angle, z);
                        z = single(z + sHeight);
                        angle = angle + angleInc;
                    }
                }
                if (sSloped) {
                    z = single(bottom + sThickness);
                    angle = startAngle;
                    for (int i = 1; i <= numSteps; i++) {
                        if (i < numSteps) {
                            nextZ = single(z + sHeight);
                        } else {
                            nextZ = z;
                        }
                        mesh.begin("Step", 3);
                        mesh.ring(vc3[1], sOuter, angle, z);
                        mesh.ring(vc3[2], sOuter, angle + angleInc, nextZ);
                        mesh.ring(vc3[3], sInner, angle + angleInc, nextZ);
                        mesh.begin("Step", 3);
                        mesh.ring(vc3[1], sOuter, angle, z);
                        mesh.ring(vc3[2], sInner, angle + angleInc, nextZ);
                        mesh.ring(vc3[3], sInner, angle, z);
                        z = single(z + sHeight);
                        angle = angle + angleInc;
                    }
                }
                if (sSloped && cSloped) {
                    angle = startAngle;
                    z = single(bottom);
                    for (int i = 1; i <= numSteps; i++) {
                        if (i < numSteps) {
                            nextZ = single(z + sHeight);
                        } else {
                            nextZ = z;
                        }
                        if (!fillInt) {
                            mesh.begin("Inner", 3);
                            mesh.ring(vc3[3], sInner, angle, z);
                            mesh.ring(vc3[2], sInner, angle + angleInc, nextZ);
                            mesh.ring(vc3[1], sInner, angle + angleInc, nextZ + sThickness);
                            mesh.begin("Inner", 3);
                            mesh.ring(vc3[3], sInner, angle, z);
                            mesh.ring(vc3[2], sInner, angle + angleInc, nextZ + sThickness);
                            mesh.ring(vc3[1], sInner, angle, z + sThickness);
                        }
                        if (!fillExt) {
                            mesh.begin("Outer", 3);
                            mesh.ring(vc3[1], sOuter, angle, z);
                            mesh.ring(vc3[2], sOuter, angle + angleInc, nextZ);
                            mesh.ring(vc3[3], sOuter, angle + angleInc, nextZ + sThickness);
                            mesh.begin("Outer", 3);
                            mesh.ring(vc3[1], sOuter, angle, z);
                            mesh.ring(vc3[2], sOuter, angle + angleInc, nextZ + sThickness);
                            mesh.ring(vc3[3], sOuter, angle, z + sThickness);
                        }
                        angle = angle + angleInc;
                        z = single(z + sHeight);
                    }
                }
                if (fillInt && cStepped) {
                    angle = startAngle + angleInc;
                    for (int i = 2; i <= numSteps; i++) {
                        mesh.begin("CenterPost", 4);
                        underBase(i);
                        mesh.ring(vc4[4], sInner, angle, zBase);
                        mesh.ring(vc4[3], sInner, angle, zTop);
                        mesh.ring(vc4[2], sInner, angle + angleInc, zTop);
                        mesh.ring(vc4[1], sInner, angle + angleInc, zBase);
                        advance(i);
                    }
                    for (int i = 1; i <= topcount; i++) {
                        mesh.begin("CenterPost", 4);
                        topBase(i);
                        mesh.ring(vc4[4], sInner, angle, zBase);
                        mesh.ring(vc4[3], sInner, angle, zTop);
                        mesh.ring(vc4[2], sInner, angle + angleInc, zTop);
                        mesh.ring(vc4[1], sInner, angle + angleInc, zBase);
                        advance(i);
                    }
                }
                if (fillExt && cStepped) {
                    angle = startAngle + angleInc;
                    for (int i = 2; i <= numSteps; i++) {
                        mesh.begin("OuterInt", 4);
                        underBase(i);
                        mesh.ring(vc4[1], sOuter, angle, zBase);
                        mesh.ring(vc4[2], sOuter, angle, zTop);
                        mesh.ring(vc4[3], sOuter, angle + angleInc, zTop);
                        mesh.ring(vc4[4], sOuter, angle + angleInc, zBase);
                        advance(i);
                    }
                    for (int i = 1; i <= topcount; i++) {
                        mesh.begin("OuterInt", 4);
                        topBase(i);
                        mesh.ring(vc4[1], sOuter, angle, zBase);
                        mesh.ring(vc4[2], sOuter, angle, zTop);
                        mesh.ring(vc4[3], sOuter, angle + angleInc, zTop);
                        mesh.ring(vc4[4], sOuter, angle + angleInc, zBase);
                        angle = angle + angleInc;
                    }
                }
                if (fillInt && cSloped) {
                    angle = startAngle;
                    zBase = single(bottom);
                    zTop = single(bottom + sHeight);
                    mesh.begin("CenterPost", 3);
                    mesh.ring(vc3[3], sInner, angle, zBase);
                    mesh.ring(vc3[2], sInner, angle + angleInc, zTop);
                    mesh.ring(vc3[1], sInner, angle + angleInc, zBase);
                    angle = angle + angleInc;
                    for (int i = 2; i <= numSteps; i++) {
                        mesh.begin("CenterPost", 4);
                        underBase(i);
                        mesh.ring(vc4[4], sInner, angle, zBase);
                        mesh.ring(vc4[3], sInner, angle, zTop);
                        mesh.ring(vc4[2], sInner, angle + angleInc, i < numSteps ? zTop + sHeight : zTop);
                        mesh.ring(vc4[1], sInner, angle + angleInc, sSloped && i > cycle ? zBase + sHeight : zBase);
                        advance(i);
                    }
                    for (int i = 1; i <= topcount; i++) {
                        topBase(i);
                        if (!sSloped || i < topcount) {
                            mesh.begin("CenterPost", 4);
                            mesh.ring(vc4[4], sInner, angle, zBase);
                            mesh.ring(vc4[3], sInner, angle, zTop);
                            mesh.ring(vc4[2], sInner, angle + angleInc, zTop);
                            mesh.ring(vc4[1], sInner, angle + angleInc, sSloped ? zBase + sHeight : zBase);
                        } else {
                            mesh.begin("CenterPost", 3);
                            mesh.ring(vc3[3], sInner, angle, zBase);
                            mesh.ring(vc3[2], sInner, angle, zTop);
                            mesh.ring(vc3[1], sInner, angle + angleInc, zTop);
                        }
                        advance(i);
                    }
                }
                if (fillExt && cSloped) {
                    angle = startAngle;
                    zBase = single(bottom);
                    zTop = single(bottom + sHeight);
                    mesh.begin("OuterInt", 3);
                    mesh.ring(vc3[1], sOuter, angle, zBase);
                    mesh.ring(vc3[2], sOuter, angle + angleInc, zTop);
                    mesh.ring(vc3[3], sOuter, angle + angleInc, zBase);
                    angle = angle + angleInc;
                    for (int i = 2; i <= numSteps; i++) {
                        mesh.begin("OuterInt", 4);
                        underBase(i);
                        mesh.ring(vc4[1], sOuter, angle, zBase);
                        mesh.ring(vc4[2], sOuter, angle, zTop);
                        mesh.ring(vc4[3], sOuter, angle + angleInc, i < numSteps ? zTop + sHeight : zTop);
                        mesh.ring(vc4[4], sOuter, angle + angleInc, sSloped && i > cycle ? zBase + sHeight : zBase);
                        advance(i);
                    }
                    for (int i = 1; i <= topcount; i++) {
                        topBase(i);
                        if (!sSloped || i < topcount) {
                            mesh.begin("OuterInt", 4);
                            mesh.ring(vc4[1], sOuter, angle, zBase);
                            mesh.ring(vc4[2], sOuter, angle, zTop);
                            mesh.ring(vc4[3], sOuter, angle + angleInc, zTop);
                            mesh.ring(vc4[4], sOuter, angle + angleInc, sSloped ? zBase + sHeight : zBase);
                        } else {
                            mesh.begin("OuterInt", 3);
                            mesh.ring(vc3[1], sOuter, angle, zBase);
                            mesh.ring(vc3[2], sOuter, angle, zTop);
                            mesh.ring(vc3[3], sOuter, angle + angleInc, zTop);
                        }
                        advance(i);
                    }
                }
                if (fillInt && cycle > 12) {
                    const char* items[2] = { "Base", "Top" };
                    for (int cap = 0; cap < 2; cap++) {
                        z = single(cap == 0 ? bottom : top);
                        angle = startAngle;
                        int sideCounter = cycle;
                        bool descending = (cap == 0) == clockwise;
                        while (sideCounter > 0) {
                            int jend = sideCounter >= 6 ? 6 : sideCounter + 1;
                            int v = descending ? jend + 1 : 1;
                            mesh.begin(items[cap], jend + 1);
                            for (int i = 1; i <= jend; i++) {
                                mesh.ring(v, sInner, angle, z);
                                v = descending ? v - 1 : v + 1;
                                if (i < jend) {
                                    angle = angle + angleInc;
                                    sideCounter = sideCounter - 1;
                                }
                            }
                            mesh.put(v, 0, 0, z);
                        }
                    }
                }
                if (fillInt && cycle <= 12) {
                    const char* items[2] = { "Base", "Top" };
                    for (int cap = 0; cap < 2; cap++) {
                        z = single(cap == 0 ? bottom : top);
                        bool descending = (cap == 0) == clockwise;
                        int v = descending ? cycle + 1 : 0;
                        mesh.begin(items[cap], cycle);
                        angle = startAngle;
                        for (int i = 1; i <= cycle; i++) {
                            v = descending ? v - 1 : v + 1;
                            mesh.ring(v, sInner, angle, z);
                            if (i < cycle) {
                                angle = angle + angleInc;
                            } else {
                                angle = startAngle;
                            }
                        }
                    }
                }
                if (fillExt) {
                    angle = startAngle;
                    for (int i = 1; i <= cycle; i++) {
                        mesh.begin("Outer", 4);
                        nextAngle = i < cycle ? angle + angleInc : startAngle;
                        mesh.ring(vc4[4], owRadius, angle, bottom);
                        mesh.ring(vc4[3], owRadius, angle, top);
                        mesh.ring(vc4[2], owRadius, nextAngle, top);
                        mesh.ring(vc4[1], owRadius, nextAngle, bottom);
                        angle = angle + angleInc;
                    }
                    angle = startAngle;
                    for (int i = 1; i <= cycle; i++) {
                        mesh.begin("Base", 4);
                        nextAngle = i < cycle ? angle + angleInc : startAngle;
                        mesh.ring(vc4[1], owRadius, angle, bottom);
                        mesh.ring(vc4[2], sOuter, angle, bottom);
                        mesh.ring(vc4[3], sOuter, nextAngle, bottom);
                        mesh.ring(vc4[4], owRadius, nextAngle, bottom);
                        angle = angle + angleInc;
                    }
                    angle = startAngle;
                    for (int i = 1; i <= cycle; i++) {
                        mesh.begin("Top", 4);
                        nextAngle = i < cycle ? angle + angleInc : startAngle;
                        mesh.ring(vc4[4], owRadius, angle, top);
                        mesh.ring(vc4[3], sOuter, angle, top);
                        mesh.ring(vc4[2], sOuter, nextAngle, top);
                        mesh.ring(vc4[1], owRadius, nextAngle, top);
                        angle = angle + angleInc;
                    }
                }
                return send(mesh, brushActor, "SPIRAL", error);
            }

            bool buildSphere(const ExtraBuilder& builder, UObject* brushActor, std::string* error)
            {
                double outer = std::floor(value(builder, "OuterRadius"));
                double inner = std::floor(value(builder, "InnerRadius"));
                double radial = std::floor(value(builder, "RadialStripes"));
                int vertical = static_cast<int>(std::floor(value(builder, "VerticalStripes")));
                bool hollow = value(builder, "IsHollow") != 0;
                if (outer == 0 || inner == 0 || radial == 0 || vertical == 0) {
                    return fail(error, "You must give all numbers, and they all must be nonzero");
                }
                if (hollow && (inner + 10) >= outer) {
                    return fail(error, "Inner radius must be 10 larger than outer radius");
                }
                if (outer < 48) {
                    warn(error, "Outer radius must be at least 48, preferably larger");
                }
                if (radial < 3) {
                    return fail(error, "You need at least 3 radial stripes");
                }
                if (vertical < 2) {
                    return fail(error, "You need at least 2 vertical stripes");
                }
                double pi = 4 * std::atan(1.0);
                double angle = 0;
                double angleInc = 2 * pi / radial;
                Mesh mesh;
                for (int i = 1; i <= radial; i++) {
                    double nextAngle = i < radial ? angle + angleInc : 0;
                    for (int j = 1; j <= vertical; j++) {
                        double u1 = single(-1 + 2.0 * (j - 1) / vertical);
                        double u2 = single(-1 + 2.0 * j / vertical);
                        double r1 = single(std::sqrt(1 - u1 * u1));
                        double r2 = single(std::sqrt(1 - u2 * u2));
                        int count = j == 1 || j == vertical ? 3 : 4;
                        int v = 1;
                        mesh.begin("OUTSIDE", count);
                        mesh.ring(v, outer * r1, angle, outer * u1);
                        if (j > 1) {
                            v = v + 1;
                            mesh.ring(v, outer * r1, nextAngle, outer * u1);
                        }
                        v = v + 1;
                        mesh.ring(v, outer * r2, nextAngle, outer * u2);
                        if (j < vertical) {
                            v = v + 1;
                            mesh.ring(v, outer * r2, angle, outer * u2);
                        }
                        if (hollow) {
                            v = count;
                            mesh.begin("INSIDE", count);
                            mesh.ring(v, inner * r1, angle, inner * u1);
                            if (j > 1) {
                                v = v - 1;
                                mesh.ring(v, inner * r1, nextAngle, inner * u1);
                            }
                            v = v - 1;
                            mesh.ring(v, inner * r2, nextAngle, inner * u2);
                            if (j < vertical) {
                                v = v - 1;
                                mesh.ring(v, inner * r2, angle, inner * u2);
                            }
                        }
                    }
                    angle = angle + angleInc;
                }
                return send(mesh, brushActor, "DOME", error);
            }

            bool buildVolumetric(const ExtraBuilder& builder, UObject* brushActor, std::string* error)
            {
                int sheets = static_cast<int>(std::floor(value(builder, "NumSheets")));
                double height = value(builder, "Height");
                double radius = value(builder, "Radius");
                if (sheets < 2 || height <= 0 || radius <= 0) {
                    return fail(error, "Bad parameters in brush builder");
                }
                double pi = 4 * std::atan(1.0);
                Mesh mesh;
                for (int i = 0; i < sheets; i++) {
                    double angle = pi * i / sheets;
                    double x = radius * std::cos(angle);
                    double y = radius * std::sin(angle);
                    mesh.begin("Sheet", 4, static_cast<int32_t>(bsp::PF_TwoSided | bsp::PF_NotSolid));
                    mesh.put(1, x, y, height / 2);
                    mesh.put(2, x, y, -height / 2);
                    mesh.put(3, -x, -y, -height / 2);
                    mesh.put(4, -x, -y, height / 2);
                }
                return send(mesh, brushActor, "Volumetric", error);
            }
        }

        std::vector<ExtraBuilder> extraBuilders()
        {
            return {
                { "LinearStair", { { "StepWidth", 256, true, false }, { "StepLength", 32, true, false }, { "StepHeight", 16, true, false }, { "NumSteps", 8, true, false }, { "FirstStep", 0, true, false } } },
                { "CurvedStair", { { "InnerRad", 240, true, false }, { "StepWidth", 256, true, false }, { "StepHeight", 16, true, false }, { "NumSteps", 4, true, false }, { "CurveAngle", 90, false, false }, { "FirstStep", 0, true, false }, { "CCWSelect", 0, false, true }, { "AlignSide", 1, false, true } } },
                { "SpiralStair", { { "OuterSRad", 240, true, false }, { "InnerSRad", 64, true, false }, { "StepHeight", 16, true, false }, { "StepThickness", 32, true, false }, { "OuterWallRadius", 256, true, false }, { "NumSteps", 8, true, false }, { "CycleNum", 8, true, false }, { "AlignSide", 1, false, true }, { "Clockwise", 1, false, true }, { "IntFill", 0, false, true }, { "ExtFill", 0, false, true }, { "CeilingSloped", 0, false, true }, { "SurfaceSloped", 0, false, true } } },
                { "Sphere", { { "OuterRadius", 512, true, false }, { "InnerRadius", 496, true, false }, { "RadialStripes", 8, true, false }, { "VerticalStripes", 4, true, false }, { "IsHollow", 0, false, true } } },
                { "Volumetric", { { "NumSheets", 2, true, false }, { "Height", 128, false, false }, { "Radius", 64, false, false } } },
            };
        }

        bool buildExtra(const ExtraBuilder& builder, UObject* brushActor, std::string* error)
        {
            if (error) {
                error->clear();
            }
            if (builder.name == "LinearStair") {
                return buildLinearStair(builder, brushActor, error);
            }
            if (builder.name == "CurvedStair") {
                return buildCurvedStair(builder, brushActor, error);
            }
            if (builder.name == "SpiralStair") {
                return buildSpiralStair(builder, brushActor, error);
            }
            if (builder.name == "Sphere") {
                return buildSphere(builder, brushActor, error);
            }
            if (builder.name == "Volumetric") {
                return buildVolumetric(builder, brushActor, error);
            }
            return fail(error, "Unknown brush builder");
        }
    }
}
