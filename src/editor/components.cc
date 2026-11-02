#include "editor/editor.h"

#include <ImGuizmo.h>

namespace dx::editor {
    namespace {
        const char* const EditModeNames[4] = { "Object", "Vertex", "Edge", "Face" };
        const char* const EditModeIcons[4] = { icons::Object, icons::Vertex, icons::Edge, icons::Face };
        const char* const EditModeKeys[4] = { "1", "2", "3", "4" };
        const char* const GizmoNames[5] = { "Off", "Move", "Rotate", "Scale", "Extrude" };
        const char* const GizmoIcons[5] = { "", icons::Move, icons::Rotate, icons::Scale, icons::Extrude };
        const char* const GizmoKeys[5] = { "", "G", "R", "T", "X" };
        const char* const EditModeHints[4] = {
            "Click to select, drag the gizmo to move, rotate or scale",
            "Click a vertex, click again to cycle overlaps, Ctrl+click to add",
            "Click an edge, click again to cycle overlaps, Ctrl+click to add",
            "Click a face, Ctrl+click to add, X then drag the arrow to extrude",
        };

        dx::Vector centerOf(const std::vector<dx::Vector>& points)
        {
            dx::Vector sum;
            for (const dx::Vector& point : points) {
                sum += point;
            }
            return sum / static_cast<float>(points.size());
        }

        void label(ImDrawList* list, const ImVec2& center, const std::string& text, ImU32 color)
        {
            ImVec2 size = ImGui::CalcTextSize(text.c_str());
            ImVec2 corner(center.x - size.x * 0.5f, center.y - size.y * 0.5f);
            list->AddRectFilled(ImVec2(corner.x - 3, corner.y - 1), ImVec2(corner.x + size.x + 3, corner.y + size.y + 1), IM_COL32(0, 0, 0, 170), 3.0f);
            list->AddText(corner, color, text.c_str());
        }
    }

    std::vector<std::vector<dx::Vector>> Editor::brushComponents(const dx::UModel& model)
    {
        std::vector<std::vector<dx::Vector>> result;
        if (editMode == 1) {
            for (const dx::Vector& vertex : dx::brushedit::localVertices(model)) {
                result.push_back({ vertex });
            }
        } else if (editMode == 2) {
            for (const auto& [a, b] : dx::brushedit::localEdges(model)) {
                result.push_back({ a, b });
            }
        } else if (editMode == 3 && model.polys) {
            for (const dx::Poly& poly : model.polys->polys) {
                result.push_back(poly.vertices);
            }
        }
        return result;
    }

    const ComponentCache& Editor::componentCache()
    {
        dx::UObject* target = editBrush();
        dx::UModel* model = target ? dx::cast<dx::UModel>(dx::actor::Brush(target)) : nullptr;
        if (!model || !model->polys) {
            cachedComponents = {};
            return cachedComponents;
        }
        uint64_t signature = 1469598103934665603ull;
        auto mix = [&](const void* data, size_t size) {
            const uint8_t* bytes = static_cast<const uint8_t*>(data);
            for (size_t i = 0; i < size; i++) {
                signature = (signature ^ bytes[i]) * 1099511628211ull;
            }
        };
        for (const dx::Poly& poly : model->polys->polys) {
            size_t count = poly.vertices.size();
            mix(&count, sizeof(count));
            mix(poly.vertices.data(), count * sizeof(dx::Vector));
        }
        mix(&dx::actor::Location(target), sizeof(dx::Vector));
        mix(&dx::actor::Rotation(target), sizeof(dx::Rotator));
        mix(&dx::actor::PrePivot(target), sizeof(dx::Vector));
        mix(dx::brush::MainScale(target), sizeof(dx::Vector));
        mix(dx::brush::PostScale(target), sizeof(dx::Vector));
        if (cachedComponents.brush == target && cachedComponents.mode == editMode && cachedComponents.signature == signature) {
            return cachedComponents;
        }
        cachedComponents.brush = target;
        cachedComponents.mode = editMode;
        cachedComponents.signature = signature;
        cachedComponents.elements = brushComponents(*model);
        cachedComponents.centers.clear();
        for (const std::vector<dx::Vector>& element : cachedComponents.elements) {
            cachedComponents.centers.push_back(dx::brushedit::toWorld(target, centerOf(element)));
        }
        cachedComponents.polys = dx::brushedit::worldPolys(target);
        return cachedComponents;
    }

