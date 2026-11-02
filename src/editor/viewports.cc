#include "editor/editor.h"
#include "engine/actor_selection.h"

namespace dx::editor {
    void Editor::buildEdges()
    {
        edges.clear();
        dx::UModel* model = level->model;
        if (!model) {
            return;
        }
        std::unordered_set<uint64_t> seen;
        for (const dx::BspNode& node : model->nodes) {
            int32_t count = node.numVertices;
            for (int32_t i = 0; i < count; i++) {
                size_t first = static_cast<size_t>(node.vertPool + i);
                size_t second = static_cast<size_t>(node.vertPool + (i + 1) % count);
                if (first >= model->verts.size() || second >= model->verts.size()) {
                    continue;
                }
                uint32_t a = static_cast<uint32_t>(model->verts[first].vertex);
                uint32_t b = static_cast<uint32_t>(model->verts[second].vertex);
                if (a >= model->points.size() || b >= model->points.size()) {
                    continue;
                }
                uint64_t key = a < b ? (static_cast<uint64_t>(a) << 32) | b : (static_cast<uint64_t>(b) << 32) | a;
                if (seen.insert(key).second) {
                    edges.push_back({ model->points[a], model->points[b] });
                }
            }
        }
    }

    std::vector<Edge> Editor::brushEdges(dx::UObject* actor)
    {
        std::vector<Edge> result;
        dx::UModel* model = dx::cast<dx::UModel>(dx::actor::Brush(actor));
        if (!model) {
            return result;
        }
        dx::objects::ensureLoaded(model);
        if (model->polys) {
            dx::objects::ensureLoaded(model->polys);
        }
        dx::objects::flushLoads();
        if (!model->polys) {
            return result;
        }
        dx::Coords axes = dx::rotationAxes(dx::actor::Rotation(actor));
        dx::Vector location = dx::actor::Location(actor);
        dx::Vector pivot = dx::actor::PrePivot(actor);
        dx::Vector mainScale = *reinterpret_cast<dx::Vector*>(dx::brush::MainScale(actor));
        dx::Vector postScale = *reinterpret_cast<dx::Vector*>(dx::brush::PostScale(actor));
        auto transform = [&](const dx::Vector& v) {
            dx::Vector local = (v - pivot) * mainScale;
            dx::Vector rotated = axes.xAxis * local.x + axes.yAxis * local.y + axes.zAxis * local.z;
            return location + rotated * postScale;
        };
        for (const dx::Poly& poly : model->polys->polys) {
            size_t count = poly.vertices.size();
            for (size_t i = 0; i < count; i++) {
                result.push_back({ transform(poly.vertices[i]), transform(poly.vertices[(i + 1) % count]) });
            }
        }
        return result;
    }

    const std::vector<Edge>& Editor::cachedBrushEdges(dx::UObject* actor)
    {
        auto found = brushCache.find(actor);
        if (found == brushCache.end() || actor == selected) {
            found = brushCache.insert_or_assign(actor, brushEdges(actor)).first;
        }
        return found->second;
    }

    ImU32 Editor::brushColor(dx::UObject* actor)
    {
        if (actor->isA(dx::Name("Mover"))) {
            return IM_COL32(200, 80, 255, 255);
        }
        if (actor == builderBrush()) {
            return IM_COL32(255, 40, 40, 255);
        }
        if (dx::brush::CsgOper(actor) == dx::csg::CsgSubtract) {
            return IM_COL32(255, 160, 40, 255);
        }
        return IM_COL32(80, 140, 255, 255);
    }

    dx::UObject* Editor::builderBrush()
    {
        for (dx::UObject* actor : actors) {
            if (actor->cls->name == dx::Name("Brush") && dx::brush::CsgOper(actor) == dx::csg::CsgActive) {
                return actor;
            }
        }
        for (dx::UObject* actor : actors) {
            if (actor->cls->name == dx::Name("Brush")) {
                return actor;
            }
        }
        return nullptr;
    }