    std::vector<dx::Vector> Editor::componentVertices()
    {
        std::vector<dx::Vector> result;
        for (const std::vector<dx::Vector>& element : componentSelection) {
            for (const dx::Vector& vertex : element) {
                bool known = std::any_of(result.begin(), result.end(), [&](const dx::Vector& other) {
                    return (other - vertex).sizeSquared() < 0.01f;
                });
                if (!known) {
                    result.push_back(vertex);
                }
            }
        }
        return result;
    }

    void Editor::chooseComponent(const std::vector<dx::Vector>& element, bool toggle)
    {
        auto existing = std::find_if(componentSelection.begin(), componentSelection.end(), [&](const std::vector<dx::Vector>& other) {
            return sameElement(element, other);
        });
        if (toggle && existing != componentSelection.end()) {
            componentSelection.erase(existing);
        } else if (toggle) {
            componentSelection.push_back(element);
        } else if (existing == componentSelection.end()) {
            componentSelection = { element };
        }
        status = "selected " + std::to_string(componentSelection.size()) + " " + lower(EditModeNames[editMode]) + (componentSelection.size() == 1 ? "" : "s");
    }

    bool Editor::handleOccluded(const std::vector<dx::Poly>& polys, const dx::Vector& world)
    {
        dx::Vector delta = world - cameraLocation;
        float length = delta.size();
        if (length < 1.0f) {
            return false;
        }
        float distance = 0.0f;
        return dx::brushedit::rayPoly(polys, cameraLocation, delta / length, distance) >= 0 && distance < length - 1.0f;
    }

    std::vector<std::vector<dx::Vector>> Editor::handlesAt(const std::function<bool(const dx::Vector&, ImVec2&)>& project, const ImVec2& mouse, bool occlude)
    {
        std::vector<std::vector<dx::Vector>> result;
        const ComponentCache& cache = componentCache();
        std::vector<std::pair<float, size_t>> found;
        for (size_t i = 0; i < cache.elements.size(); i++) {
            ImVec2 point;
            if (!project(cache.centers[i], point)) {
                continue;
            }
            float distance = std::sqrt((point.x - mouse.x) * (point.x - mouse.x) + (point.y - mouse.y) * (point.y - mouse.y));
            if (distance < 10.0f && !(occlude && handleOccluded(cache.polys, cache.centers[i]))) {
                found.emplace_back(distance, i);
            }
        }
        std::stable_sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });
        for (const auto& [distance, index] : found) {
            if (distance <= found.front().first + 3.0f) {
                result.push_back(cache.elements[index]);
            }
        }
        return result;
    }

    std::vector<dx::Vector> Editor::cycleChoice(const std::vector<std::vector<dx::Vector>>& candidates)
    {
        if (componentSelection.size() == 1 && candidates.size() > 1) {
            for (size_t k = 0; k < candidates.size(); k++) {
                if (sameElement(candidates[k], componentSelection[0])) {
                    return candidates[(k + 1) % candidates.size()];
                }
            }
        }
        return candidates.front();
    }

    void Editor::drawComponents(ImDrawList* list, const std::function<bool(const dx::Vector&, ImVec2&)>& project, const std::vector<dx::Vector>& hoveredWorld, bool occlude)
    {
        dx::UObject* target = editBrush();
        dx::UModel* model = target ? dx::cast<dx::UModel>(dx::actor::Brush(target)) : nullptr;
        if (!model || !model->polys) {
            return;
        }
        ImU32 handleColor = editMode == 1 ? IM_COL32(255, 200, 60, 255) : editMode == 2 ? IM_COL32(60, 220, 255, 255)
                                                                                        : IM_COL32(255, 90, 200, 255);
        bool moving = componentDragging || vertexDragging;
        ImU32 selectedColor = moving ? IM_COL32(255, 170, 40, 255) : IM_COL32(255, 255, 255, 255);
        const ComponentCache& cache = componentCache();
        for (const dx::Vector& world : cache.centers) {
            ImVec2 p;
            if (!project(world, p)) {
                continue;
            }
            ImU32 color = occlude && handleOccluded(cache.polys, world) ? (handleColor & ~IM_COL32_A_MASK) | (70u << IM_COL32_A_SHIFT) : handleColor;
            list->AddRect(ImVec2(p.x - 3, p.y - 3), ImVec2(p.x + 3, p.y + 3), color);
        }
        std::vector<ImVec2> hoverPoints;
        for (const dx::Vector& world : hoveredWorld) {
            ImVec2 p;
            if (!project(world, p)) {
                hoverPoints.clear();
                break;
            }
            hoverPoints.push_back(p);
        }
        if (hoverPoints.size() == 1) {
            list->AddRect(ImVec2(hoverPoints[0].x - 6, hoverPoints[0].y - 6), ImVec2(hoverPoints[0].x + 6, hoverPoints[0].y + 6), handleColor, 0.0f, 0, 2.0f);
        } else if (hoverPoints.size() == 2) {
            list->AddLine(hoverPoints[0], hoverPoints[1], handleColor, 3.0f);
        } else if (hoverPoints.size() > 2) {
            list->AddConvexPolyFilled(hoverPoints.data(), static_cast<int>(hoverPoints.size()), (handleColor & ~IM_COL32_A_MASK) | (40u << IM_COL32_A_SHIFT));
            list->AddPolyline(hoverPoints.data(), static_cast<int>(hoverPoints.size()), handleColor, ImDrawFlags_Closed, 2.0f);
        }
        std::vector<Edge> measured;
        for (const std::vector<dx::Vector>& element : componentSelection) {
            std::vector<dx::Vector> worldPoints;
            std::vector<ImVec2> points;
            for (const dx::Vector& vertex : element) {
                worldPoints.push_back(dx::brushedit::toWorld(target, vertex));
                ImVec2 p;
                if (!project(worldPoints.back(), p)) {
                    break;
                }
                points.push_back(p);
            }
            if (points.size() != element.size()) {
                continue;
            }
            if (points.size() == 1) {
                list->AddRectFilled(ImVec2(points[0].x - 4, points[0].y - 4), ImVec2(points[0].x + 4, points[0].y + 4), selectedColor);
            } else if (points.size() == 2) {
                list->AddLine(points[0], points[1], selectedColor, 3.0f);
            } else {
                list->AddConvexPolyFilled(points.data(), static_cast<int>(points.size()), moving ? IM_COL32(255, 150, 40, 70) : IM_COL32(255, 60, 60, 70));
                list->AddPolyline(points.data(), static_cast<int>(points.size()), moving ? selectedColor : IM_COL32(255, 80, 80, 255), ImDrawFlags_Closed, 2.0f);
            }
            for (size_t i = 0; points.size() > 1 && i < (points.size() == 2 ? 1 : points.size()); i++) {
                Edge edge { worldPoints[i], worldPoints[(i + 1) % worldPoints.size()] };
                bool known = std::any_of(measured.begin(), measured.end(), [&](const Edge& other) {
                    return ((other.a - edge.a).sizeSquared() < 0.01f && (other.b - edge.b).sizeSquared() < 0.01f) || ((other.a - edge.b).sizeSquared() < 0.01f && (other.b - edge.a).sizeSquared() < 0.01f);
                });
                if (!known) {
                    measured.push_back(edge);
                }
            }
        }
        for (const Edge& edge : measured) {
            ImVec2 a;
            ImVec2 b;
            if (!project(edge.a, a) || !project(edge.b, b) || (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) < 24.0f * 24.0f) {
                continue;
            }
            label(list, ImVec2((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f), formatNumber((edge.b - edge.a).size()), IM_COL32(255, 230, 120, 255));
        }
        if (occlude && componentSelection.size() == 1 && componentSelection[0].size() == 1) {
            dx::Vector world = dx::brushedit::toWorld(target, componentSelection[0][0]);
            ImVec2 p;
            if (project(world, p)) {
                label(list, ImVec2(p.x, p.y - 16), formatNumber(world.x) + ", " + formatNumber(world.y) + ", " + formatNumber(world.z), IM_COL32(255, 230, 120, 255));
            }
        }
    }

    bool Editor::perspectivePoint(const dx::Vector& world, ImVec2& out)
    {
        dx::Vector camera;
        if (!projectCamera(world, camera)) {
            return false;
        }
        out = cameraToScreen(camera, perspectiveOrigin, perspectiveSize);
        return true;
    }

    std::vector<dx::Vector> Editor::hoveredComponent(const ImVec2& mouse)
    {
        dx::UObject* target = editBrush();
        if (!target || !isBrush(target)) {
            return {};
        }
        if (editMode == 3) {
            const std::vector<dx::Poly>& polys = componentCache().polys;
            float distance = 0.0f;
            int32_t hit = dx::brushedit::rayPoly(polys, cameraLocation, rayDirection(mouse, perspectiveOrigin, perspectiveSize), distance);
            return hit >= 0 ? polys[static_cast<size_t>(hit)].vertices : std::vector<dx::Vector>();
        }
        std::vector<std::vector<dx::Vector>> candidates = handlesAt([&](const dx::Vector& world, ImVec2& out) {
            return perspectivePoint(world, out);
        },
            mouse, true);
        std::vector<dx::Vector> result;
        if (!candidates.empty()) {
            for (const dx::Vector& vertex : candidates.front()) {
                result.push_back(dx::brushedit::toWorld(target, vertex));
            }
        }
        return result;
    }

    bool Editor::pickPerspectiveComponent(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size, bool toggle)
    {
        if (editMode != 3) {
            std::vector<std::vector<dx::Vector>> candidates = handlesAt([&](const dx::Vector& world, ImVec2& out) {
                return perspectivePoint(world, out);
            },
                mouse, true);
            if (!candidates.empty()) {
                chooseComponent(toggle ? candidates.front() : cycleChoice(candidates), toggle);
                return true;
            }
        }
        dx::Vector direction = rayDirection(mouse, origin, size);
        dx::UObject* hitBrush = nullptr;
        std::vector<dx::Vector> picked;
        float bestDistance = FLT_MAX;
        for (dx::UObject* actor : actors) {
            if (!isBrush(actor) || dx::actor::bHiddenEd(actor)) {
                continue;
            }
            float distance = 0.0f;
            int32_t hit = dx::brushedit::rayPoly(dx::brushedit::worldPolys(actor), cameraLocation, direction, distance);
            if (hit >= 0 && distance < bestDistance) {
                bestDistance = distance;
                hitBrush = actor;
                picked = dx::cast<dx::UModel>(dx::actor::Brush(actor))->polys->polys[static_cast<size_t>(hit)].vertices;
            }
        }
        if (!hitBrush) {
            if (!toggle) {
                componentSelection.clear();
            }
            return false;
        }
        if (hitBrush != editBrush()) {
            select(hitBrush);
        }
        if (editMode == 3) {
            chooseComponent(picked, toggle);
        }
        return true;
    }

    void Editor::gizmoMatrices(const ImVec2& size, float* view, float* projection)
    {
        dx::Coords camera = dx::rotationAxes(cameraRotation);
        const dx::Vector& r = camera.yAxis;
        const dx::Vector& u = camera.zAxis;
        const dx::Vector& f = camera.xAxis;
        const dx::Vector& e = cameraLocation;
        float viewMatrix[16] = { r.x, u.x, -f.x, 0, r.y, u.y, -f.y, 0, r.z, u.z, -f.z, 0, -r.dot(e), -u.dot(e), f.dot(e), 1 };
        float nearPlane = 4.0f;
        float farPlane = 65536.0f;
        float focal = 1.0f / std::tan(fov * dx::Pi / 360.0f);
        float projectionMatrix[16] = { focal, 0, 0, 0, 0, focal * size.x / size.y, 0, 0, 0, 0, (farPlane + nearPlane) / (nearPlane - farPlane), -1, 0, 0, 2.0f * farPlane * nearPlane / (nearPlane - farPlane), 0 };
        std::copy_n(viewMatrix, 16, view);
        std::copy_n(projectionMatrix, 16, projection);
    }

    void Editor::drawReadout(const std::string& text)
    {
        ImDrawList* list = ImGui::GetForegroundDrawList();
        ImVec2 mouse = ImGui::GetIO().MousePos;
        ImVec2 at(mouse.x + 20, mouse.y + 20);
        ImVec2 size = ImGui::CalcTextSize(text.c_str());
        list->AddRectFilled(ImVec2(at.x - 5, at.y - 3), ImVec2(at.x + size.x + 5, at.y + size.y + 3), IM_COL32(20, 20, 24, 230), 4.0f);
        list->AddRect(ImVec2(at.x - 5, at.y - 3), ImVec2(at.x + size.x + 5, at.y + size.y + 3), IM_COL32(255, 170, 40, 255), 4.0f);
        list->AddText(at, IM_COL32(255, 255, 255, 255), text.c_str());
    }

    void Editor::drawDimensions(ImDrawList* list, const OrthoView& view, const ImVec2& origin, const ImVec2& size)
    {
        dx::UObject* target = editMode != 0 ? editBrush() : selected;
        if (!target || !isBrush(target)) {
            return;
        }
        std::vector<Edge> outline = brushEdges(target);
        if (outline.empty()) {
            return;
        }
        dx::Vector low { FLT_MAX, FLT_MAX, FLT_MAX };
        dx::Vector high { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        for (const Edge& edge : outline) {
            for (const dx::Vector& point : { edge.a, edge.b }) {
                low = { std::min(low.x, point.x), std::min(low.y, point.y), std::min(low.z, point.z) };
                high = { std::max(high.x, point.x), std::max(high.y, point.y), std::max(high.z, point.z) };
            }
        }
        ImVec2 a = orthoToScreen(view, low, origin, size);
        ImVec2 b = orthoToScreen(view, high, origin, size);
        ImVec2 min(std::min(a.x, b.x), std::min(a.y, b.y));
        ImVec2 max(std::max(a.x, b.x), std::max(a.y, b.y));
        if (max.x - min.x < 48.0f && max.y - min.y < 48.0f) {
            return;
        }
        ImU32 color = IM_COL32(255, 230, 120, 255);
        label(list, ImVec2((min.x + max.x) * 0.5f, min.y - 12.0f), formatNumber(component(high, view.horizontal) - component(low, view.horizontal)), color);
        std::string height = formatNumber(component(high, view.vertical) - component(low, view.vertical));
        label(list, ImVec2(min.x - ImGui::CalcTextSize(height.c_str()).x * 0.5f - 8.0f, (min.y + max.y) * 0.5f), height, color);
    }

    bool Editor::componentGizmo(const ImVec2& origin, const ImVec2& size)
    {
        dx::UObject* target = editBrush();
        dx::UModel* model = target ? dx::cast<dx::UModel>(dx::actor::Brush(target)) : nullptr;
        if (gizmoMode == 0 || componentSelection.empty() || !model || !model->polys || size.x < 2.0f || size.y < 2.0f) {
            return false;
        }
        std::vector<dx::Vector> chosen = componentVertices();
        bool extrude = gizmoMode == 4 && editMode == 3;
        if (!componentDragging) {
            componentPivot = {};
            for (const dx::Vector& vertex : chosen) {
                componentPivot += dx::brushedit::toWorld(target, vertex);
            }
            componentPivot = componentPivot / static_cast<float>(chosen.size());
            dx::Vector x { 1, 0, 0 };
            dx::Vector y { 0, 1, 0 };
            dx::Vector z { 0, 0, 1 };
            if (extrude) {
                dx::Vector sum;
                for (const std::vector<dx::Vector>& element : componentSelection) {
                    int32_t index = dx::brushedit::findPoly(*model, element);
                    if (index >= 0) {
                        const dx::Poly& poly = model->polys->polys[static_cast<size_t>(index)];
                        sum += dx::brushedit::toWorld(target, poly.vertices[0] + poly.normal) - dx::brushedit::toWorld(target, poly.vertices[0]);
                    }
                }
                if (sum.sizeSquared() < 0.0001f) {
                    return false;
                }
                z = sum.normal();
                dx::Vector reference = std::fabs(z.z) < 0.9f ? dx::Vector { 0, 0, 1 } : dx::Vector { 1, 0, 0 };
                x = reference.cross(z).normal();
                y = z.cross(x);
            }
            componentNormal = z;
            componentMatrix = { x.x, x.y, x.z, 0, y.x, y.y, y.z, 0, z.x, z.y, z.z, 0, componentPivot.x, componentPivot.y, componentPivot.z, 1 };
        }
        ImGuizmo::BeginFrame();
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(origin.x, origin.y, size.x, size.y);
        float view[16];
        float projection[16];
        gizmoMatrices(size, view, projection);
        const dx::brushbuilder::Constraints& constraints = dx::brushbuilder::constraints();
        float snap[3] = { static_cast<float>(constraints.rotGridSize.yaw) * 360.0f / 65536.0f, 0.0f, 0.0f };
        bool snapping = gizmoMode == 2 && constraints.rotGridEnabled;
        ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
        if (extrude) {
            operation = ImGuizmo::TRANSLATE_Z;
        } else if (gizmoMode == 2) {
            operation = ImGuizmo::ROTATE;
        } else if (gizmoMode == 3) {
            operation = ImGuizmo::SCALE;
        }
        bool changed = ImGuizmo::Manipulate(view, projection, operation, extrude ? ImGuizmo::LOCAL : ImGuizmo::WORLD, componentMatrix.data(), nullptr, snapping ? snap : nullptr);
        bool usingGizmo = ImGuizmo::IsUsing();
        if (usingGizmo && !componentDragging && !history.recording()) {
            componentDragging = true;
            history.begin(level, { target, model, model->polys }, extrude ? "Extrude faces" : std::string(GizmoNames[gizmoMode]) + " " + lower(EditModeNames[editMode]) + "s");
            vertexDragPolys = model->polys->polys;
        }
        std::array<float, 16> effective = componentMatrix;
        dx::Vector translation { effective[12], effective[13], effective[14] };
        if (gizmoMode == 1 && constraints.gridEnabled) {
            dx::Vector reference = dx::brushedit::toWorld(target, chosen[0]);
            translation = componentPivot + snapped(reference + translation - componentPivot) - reference;
            effective[12] = translation.x;
            effective[13] = translation.y;
            effective[14] = translation.z;
        }
        const float* m = effective.data();
        auto apply = [&](const dx::Vector& local) {
            dx::Vector d = dx::brushedit::toWorld(target, local) - componentPivot;
            dx::Vector moved { m[0] * d.x + m[4] * d.y + m[8] * d.z + m[12], m[1] * d.x + m[5] * d.y + m[9] * d.z + m[13], m[2] * d.x + m[6] * d.y + m[10] * d.z + m[14] };
            return dx::brushedit::toLocal(target, moved);
        };
        float distance = (translation - componentPivot).dot(componentNormal);
        if (extrude && constraints.gridEnabled) {
            float axis = std::max({ std::fabs(componentNormal.x), std::fabs(componentNormal.y), std::fabs(componentNormal.z) });
            dx::Vector reference = dx::brushedit::toWorld(target, componentSelection[0][0]);
            if (axis > 0.999f) {
                distance = (snapped(reference + componentNormal * distance) - reference).dot(componentNormal);
            } else {
                distance = std::round(distance / constraints.gridSize.x) * constraints.gridSize.x;
            }
        }
        dx::Vector offset = dx::brushedit::toLocal(target, componentPivot + componentNormal * distance) - dx::brushedit::toLocal(target, componentPivot);
        if (changed && componentDragging) {
            model->polys->polys = vertexDragPolys;
            if (extrude) {
                for (const std::vector<dx::Vector>& element : componentSelection) {
                    int32_t index = dx::brushedit::findPoly(*model, element);
                    if (index >= 0) {
                        dx::brushedit::extrudeFace(*model, static_cast<size_t>(index), offset);
                    }
                }
            } else {
                dx::brushedit::transformVertices(*model, chosen, apply);
                dx::csg::updateBounds(*model);
            }
            brushCache.erase(target);
        }
        if (componentDragging && usingGizmo) {
            dx::Vector moved = translation - componentPivot;
            if (extrude) {
                drawReadout(withIcon(icons::Extrude, formatNumber(distance).c_str()));
            } else if (gizmoMode == 2) {
                float angle = std::acos(std::clamp((m[0] + m[5] + m[10] - 1.0f) * 0.5f, -1.0f, 1.0f)) * 180.0f / dx::Pi;
                drawReadout(withIcon(icons::Angle, (formatNumber(angle) + "\xc2\xb0").c_str()));
            } else if (gizmoMode == 3) {
                std::string factors = formatNumber(dx::Vector { m[0], m[1], m[2] }.size()) + " x " + formatNumber(dx::Vector { m[4], m[5], m[6] }.size()) + " x " + formatNumber(dx::Vector { m[8], m[9], m[10] }.size());
                drawReadout(withIcon(icons::Scale, factors.c_str()));
            } else {
                std::string delta = formatNumber(moved.x) + ", " + formatNumber(moved.y) + ", " + formatNumber(moved.z) + "   " + icons::Ruler + " " + formatNumber(moved.size());
                drawReadout(withIcon(icons::Move, delta.c_str()));
            }
        }
        if (!usingGizmo && componentDragging) {
            componentDragging = false;
            bool moved = extrude ? std::fabs(distance) > 0.01f : std::any_of(chosen.begin(), chosen.end(), [&](const dx::Vector& vertex) {
                return (apply(vertex) - vertex).sizeSquared() > 0.0001f;
            });
            if (moved) {
                if (!extrude) {
                    dx::brushedit::validate(*model);
                }
                for (std::vector<dx::Vector>& element : componentSelection) {
                    for (dx::Vector& vertex : element) {
                        vertex = extrude ? vertex + offset : apply(vertex);
                    }
                }
                history.commit();
                status = extrude ? "extruded " + std::to_string(componentSelection.size()) + (componentSelection.size() == 1 ? " face" : " faces") : "edited " + std::to_string(chosen.size()) + " brush vertices";
            } else {
                model->polys->polys = vertexDragPolys;
                history.cancel();
            }
            brushCache.erase(target);
        }
        return usingGizmo || ImGuizmo::IsOver();
    }

    bool Editor::editToolbar(const ImVec2& origin)
    {
        std::array<std::string, 4> modeLabels;
        std::array<std::string, 5> gizmoLabels;
        float modeWidth = 0.0f;
        float gizmoWidth = 0.0f;
        for (int mode = 0; mode < 5; mode++) {
            if (mode < 4) {
                modeLabels[static_cast<size_t>(mode)] = withIcon(EditModeIcons[mode], EditModeNames[mode]) + " " + EditModeKeys[mode];
                modeWidth += ImGui::CalcTextSize(modeLabels[static_cast<size_t>(mode)].c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f + 2.0f;
            }
            if (mode > 0) {
                gizmoLabels[static_cast<size_t>(mode)] = withIcon(GizmoIcons[mode], GizmoNames[mode]) + " " + GizmoKeys[mode];
                gizmoWidth += ImGui::CalcTextSize(gizmoLabels[static_cast<size_t>(mode)].c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f + 2.0f;
            }
        }
        if (std::max(modeWidth, gizmoWidth) > perspectiveSize.x - 12.0f) {
            for (int mode = 0; mode < 5; mode++) {
                if (mode < 4) {
                    modeLabels[static_cast<size_t>(mode)] = std::string(EditModeIcons[mode]) + "##mode" + EditModeNames[mode];
                }
                if (mode > 0) {
                    gizmoLabels[static_cast<size_t>(mode)] = std::string(GizmoIcons[mode]) + "##gizmo" + GizmoNames[mode];
                }
            }
        }
        ImGui::SetCursorScreenPos(ImVec2(origin.x + 6, origin.y + 24));
        ImGui::BeginGroup();
        for (int mode = 0; mode < 4; mode++) {
            if (mode > 0) {
                ImGui::SameLine(0, 2);
            }
            const std::string& text = modeLabels[static_cast<size_t>(mode)];
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(editMode == mode ? ImGuiCol_ButtonActive : ImGuiCol_FrameBg));
            if (ImGui::SmallButton(text.c_str())) {
                command("editmode " + lower(EditModeNames[mode]));
            }
            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s mode (%s)", EditModeNames[mode], EditModeKeys[mode]);
            }
        }
        for (int mode = 1; mode < 5; mode++) {
            if (mode > 1) {
                ImGui::SameLine(0, 2);
            }
            const std::string& text = gizmoLabels[static_cast<size_t>(mode)];
            ImGui::BeginDisabled(mode == 4 && editMode != 3);
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(gizmoMode == mode ? ImGuiCol_ButtonActive : ImGuiCol_FrameBg));
            if (ImGui::SmallButton(text.c_str())) {
                command("gizmo " + lower(GizmoNames[mode]));
            }
            ImGui::PopStyleColor();
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s (%s)%s", GizmoNames[mode], GizmoKeys[mode], mode == 4 && editMode != 3 ? ", needs face mode" : "");
            }
        }
        if (editMode != 0 && !componentSelection.empty()) {
            std::string what = std::to_string(componentSelection.size()) + " " + lower(EditModeNames[editMode]) + (componentSelection.size() == 1 ? "" : "s") + " selected, Esc to clear";
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.5f, 1.0f));
            ImGui::TextWrapped("%s", withIcon(EditModeIcons[editMode], what.c_str()).c_str());
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.75f));
            ImGui::TextWrapped("%s", EditModeHints[editMode]);
        }
        ImGui::PopStyleColor();
        ImGui::EndGroup();
        return ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }

    void Editor::editModeKeys()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput || io.KeyCtrl || io.KeyAlt) {
            return;
        }
        const ImGuiKey modeKeys[4] = { ImGuiKey_1, ImGuiKey_2, ImGuiKey_3, ImGuiKey_4 };
        for (int mode = 0; mode < 4; mode++) {
            if (ImGui::IsKeyPressed(modeKeys[mode], false)) {
                command("editmode " + lower(EditModeNames[mode]));
            }
        }
        const ImGuiKey gizmoKeys[5] = { ImGuiKey_None, ImGuiKey_G, ImGuiKey_R, ImGuiKey_T, ImGuiKey_X };
        for (int mode = 1; mode < 5; mode++) {
            if (ImGui::IsKeyPressed(gizmoKeys[mode], false)) {
                command("gizmo " + lower(GizmoNames[mode]));
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !popupWasOpen) {
            if (!componentSelection.empty()) {
                componentSelection.clear();
            } else {
                command("editmode object");
            }
        }
    }

    void Editor::editModeMenuItems()
    {
        if (!ImGui::BeginMenu(withIcon(EditModeIcons[editMode], "Edit Mode").c_str())) {
            return;
        }
        for (int mode = 0; mode < 4; mode++) {
            if (ImGui::MenuItem(withIcon(EditModeIcons[mode], EditModeNames[mode]).c_str(), EditModeKeys[mode], editMode == mode)) {
                command("editmode " + lower(EditModeNames[mode]));
            }
        }
        ImGui::Separator();
        if (ImGui::MenuItem(withIcon(icons::Extrude, "Extrude Selected Faces").c_str(), "X", false, editMode == 3 && !componentSelection.empty())) {
            command("gizmo extrude");
        }
        ImGui::EndMenu();
    }

    bool Editor::componentCommand(const std::string& verb, const std::string& rest)
    {
        if (verb == "editmode") {
            for (int mode = 0; mode < 4; mode++) {
                if (lower(rest) == lower(EditModeNames[mode])) {
                    if (editMode != mode) {
                        componentSelection.clear();
                    }
                    editMode = mode;
                    if (mode != 0) {
                        clipMode = false;
                    }
                    if (gizmoMode == 4 && mode != 3) {
                        gizmoMode = 1;
                    }
                    status = std::string(EditModeNames[mode]) + " mode";
                    return true;
                }
            }
            status = "unknown edit mode " + rest;
            return false;
        }
        float distance = std::strtof(rest.c_str(), nullptr);
        if (editMode != 3 || componentSelection.empty()) {
            status = "select faces in face mode to extrude";
            return false;
        }
        return brushPolysEdit("Extrude faces", [&](dx::UObject* actor, dx::UModel& model) {
            bool done = false;
            for (std::vector<dx::Vector>& element : componentSelection) {
                int32_t index = dx::brushedit::findPoly(model, element);
                if (index < 0) {
                    continue;
                }
                const dx::Poly& poly = model.polys->polys[static_cast<size_t>(index)];
                dx::Vector base = dx::brushedit::toWorld(actor, poly.vertices[0]);
                dx::Vector normal = (dx::brushedit::toWorld(actor, poly.vertices[0] + poly.normal) - base).normal();
                dx::Vector offset = dx::brushedit::toLocal(actor, base + normal * distance) - poly.vertices[0];
                if (dx::brushedit::extrudeFace(model, static_cast<size_t>(index), offset)) {
                    for (dx::Vector& vertex : element) {
                        vertex += offset;
                    }
                    done = true;
                }
            }
            return done;
        });
    }
}