    bool Editor::isBrush(dx::UObject* actor)
    {
        return actor->isA(dx::Name("Brush")) && dx::actor::Brush(actor);
    }

    void Editor::ensureFramebuffer(int width, int height)
    {
        if (framebuffer && width == fbWidth && height == fbHeight) {
            return;
        }
        releaseFramebuffer();
        fbWidth = width;
        fbHeight = height;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        glGenRenderbuffers(1, &depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Editor::releaseFramebuffer()
    {
        if (framebuffer) {
            glDeleteFramebuffers(1, &framebuffer);
            glDeleteTextures(1, &colorTexture);
            glDeleteRenderbuffers(1, &depthBuffer);
            framebuffer = 0;
            colorTexture = 0;
            depthBuffer = 0;
        }
    }

    void Editor::renderPerspective(int width, int height)
    {
        ensureFramebuffer(width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        std::vector<dx::UObject*> shown;
        for (dx::UObject* actor : actors) {
            if (dx::actor::bHidden(actor) && dx::actor::DrawType(actor) == dx::DT_Sprite && dx::actor::Texture(actor)) {
                shown.push_back(actor);
                dx::actor::bHidden(actor) = 0;
            }
        }
        renderer->beginFrame(width, height);
        renderer->setRenderMode(perspectiveMode);
        renderer->drawScene(level, dx::actorselect::visible(actors), cameraLocation, cameraRotation, fov, nullptr);
        renderer->setRenderMode(dx::RenderMode::Textured);
        renderer->endFrame();
        for (dx::UObject* actor : shown) {
            dx::actor::bHidden(actor) = 1;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool Editor::projectCamera(const dx::Vector& point, dx::Vector& out)
    {
        dx::Coords axes = dx::rotationAxes(cameraRotation);
        dx::Vector delta = point - cameraLocation;
        out = { delta.dot(axes.yAxis), delta.dot(axes.zAxis), delta.dot(axes.xAxis) };
        return out.z > 1.0f;
    }

    ImVec2 Editor::cameraToScreen(const dx::Vector& camera, const ImVec2& origin, const ImVec2& size)
    {
        float scale = size.x * 0.5f / std::tan(fov * dx::Pi / 360.0f);
        return ImVec2(origin.x + size.x * 0.5f + camera.x / camera.z * scale, origin.y + size.y * 0.5f - camera.y / camera.z * scale);
    }

    void Editor::drawPerspectiveLine(ImDrawList* list, const dx::Vector& a, const dx::Vector& b, const ImVec2& origin, const ImVec2& size, ImU32 color)
    {
        dx::Vector ca;
        dx::Vector cb;
        bool va = projectCamera(a, ca);
        bool vb = projectCamera(b, cb);
        if (!va && !vb) {
            return;
        }
        if (!va || !vb) {
            float t = (1.0f - ca.z) / (cb.z - ca.z);
            dx::Vector clipped = ca + (cb - ca) * t;
            if (!va) {
                ca = clipped;
            } else {
                cb = clipped;
            }
        }
        list->AddLine(cameraToScreen(ca, origin, size), cameraToScreen(cb, origin, size), color, 1.5f);
    }

    void Editor::drawCylinder(ImDrawList* list, dx::UObject* actor, const ImVec2& origin, const ImVec2& size)
    {
        dx::Vector center = dx::actor::Location(actor);
        float radius = std::max(dx::actor::CollisionRadius(actor), 8.0f);
        float height = std::max(dx::actor::CollisionHeight(actor), 8.0f);
        ImU32 color = IM_COL32(255, 255, 0, 255);
        const int segments = 16;
        for (int i = 0; i < segments; i++) {
            float a0 = static_cast<float>(i) * 2.0f * dx::Pi / segments;
            float a1 = static_cast<float>(i + 1) * 2.0f * dx::Pi / segments;
            dx::Vector p0 { center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius, 0 };
            dx::Vector p1 { center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius, 0 };
            dx::Vector top0 { p0.x, p0.y, center.z + height };
            dx::Vector top1 { p1.x, p1.y, center.z + height };
            dx::Vector bottom0 { p0.x, p0.y, center.z - height };
            dx::Vector bottom1 { p1.x, p1.y, center.z - height };
            drawPerspectiveLine(list, top0, top1, origin, size, color);
            drawPerspectiveLine(list, bottom0, bottom1, origin, size, color);
            if (i % 4 == 0) {
                drawPerspectiveLine(list, top0, bottom0, origin, size, color);
            }
        }
    }

    dx::Vector Editor::rayDirection(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size)
    {
        float scale = size.x * 0.5f / std::tan(fov * dx::Pi / 360.0f);
        float right = (mouse.x - origin.x - size.x * 0.5f) / scale;
        float up = -(mouse.y - origin.y - size.y * 0.5f) / scale;
        dx::Coords axes = dx::rotationAxes(cameraRotation);
        return (axes.xAxis + axes.yAxis * right + axes.zAxis * up).normal();
    }

    dx::UObject* Editor::pickPerspective(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size)
    {
        dx::Vector direction = rayDirection(mouse, origin, size);
        dx::UObject* best = nullptr;
        float bestT = 1e9f;
        for (dx::UObject* actor : actors) {
            if (actor->isA(dx::Name("Brush")) || actor->isA(dx::Name("ZoneInfo")) || dx::actor::bHiddenEd(actor)) {
                continue;
            }
            dx::Vector center = dx::actor::Location(actor);
            float radius = std::max(dx::actor::CollisionRadius(actor), 8.0f);
            float height = std::max(dx::actor::CollisionHeight(actor), 8.0f);
            dx::Vector delta = center - cameraLocation;
            float dx2 = direction.x * direction.x + direction.y * direction.y;
            if (dx2 < 1e-6f) {
                continue;
            }
            float b = delta.x * direction.x + delta.y * direction.y;
            float c = delta.x * delta.x + delta.y * delta.y - radius * radius;
            float disc = b * b - dx2 * c;
            if (disc < 0) {
                continue;
            }
            float t = (b - std::sqrt(disc)) / dx2;
            if (t < 0) {
                t = (b + std::sqrt(disc)) / dx2;
            }
            if (t < 0 || t >= bestT) {
                continue;
            }
            float z = cameraLocation.z + direction.z * t;
            if (z < center.z - height || z > center.z + height) {
                continue;
            }
            bestT = t;
            best = actor;
        }
        return best;
    }

    int32_t Editor::pickSurface(const dx::Vector& direction)
    {
        dx::UModel* model = level ? level->model : nullptr;
        if (!model) {
            return -1;
        }
        int32_t best = -1;
        float bestT = 1e9f;
        for (const dx::BspNode& node : model->nodes) {
            if (node.numVertices < 3 || node.surf < 0) {
                continue;
            }
            dx::Vector normal { node.plane.x, node.plane.y, node.plane.z };
            float facing = normal.dot(direction);
            if (facing >= 0) {
                continue;
            }
            float t = (node.plane.w - normal.dot(cameraLocation)) / facing;
            if (t <= 0 || t >= bestT) {
                continue;
            }
            dx::Vector hit = cameraLocation + direction * t;
            bool inside = true;
            for (int32_t i = 0; i < node.numVertices; i++) {
                const dx::Vector& a = model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + i)].vertex)];
                const dx::Vector& b = model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + (i + 1) % node.numVertices)].vertex)];
                if ((b - a).cross(hit - a).dot(normal) < -0.01f) {
                    inside = false;
                    break;
                }
            }
            if (inside) {
                bestT = t;
                best = node.surf;
            }
        }
        return best;
    }

    void Editor::drawSelectedSurfaces(ImDrawList* list, const ImVec2& origin, const ImVec2& size)
    {
        dx::UModel* model = level ? level->model : nullptr;
        if (!model || selectedSurfs.empty()) {
            return;
        }
        for (const dx::BspNode& node : model->nodes) {
            if (node.numVertices < 3 || !selectedSurfs.count(node.surf)) {
                continue;
            }
            for (int32_t i = 0; i < node.numVertices; i++) {
                const dx::Vector& a = model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + i)].vertex)];
                const dx::Vector& b = model->points[static_cast<size_t>(model->verts[static_cast<size_t>(node.vertPool + (i + 1) % node.numVertices)].vertex)];
                drawPerspectiveLine(list, a, b, origin, size, IM_COL32(80, 255, 80, 255));
            }
        }
    }

    void Editor::perspectiveWindow(const ImVec2& position, const ImVec2& size)
    {
        ImGui::SetNextWindowPos(position);
        ImGui::SetNextWindowSize(size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(paneTitle.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);
        paneMenu(currentPane);
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 area = ImGui::GetContentRegionAvail();
        perspectiveOrigin = origin;
        perspectiveSize = area;
        viewRects[paneKey] = { origin, area };
        int width = std::max(static_cast<int>(area.x), 1);
        int height = std::max(static_cast<int>(area.y), 1);
        renderPerspective(width, height);
        ImGui::Image(static_cast<ImTextureID>(colorTexture), ImVec2(static_cast<float>(width), static_cast<float>(height)), ImVec2(0, 1), ImVec2(1, 0));
        bool hovered = ImGui::IsItemHovered();
        ImDrawList* list = ImGui::GetWindowDrawList();
        if (perspectiveMode == dx::RenderMode::Wireframe) {
            for (dx::UObject* actor : actors) {
                if (!isBrush(actor) || actor == selected) {
                    continue;
                }
                for (const Edge& edge : cachedBrushEdges(actor)) {
                    drawPerspectiveLine(list, edge.a, edge.b, origin, area, brushColor(actor));
                }
            }
        }
        drawPerspectivePaths(list, origin, area);
        if (selected) {
            if (isBrush(selected)) {
                for (const Edge& edge : brushEdges(selected)) {
                    drawPerspectiveLine(list, edge.a, edge.b, origin, area, IM_COL32(255, 255, 0, 255));
                }
            } else {
                drawCylinder(list, selected, origin, area);
            }
        }
        drawSelectedSurfaces(list, origin, area);
        drawSelectionPerspective(list, origin, area);
        if (editMode != 0) {
            dx::UObject* target = editBrush();
            if (target && target != selected) {
                for (const Edge& edge : brushEdges(target)) {
                    drawPerspectiveLine(list, edge.a, edge.b, origin, area, brushColor(target));
                }
            }
            std::vector<dx::Vector> hoverWorld = hovered && !componentDragging ? hoveredComponent(ImGui::GetIO().MousePos) : std::vector<dx::Vector>();
            drawComponents(list, [&](const dx::Vector& world, ImVec2& out) { return perspectivePoint(world, out); }, hoverWorld, true);
        }
        bool gizmoUsed = editMode == 0 ? perspectiveGizmo(origin, area) : componentGizmo(origin, area);
        char text[160];
        std::snprintf(text, sizeof(text), "%s  %.0f %.0f %.0f  speed %.0f%s", dx::renderModeName(perspectiveMode), static_cast<double>(cameraLocation.x), static_cast<double>(cameraLocation.y), static_cast<double>(cameraLocation.z), static_cast<double>(moveSpeed), surfaceMode ? "  surface mode" : "");
        list->AddText(ImVec2(origin.x + 6, origin.y + 4), IM_COL32(255, 255, 255, 220), text);
        bool overToolbar = editToolbar(origin);
        hovered = hovered && !overToolbar;
        ImGuiIO& io = ImGui::GetIO();
        if (hovered) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !gizmoUsed) {
                if (editMode != 0) {
                    pickPerspectiveComponent(io.MousePos, origin, area, io.KeyCtrl);
                } else if (surfaceMode || io.KeyShift) {
                    int32_t surf = pickSurface(rayDirection(io.MousePos, origin, area));
                    if (!io.KeyCtrl) {
                        selectedSurfs.clear();
                    }
                    if (surf >= 0) {
                        selectedSurfs.insert(surf);
                    }
                    status = "selected " + std::to_string(selectedSurfs.size()) + " surfaces";
                } else {
                    clickSelect(pickPerspective(io.MousePos, origin, area), io.KeyCtrl);
                }
            }
            if (io.MouseWheel != 0) {
                moveSpeed = std::clamp(moveSpeed * (io.MouseWheel > 0 ? 1.25f : 0.8f), 32.0f, 8192.0f);
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                cameraRotation.yaw += static_cast<int32_t>(io.MouseDelta.x * 40.0f);
                cameraRotation.pitch = std::clamp(cameraRotation.pitch - static_cast<int32_t>(io.MouseDelta.y * 40.0f), -16000, 16000);
            }
            if (!io.KeyCtrl) {
                dx::Coords axes = dx::rotationAxes(cameraRotation);
                dx::Vector move;
                if (ImGui::IsKeyDown(ImGuiKey_W)) {
                    move += axes.xAxis;
                }
                if (ImGui::IsKeyDown(ImGuiKey_S)) {
                    move -= axes.xAxis;
                }
                if (ImGui::IsKeyDown(ImGuiKey_D)) {
                    move += axes.yAxis;
                }
                if (ImGui::IsKeyDown(ImGuiKey_A)) {
                    move -= axes.yAxis;
                }
                if (ImGui::IsKeyDown(ImGuiKey_E)) {
                    move += dx::Vector { 0, 0, 1 };
                }
                if (ImGui::IsKeyDown(ImGuiKey_Q)) {
                    move -= dx::Vector { 0, 0, 1 };
                }
                cameraLocation += move * (moveSpeed * io.DeltaTime);
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < 16.0f && !selectedSurfs.empty()) {
                ImGui::OpenPopup("SurfacePopup");
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < 16.0f) {
                perspectiveContext(io.MousePos, origin, area);
            }
        }
        surfacePopup();
        selectionPopup();
        ImGui::End();
        ImGui::PopStyleVar();
    }

    ImVec2 Editor::orthoToScreen(const OrthoView& view, const dx::Vector& point, const ImVec2& origin, const ImVec2& size)
    {
        float h = (component(point, view.horizontal) - view.centerH) * view.zoom;
        float v = (component(point, view.vertical) - view.centerV) * view.zoom * view.verticalSign;
        return ImVec2(origin.x + size.x * 0.5f + h, origin.y + size.y * 0.5f + v);
    }

    void Editor::orthoWindow(OrthoView& view, const ImVec2& position, const ImVec2& size)
    {
        ImGui::SetNextWindowPos(position);
        ImGui::SetNextWindowSize(size);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(paneTitle.c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar);
        paneMenu(currentPane);
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 area = ImGui::GetContentRegionAvail();
        area.x = std::max(area.x, 1.0f);
        area.y = std::max(area.y, 1.0f);
        viewRects[paneKey] = { origin, area };
        ImGui::InvisibleButton("canvas", area, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
        bool hovered = ImGui::IsItemHovered();
        ImDrawList* list = ImGui::GetWindowDrawList();
        ImVec2 end(origin.x + area.x, origin.y + area.y);
        list->PushClipRect(origin, end, true);
        list->AddRectFilled(origin, end, IM_COL32(20, 20, 24, 255));
        float grid = 128.0f;
        while (grid * view.zoom < 12.0f) {
            grid *= 2.0f;
        }
        float halfH = area.x * 0.5f / view.zoom;
        float halfV = area.y * 0.5f / view.zoom;
        for (float g = std::floor((view.centerH - halfH) / grid) * grid; g <= view.centerH + halfH; g += grid) {
            float sx = origin.x + area.x * 0.5f + (g - view.centerH) * view.zoom;
            list->AddLine(ImVec2(sx, origin.y), ImVec2(sx, end.y), std::fmod(std::fabs(g), grid * 8.0f) < 0.5f ? IM_COL32(70, 70, 80, 255) : IM_COL32(40, 40, 48, 255));
        }
        for (float g = std::floor((view.centerV - halfV) / grid) * grid; g <= view.centerV + halfV; g += grid) {
            float sy = origin.y + area.y * 0.5f + (g - view.centerV) * view.zoom * view.verticalSign;
            list->AddLine(ImVec2(origin.x, sy), ImVec2(end.x, sy), std::fmod(std::fabs(g), grid * 8.0f) < 0.5f ? IM_COL32(70, 70, 80, 255) : IM_COL32(40, 40, 48, 255));
        }
        for (Edge edge : edges) {
            if (!zClipEdge(view, edge)) {
                continue;
            }
            ImVec2 a = orthoToScreen(view, edge.a, origin, area);
            ImVec2 b = orthoToScreen(view, edge.b, origin, area);
            if ((a.x < origin.x && b.x < origin.x) || (a.x > end.x && b.x > end.x) || (a.y < origin.y && b.y < origin.y) || (a.y > end.y && b.y > end.y)) {
                continue;
            }
            list->AddLine(a, b, IM_COL32(150, 150, 150, 255));
        }
        for (dx::UObject* actor : actors) {
            if (!isBrush(actor) || actorHidden(actor, view)) {
                continue;
            }
            ImU32 color = isSelected(actor) ? IM_COL32(255, 255, 0, 255) : brushColor(actor);
            for (Edge edge : cachedBrushEdges(actor)) {
                if (!zClipEdge(view, edge)) {
                    continue;
                }
                list->AddLine(orthoToScreen(view, edge.a, origin, area), orthoToScreen(view, edge.b, origin, area), color);
            }
        }
        drawOrthoPaths(list, view, origin, area);
        for (dx::UObject* actor : actors) {
            if (isBrush(actor) || actorHidden(actor, view)) {
                continue;
            }
            ImVec2 p = orthoToScreen(view, dx::actor::Location(actor), origin, area);
            if (p.x < origin.x || p.x > end.x || p.y < origin.y || p.y > end.y) {
                continue;
            }
            bool icon = drawActorIcon(list, actor, p, view.zoom);
            if (isSelected(actor)) {
                if (!icon) {
                    list->AddRectFilled(ImVec2(p.x - 4, p.y - 4), ImVec2(p.x + 4, p.y + 4), IM_COL32(255, 255, 0, 255));
                }
                float radius = dx::actor::CollisionRadius(actor) * view.zoom;
                if (radius > 3.0f) {
                    list->AddCircle(p, radius, IM_COL32(255, 255, 0, 255));
                }
            } else if (!icon) {
                list->AddRectFilled(ImVec2(p.x - 2, p.y - 2), ImVec2(p.x + 2, p.y + 2), IM_COL32(255, 90, 90, 255));
            }
        }
        char label[96];
        std::snprintf(label, sizeof(label), "%s  %.0f %.0f", view.title, static_cast<double>(view.centerH), static_cast<double>(view.centerV));
        list->AddText(ImVec2(origin.x + 6, origin.y + 4), IM_COL32(255, 255, 255, 220), label);
        orthoRightClick(view, origin, area, hovered);
        bool brushToolUsed = brushEditOrtho(view, origin, area, hovered);
        list->PopClipRect();
        ImGuiIO& io = ImGui::GetIO();
        if (hovered) {
            if (io.MouseWheel != 0) {
                view.zoom = std::clamp(view.zoom * (io.MouseWheel > 0 ? 1.25f : 0.8f), 0.002f, 8.0f);
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !brushToolUsed) {
                dx::UObject* best = nullptr;
                float bestDistance = 64.0f;
                for (dx::UObject* actor : actors) {
                    if (actorHidden(actor, view)) {
                        continue;
                    }
                    ImVec2 p = orthoToScreen(view, dx::actor::Location(actor), origin, area);
                    float distance = (p.x - io.MousePos.x) * (p.x - io.MousePos.x) + (p.y - io.MousePos.y) * (p.y - io.MousePos.y);
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        best = actor;
                    }
                }
                orthoPress(view, best);
            }
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f)) {
            view.centerH -= io.MouseDelta.x / view.zoom;
            view.centerV -= io.MouseDelta.y / view.zoom * view.verticalSign;
        }
        if (ImGui::IsItemActive() && dragging && selected && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
            componentRef(dragRaw, view.horizontal) += io.MouseDelta.x / view.zoom;
            componentRef(dragRaw, view.vertical) += io.MouseDelta.y / view.zoom * view.verticalSign;
            translateSelection(snapped(dragRaw) - dx::actor::Location(selected));
        }
        orthoBox(view, origin, area);
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void Editor::finishDrag()
    {
        if (dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            dragging = false;
            if (history.recording()) {
                if (selected && !(dx::actor::Location(selected) == dragStart)) {
                    for (dx::UObject* actor : selectionList()) {
                        noteMoved(actor);
                    }
                    history.commit();
                } else {
                    history.cancel();
                }
            }
        }
    }

    void Editor::moveSelectionWithKeys()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput) {
            return;
        }
        if (io.KeyCtrl) {
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
                command("undo");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
                command("redo");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_S, false)) {
                command("save");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_D, false)) {
                command("duplicate");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
                command("copy");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_X, false)) {
                command("cut");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_V, false)) {
                command("paste");
            }
            if (ImGui::IsKeyPressed(ImGuiKey_A, false)) {
                command("selectall");
            }
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            command("delete");
        }
        if (!selected) {
            return;
        }
        dx::Vector step;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
            step.x -= 16.0f;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            step.x += 16.0f;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            step.y -= 16.0f;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            step.y += 16.0f;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
            step.z += 16.0f;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
            step.z -= 16.0f;
        }
        if (!step.zero()) {
            char text[96];
            std::snprintf(text, sizeof(text), "move %g %g %g", static_cast<double>(step.x), static_cast<double>(step.y), static_cast<double>(step.z));
            command(text);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket)) {
            command("rotate 0 -2048 0");
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightBracket)) {
            command("rotate 0 2048 0");
        }
    }

    void Editor::gridWindow()
    {
        if (!showGrid) {
            return;
        }
        if (!ImGui::Begin("Grid", &showGrid, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            return;
        }
        dx::brushbuilder::Constraints& constraints = dx::brushbuilder::constraints();
        ImGui::Checkbox("Snap to grid", &constraints.gridEnabled);
        float size = constraints.gridSize.x;
        if (ImGui::InputFloat("Grid size", &size, 1.0f, 16.0f, "%.0f")) {
            size = std::max(size, 1.0f);
            constraints.gridSize = { size, size, size };
        }
        ImGui::Checkbox("Snap rotation", &constraints.rotGridEnabled);
        int rotation = constraints.rotGridSize.yaw;
        if (ImGui::InputInt("Rotation grid", &rotation, 256, 1024)) {
            rotation = std::max(rotation, 1);
            constraints.rotGridSize = { rotation, rotation, rotation };
        }
        ImGui::End();
    }

    dx::Vector Editor::snapped(const dx::Vector& location)
    {
        const dx::brushbuilder::Constraints& constraints = dx::brushbuilder::constraints();
        if (!constraints.gridEnabled) {
            return location;
        }
        auto snap = [](float value, float grid) {
            return grid == 0.0f ? value : std::floor((value + 0.5f * grid) / grid) * grid;
        };
        return { snap(location.x, constraints.gridSize.x), snap(location.y, constraints.gridSize.y), snap(location.z, constraints.gridSize.z) };
    }
}
