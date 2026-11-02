#include "editor/editor.h"
#include "engine/actor_selection.h"
#include "engine/t3d.h"

#include <ImGuizmo.h>

namespace dx::editor {
    bool Editor::isSelected(dx::UObject* actor)
    {
        return actor && (actor == selected || selectedActors.count(actor) > 0);
    }

    std::vector<dx::UObject*> Editor::selectionList()
    {
        std::vector<dx::UObject*> result;
        for (dx::UObject* actor : actors) {
            if (isSelected(actor)) {
                result.push_back(actor);
            }
        }
        return result;
    }

    std::vector<dx::UObject*> Editor::editableSelection()
    {
        std::vector<dx::UObject*> result;
        for (dx::UObject* actor : selectionList()) {
            if (actor != levelInfo() && actor != builderBrush()) {
                result.push_back(actor);
            }
        }
        return result;
    }

    void Editor::setSelection(const std::vector<dx::UObject*>& chosen)
    {
        bool keepPrimary = std::find(chosen.begin(), chosen.end(), selected) != chosen.end();
        select(keepPrimary ? selected : (chosen.empty() ? nullptr : chosen.front()));
        selectedActors.insert(chosen.begin(), chosen.end());
        status = "selected " + std::to_string(selectedActors.size()) + " actors";
    }

    void Editor::toggleSelect(dx::UObject* actor)
    {
        if (!actor) {
            return;
        }
        selectionChanged = true;
        editingKey = nullptr;
        if (!isSelected(actor)) {
            selectedActors.insert(actor);
            selected = actor;
            return;
        }
        selectedActors.erase(actor);
        if (actor == selected) {
            std::vector<dx::UObject*> remaining = selectionList();
            selected = remaining.empty() ? nullptr : remaining.front();
        }
    }

    void Editor::clickSelect(dx::UObject* actor, bool toggle)
    {
        if (toggle) {
            toggleSelect(actor);
        } else {
            select(actor);
        }
    }

    void Editor::translateSelection(const dx::Vector& delta)
    {
        for (dx::UObject* actor : selectionList()) {
            dx::actor::Location(actor) += delta;
            brushCache.erase(actor);
        }
    }

    void Editor::rotateSelection(const dx::Rotator& delta)
    {
        for (dx::UObject* actor : selectionList()) {
            dx::Rotator& rotation = dx::actor::Rotation(actor);
            rotation.pitch += delta.pitch;
            rotation.yaw += delta.yaw;
            rotation.roll += delta.roll;
            brushCache.erase(actor);
        }
    }

    bool Editor::actorHidden(dx::UObject* actor, const OrthoView& view)
    {
        if (dx::actor::bHiddenEd(actor)) {
            return true;
        }
        if (!zClip || view.horizontal == 2 || view.vertical == 2 || isBrush(actor)) {
            return false;
        }
        float z = dx::actor::Location(actor).z;
        return z < zClipRange[0] || z > zClipRange[1];
    }

    bool Editor::zClipEdge(const OrthoView& view, Edge& edge)
    {
        if (!zClip || view.horizontal == 2 || view.vertical == 2) {
            return true;
        }
        return dx::actorselect::clipZ(edge.a, edge.b, zClipRange[0], zClipRange[1]);
    }

    bool Editor::selectionCommand(const std::string& verb, const std::string& rest)
    {
        std::istringstream values(rest);
        if (verb == "selectall") {
            setSelection(dx::actorselect::visible(actors));
            return true;
        }
        if (verb == "selectnone") {
            setSelection({});
            return true;
        }
        if (verb == "selectinvert") {
            std::vector<dx::UObject*> current = selectionList();
            setSelection(dx::actorselect::inverted(actors, std::unordered_set<dx::UObject*>(current.begin(), current.end()), builderBrush()));
            return true;
        }
        if (verb == "selectclass") {
            dx::UClass* cls = rest.empty() ? (selected ? selected->cls : nullptr) : resolveClass(rest);
            if (!cls) {
                status = "no class " + rest;
                return false;
            }
            setSelection(dx::actorselect::ofClass(actors, cls));
            return true;
        }
        if (verb == "selectadd" || verb == "selecttoggle") {
            for (dx::UObject* actor : actors) {
                if (lower(actor->name.str()) == lower(rest)) {
                    if (verb == "selecttoggle" || !isSelected(actor)) {
                        toggleSelect(actor);
                    }
                    status = "selected " + std::to_string(selectionList().size()) + " actors";
                    return true;
                }
            }
            status = "no actor " + rest;
            return false;
        }
        if (verb == "boxselect") {
            std::string view;
            float h0 = 0;
            float v0 = 0;
            float h1 = 0;
            float v1 = 0;
            values >> view >> h0 >> v0 >> h1 >> v1;
            int horizontal = view == "side" ? 1 : 0;
            int vertical = view == "top" ? 1 : 2;
            setSelection(dx::actorselect::inBox(actors, horizontal, vertical, h0, v0, h1, v1));
            return true;
        }
        if (verb == "copy" || verb == "cut") {
            std::vector<dx::UObject*> chosen = editableSelection();
            if (chosen.empty()) {
                status = "nothing to " + verb;
                return false;
            }
            SDL_SetClipboardText(dx::t3d::exportActors(chosen).c_str());
            status = "copied " + std::to_string(chosen.size()) + " actors";
            return verb == "copy" || command("delete");
        }
        if (verb == "paste") {
            char* clipboard = SDL_GetClipboardText();
            std::string text = clipboard ? clipboard : "";
            SDL_free(clipboard);
            if (text.find("Begin Actor") == std::string::npos) {
                status = "clipboard has no T3D actors";
                return false;
            }
            history.begin(level, {}, "Paste");
            std::vector<dx::UObject*> created = dx::t3d::importActors(level, text, false);
            if (created.empty()) {
                history.cancel();
                status = "nothing pasted";
                return false;
            }
            dx::Vector point;
            if (values >> point.x >> point.y >> point.z) {
                dx::Vector delta = snapped(point) - dx::actor::Location(created.front());
                for (dx::UObject* actor : created) {
                    dx::actor::Location(actor) += delta;
                }
            }
            history.commit();
            refreshActors();
            setSelection(created);
            status = "pasted " + std::to_string(created.size()) + " actors";
            return true;
        }
        if (verb == "addat") {
            std::string className;
            dx::Vector point;
            values >> className >> point.x >> point.y >> point.z;
            dx::UClass* cls = resolveClass(className);
            if (!cls || (cls->classFlags & dx::CLASS_Abstract) || !actorClass || !cls->isChildOf(actorClass)) {
                status = "cannot add " + className;
                return false;
            }
            history.begin(level, {}, "Add " + cls->name.str());
            dx::UObject* actor = spawnActor(cls, snapped(point), { 0, cameraRotation.yaw, 0 }, nullptr);
            history.commit();
            refreshActors();
            select(actor);
            status = "added " + actor->name.str();
            return true;
        }
        if (verb == "exportt3d") {
            std::string file;
            std::string scope;
            values >> file >> scope;
            std::vector<dx::UObject*> chosen = scope == "selected" ? selectionList() : actors;
            std::ofstream out(file, std::ios::binary);
            out << dx::t3d::exportActors(chosen);
            status = out ? "exported " + std::to_string(chosen.size()) + " actors to " + file : "cannot write " + file;
            return static_cast<bool>(out);
        }
        if (verb == "importt3d") {
            std::ifstream in(rest, std::ios::binary);
            if (!in) {
                status = "cannot read " + rest;
                return false;
            }
            std::stringstream text;
            text << in.rdbuf();
            std::string name = std::filesystem::path(rest).stem().string();
            if (name == currentMap || dx::objects::findLinker(dx::Name(name))) {
                name += "_Import";
            }
            if (!newMap(name)) {
                return false;
            }
            std::vector<dx::UObject*> created = dx::t3d::importActors(level, text.str(), true);
            refreshActors();
            placeCamera();
            command("rebuild");
            status = "imported " + std::to_string(created.size()) + " actors from " + rest;
            return !created.empty();
        }
        if (verb == "hide") {
            std::vector<dx::UObject*> targets;
            for (dx::UObject* actor : actors) {
                bool chosen = rest == "unselected" ? !isSelected(actor) : isSelected(actor);
                if (chosen && actor != builderBrush() && !dx::actor::bHiddenEd(actor)) {
                    targets.push_back(actor);
                }
            }
            if (targets.empty()) {
                status = "nothing to hide";
                return false;
            }
            history.begin(level, targets, rest == "unselected" ? "Hide Unselected" : "Hide Selected");
            for (dx::UObject* actor : targets) {
                dx::actor::bHiddenEd(actor) = 1;
            }
            history.commit();
            setSelection({});
            status = "hid " + std::to_string(targets.size()) + " actors";
            return true;
        }
        if (verb == "unhide") {
            std::vector<dx::UObject*> targets = dx::actorselect::toUnhide(actors);
            if (targets.empty()) {
                status = "no hidden actors";
                return false;
            }
            history.begin(level, targets, "Show All Actors");
            for (dx::UObject* actor : targets) {
                dx::actor::bHiddenEd(actor) = 0;
            }
            history.commit();
            status = "showing " + std::to_string(targets.size()) + " hidden actors";
            return true;
        }
        if (verb == "zclip") {
            if (rest == "off") {
                zClip = false;
                return true;
            }
            if (rest == "brush") {
                dx::UObject* builder = builderBrush();
                if (!builder) {
                    return false;
                }
                zClipRange[0] = FLT_MAX;
                zClipRange[1] = -FLT_MAX;
                for (const Edge& edge : brushEdges(builder)) {
                    zClipRange[0] = std::min({ zClipRange[0], edge.a.z, edge.b.z });
                    zClipRange[1] = std::max({ zClipRange[1], edge.a.z, edge.b.z });
                }
                zClip = zClipRange[0] <= zClipRange[1];
                return zClip;
            }
            values >> zClipRange[0] >> zClipRange[1];
            zClip = true;
            return true;
        }
        if (verb == "gizmo") {
            gizmoMode = rest == "off" ? 0 : rest == "rotate" ? 2
                : rest == "scale"                            ? 3
                : rest == "extrude"                          ? 4
                                                             : 1;
            if (gizmoMode == 4 && editMode != 3) {
                gizmoMode = 1;
                status = "extrude needs face mode";
                return false;
            }
            return true;
        }
        status = "unknown selection command " + verb;
        return false;
    }

    void Editor::drawSelectionPerspective(ImDrawList* list, const ImVec2& origin, const ImVec2& size)
    {
        for (dx::UObject* actor : selectionList()) {
            if (actor == selected) {
                continue;
            }
            if (isBrush(actor)) {
                for (const Edge& edge : brushEdges(actor)) {
                    drawPerspectiveLine(list, edge.a, edge.b, origin, size, IM_COL32(255, 255, 0, 255));
                }
            } else {
                drawCylinder(list, actor, origin, size);
            }
        }
    }

    bool Editor::perspectiveGizmo(const ImVec2& origin, const ImVec2& size)
    {
        if (gizmoMode == 0 || !selected || size.x < 2.0f || size.y < 2.0f) {
            return false;
        }
        ImGuizmo::BeginFrame();
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(origin.x, origin.y, size.x, size.y);
        float view[16];
        float projection[16];
        gizmoMatrices(size, view, projection);
        bool brush = isBrush(selected);
        dx::Vector scale = brush ? dx::brushedit::mainScale(selected) : dx::Vector { dx::actor::DrawScale(selected), dx::actor::DrawScale(selected), dx::actor::DrawScale(selected) };
        dx::Coords axes = dx::rotationAxes(dx::actor::Rotation(selected));
        if (gizmoMode == 3) {
            axes.xAxis = axes.xAxis * std::fabs(scale.x);
            axes.yAxis = axes.yAxis * std::fabs(scale.y);
            axes.zAxis = axes.zAxis * std::fabs(scale.z);
        }
        dx::Vector location = dx::actor::Location(selected);
        float matrix[16] = { axes.xAxis.x, axes.xAxis.y, axes.xAxis.z, 0, axes.yAxis.x, axes.yAxis.y, axes.yAxis.z, 0, axes.zAxis.x, axes.zAxis.y, axes.zAxis.z, 0, location.x, location.y, location.z, 1 };
        const dx::brushbuilder::Constraints& constraints = dx::brushbuilder::constraints();
        float snap[3] = { static_cast<float>(constraints.rotGridSize.yaw) * 360.0f / 65536.0f, 0.0f, 0.0f };
        bool snapping = gizmoMode == 2 && constraints.rotGridEnabled;
        ImGuizmo::OPERATION operation = gizmoMode == 2 ? ImGuizmo::ROTATE : gizmoMode == 3 ? ImGuizmo::SCALE
                                                                                           : ImGuizmo::TRANSLATE;
        ImGuizmo::MODE mode = gizmoMode == 2 || gizmoMode == 3 ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
        bool changed = ImGuizmo::Manipulate(view, projection, operation, mode, matrix, nullptr, snapping ? snap : nullptr);
        bool usingGizmo = ImGuizmo::IsUsing();
        if (usingGizmo && !gizmoActive) {
            gizmoActive = true;
            gizmoStart = location;
            gizmoStartRotation = dx::actor::Rotation(selected);
            gizmoStartScale = scale;
            history.begin(level, selectionList(), (gizmoMode == 2 ? "Rotate " : gizmoMode == 3 ? "Scale "
                                                                                               : "Move ")
                    + selected->name.str());
        }
        if (changed && gizmoActive) {
            dx::Vector target { matrix[12], matrix[13], matrix[14] };
            translateSelection((gizmoMode == 1 ? snapped(target) : target) - location);
            if (gizmoMode == 3) {
                dx::Vector factor { dx::Vector { matrix[0], matrix[1], matrix[2] }.size() / std::max(std::fabs(scale.x), 0.0001f), dx::Vector { matrix[4], matrix[5], matrix[6] }.size() / std::max(std::fabs(scale.y), 0.0001f), dx::Vector { matrix[8], matrix[9], matrix[10] }.size() / std::max(std::fabs(scale.z), 0.0001f) };
                float uniform = factor.x;
                for (float value : { factor.y, factor.z }) {
                    if (std::fabs(value - 1.0f) > std::fabs(uniform - 1.0f)) {
                        uniform = value;
                    }
                }
                for (dx::UObject* actor : selectionList()) {
                    if (isBrush(actor)) {
                        dx::Vector current = dx::brushedit::mainScale(actor);
                        dx::brushedit::setMainScale(actor, { current.x * factor.x, current.y * factor.y, current.z * factor.z });
                        brushCache.erase(actor);
                    } else {
                        dx::actor::DrawScale(actor) *= uniform;
                    }
                }
            }
            if (gizmoMode == 2) {
                dx::Coords turned;
                turned.xAxis = dx::Vector { matrix[0], matrix[1], matrix[2] }.normal();
                turned.yAxis = dx::Vector { matrix[4], matrix[5], matrix[6] }.normal();
                turned.zAxis = dx::Vector { matrix[8], matrix[9], matrix[10] }.normal();
                rotateSelection(dx::actorselect::rotationDelta(dx::actor::Rotation(selected), dx::actorselect::rotatorFromAxes(turned)));
            }
        }
        if (usingGizmo && gizmoActive) {
            if (gizmoMode == 2) {
                dx::Rotator turned = dx::actorselect::rotationDelta(gizmoStartRotation, dx::actor::Rotation(selected));
                auto degrees = [](int32_t value) {
                    return formatNumber(static_cast<float>(static_cast<int16_t>(value)) * 360.0f / 65536.0f) + "\xc2\xb0";
                };
                drawReadout(withIcon(icons::Angle, ("pitch " + degrees(turned.pitch) + "  yaw " + degrees(turned.yaw) + "  roll " + degrees(turned.roll)).c_str()));
            } else if (gizmoMode == 3) {
                dx::Vector now = isBrush(selected) ? dx::brushedit::mainScale(selected) : dx::Vector { dx::actor::DrawScale(selected), dx::actor::DrawScale(selected), dx::actor::DrawScale(selected) };
                drawReadout(withIcon(icons::Scale, (formatNumber(now.x) + " x " + formatNumber(now.y) + " x " + formatNumber(now.z)).c_str()));
            } else {
                dx::Vector moved = dx::actor::Location(selected) - gizmoStart;
                drawReadout(withIcon(icons::Move, (formatNumber(moved.x) + ", " + formatNumber(moved.y) + ", " + formatNumber(moved.z) + "   " + icons::Ruler + " " + formatNumber(moved.size())).c_str()));
            }
        }
        if (!usingGizmo && gizmoActive) {
            gizmoActive = false;
            dx::Vector endScale = isBrush(selected) ? dx::brushedit::mainScale(selected) : dx::Vector { dx::actor::DrawScale(selected), dx::actor::DrawScale(selected), dx::actor::DrawScale(selected) };
            if (history.recording() && (!(dx::actor::Location(selected) == gizmoStart) || !(dx::actor::Rotation(selected) == gizmoStartRotation) || !(endScale == gizmoStartScale))) {
                for (dx::UObject* actor : selectionList()) {
                    noteMoved(actor);
                }
                history.commit();
            } else if (history.recording()) {
                history.cancel();
            }
        }
        return usingGizmo || ImGuizmo::IsOver();
    }

    void Editor::perspectiveContext(const ImVec2& mouse, const ImVec2& origin, const ImVec2& size)
    {
        dx::Vector direction = rayDirection(mouse, origin, size);
        int32_t surf = pickSurface(direction);
        float surfDistance = FLT_MAX;
        dx::Vector normal;
        dx::UModel* model = level ? level->model : nullptr;
        if (surf >= 0 && model) {
            const dx::BspSurf& hit = model->surfs[static_cast<size_t>(surf)];
            normal = model->vectors[static_cast<size_t>(hit.vNormal)];
            surfDistance = (model->points[static_cast<size_t>(hit.pBase)] - cameraLocation).dot(normal) / direction.dot(normal);
        }
        dx::UObject* actor = nullptr;
        float scale = size.x * 0.5f / std::tan(fov * dx::Pi / 360.0f);
        for (dx::UObject* candidate : actors) {
            dx::Vector camera;
            if (isBrush(candidate) || dx::actor::bHiddenEd(candidate) || !projectCamera(dx::actor::Location(candidate), camera) || camera.z >= surfDistance) {
                continue;
            }
            ImVec2 point = cameraToScreen(camera, origin, size);
            float reach = std::clamp(dx::actor::CollisionRadius(candidate) * scale / camera.z, 16.0f, 48.0f);
            float dxScreen = point.x - mouse.x;
            float dyScreen = point.y - mouse.y;
            if (dxScreen * dxScreen + dyScreen * dyScreen < reach * reach) {
                surfDistance = camera.z;
                actor = candidate;
            }
        }
        if (actor) {
            contextActor = actor;
            if (!isSelected(actor)) {
                select(actor);
            }
            ImGui::OpenPopup("selectionContext");
            return;
        }
        contextActor = nullptr;
        if (surf >= 0 && model) {
            contextPoint = cameraLocation + direction * surfDistance + normal * 48.0f;
            if (!selectedSurfs.count(surf)) {
                selectedSurfs.clear();
                selectedSurfs.insert(surf);
            }
            ImGui::OpenPopup("SurfacePopup");
            return;
        }
        contextPoint = cameraLocation + direction * 256.0f;
        ImGui::OpenPopup("selectionContext");
    }

    void Editor::orthoPress(const OrthoView& view, dx::UObject* best)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (!best) {
            if (!io.KeyCtrl) {
                setSelection({});
            }
            boxSelecting = true;
            boxView = &view;
            boxStart = io.MousePos;
            return;
        }
        if (io.KeyCtrl) {
            toggleSelect(best);
            return;
        }
        if (isSelected(best)) {
            selected = best;
            selectionChanged = true;
        } else {
            select(best);
        }
        dragging = true;
        dragStart = dx::actor::Location(best);
        dragRaw = dragStart;
        std::vector<dx::UObject*> chosen = selectionList();
        history.begin(level, chosen, chosen.size() > 1 ? "Move " + std::to_string(chosen.size()) + " actors" : "Move " + best->name.str());
    }

    void Editor::orthoBox(const OrthoView& view, const ImVec2& origin, const ImVec2& size)
    {
        if (!boxSelecting || boxView != &view) {
            return;
        }
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 corner = io.MousePos;
        ImDrawList* list = ImGui::GetWindowDrawList();
        list->AddRectFilled(ImVec2(std::min(boxStart.x, corner.x), std::min(boxStart.y, corner.y)), ImVec2(std::max(boxStart.x, corner.x), std::max(boxStart.y, corner.y)), IM_COL32(255, 255, 0, 40));
        list->AddRect(ImVec2(std::min(boxStart.x, corner.x), std::min(boxStart.y, corner.y)), ImVec2(std::max(boxStart.x, corner.x), std::max(boxStart.y, corner.y)), IM_COL32(255, 255, 0, 200));
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            return;
        }
        boxSelecting = false;
        boxView = nullptr;
        if (std::fabs(corner.x - boxStart.x) < 4.0f && std::fabs(corner.y - boxStart.y) < 4.0f) {
            return;
        }
        auto worldH = [&](float x) {
            return view.centerH + (x - origin.x - size.x * 0.5f) / view.zoom;
        };
        auto worldV = [&](float y) {
            return view.centerV + (y - origin.y - size.y * 0.5f) / (view.zoom * view.verticalSign);
        };
        std::vector<dx::UObject*> shown;
        for (dx::UObject* actor : actors) {
            if (!actorHidden(actor, view)) {
                shown.push_back(actor);
            }
        }
        std::vector<dx::UObject*> chosen = dx::actorselect::inBox(shown, view.horizontal, view.vertical, worldH(boxStart.x), worldV(boxStart.y), worldH(corner.x), worldV(corner.y));
        if (io.KeyCtrl) {
            std::vector<dx::UObject*> current = selectionList();
            chosen.insert(chosen.end(), current.begin(), current.end());
        }
        setSelection(chosen);
    }

    void Editor::orthoRightClick(const OrthoView& view, const ImVec2& origin, const ImVec2& size, bool hovered)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (!hovered || !ImGui::IsMouseReleased(ImGuiMouseButton_Right) || io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] >= 16.0f) {
            return;
        }
        contextActor = nullptr;
        float bestDistance = 64.0f;
        for (dx::UObject* actor : actors) {
            if (actorHidden(actor, view)) {
                continue;
            }
            ImVec2 p = orthoToScreen(view, dx::actor::Location(actor), origin, size);
            float distance = (p.x - io.MousePos.x) * (p.x - io.MousePos.x) + (p.y - io.MousePos.y) * (p.y - io.MousePos.y);
            if (distance < bestDistance) {
                bestDistance = distance;
                contextActor = actor;
            }
        }
        if (contextActor && !isSelected(contextActor)) {
            select(contextActor);
        }
        contextPoint = cameraLocation;
        componentRef(contextPoint, view.horizontal) = view.centerH + (io.MousePos.x - origin.x - size.x * 0.5f) / view.zoom;
        componentRef(contextPoint, view.vertical) = view.centerV + (io.MousePos.y - origin.y - size.y * 0.5f) / (view.zoom * view.verticalSign);
    }

    void Editor::selectionContextItems()
    {
        if (contextActor && !contextActor->destroyed) {
            std::string className = contextActor->cls->name.str();
            size_t count = selectionList().size();
            if (count > 1) {
                ImGui::TextDisabled("%zu actors selected", count);
            } else {
                ImGui::TextDisabled("%s", contextActor->name.str().c_str());
            }
            if (ImGui::MenuItem((className + " Properties").c_str())) {
                ImGui::SetWindowFocus("Properties");
            }
            if (ImGui::MenuItem(("Select All " + className).c_str())) {
                command("selectclass " + contextActor->cls->pathName());
            }
            if (ImGui::MenuItem("Select None")) {
                command("selectnone");
            }
            if (ImGui::MenuItem("Invert Selection")) {
                command("selectinvert");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                command("cut");
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                command("copy");
            }
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                command("duplicate");
            }
            if (ImGui::MenuItem("Delete", "Del")) {
                command("delete");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Hide Selected")) {
                command("hide selected");
            }
            if (ImGui::MenuItem("Hide Unselected")) {
                command("hide unselected");
            }
            if (ImGui::MenuItem("Edit Script", nullptr, false, contextActor->cls->scriptText != nullptr)) {
                command("editscript " + contextActor->cls->pathName());
            }
            ImGui::Separator();
        }
        addHereItems();
        if (ImGui::MenuItem("Select All")) {
            command("selectall");
        }
        if (ImGui::MenuItem("Show All Actors")) {
            command("unhide");
        }
    }

    void Editor::addHereItems()
    {
        char point[128];
        std::snprintf(point, sizeof(point), " %g %g %g", static_cast<double>(contextPoint.x), static_cast<double>(contextPoint.y), static_cast<double>(contextPoint.z));
        if (placeClass && actorClass && placeClass->isChildOf(actorClass) && !(placeClass->classFlags & dx::CLASS_Abstract)) {
            if (ImGui::MenuItem(("Add " + placeClass->name.str() + " here").c_str())) {
                command("addat " + placeClass->pathName() + point);
            }
        }
        const std::pair<const char*, const char*> common[] = { { "Add Light here", "Engine.Light" }, { "Add PlayerStart here", "Engine.PlayerStart" }, { "Add PathNode here", "Engine.PathNode" } };
        for (const auto& [label, className] : common) {
            if (ImGui::MenuItem(label)) {
                command(std::string("addat ") + className + point);
            }
        }
        if (ImGui::MenuItem("Paste Here")) {
            command(std::string("paste") + point);
        }
    }

    void Editor::selectionPopup()
    {
        if (ImGui::BeginPopup("selectionContext")) {
            selectionContextItems();
            ImGui::EndPopup();
        }
    }

    void Editor::multiPropertiesPanel()
    {
        std::vector<dx::UObject*> chosen = selectionList();
        dx::UClass* common = dx::actorselect::commonClass(chosen);
        ImGui::Text("%zu actors selected", chosen.size());
        ImGui::Text("Common class %s", common ? common->pathName().c_str() : "None");
        dx::Vector location = dx::actor::Location(selected);
        dx::Rotator rotation = dx::actor::Rotation(selected);
        float position[3] = { location.x, location.y, location.z };
        int angles[3] = { rotation.pitch, rotation.yaw, rotation.roll };
        if (ImGui::DragFloat3("Location", position, 4.0f)) {
            translateSelection(dx::Vector { position[0], position[1], position[2] } - location);
        }
        if (ImGui::IsItemActivated() && !history.recording()) {
            history.begin(level, chosen, "Move " + std::to_string(chosen.size()) + " actors");
        }
        if (ImGui::IsItemDeactivated() && history.recording()) {
            history.commit();
        }
        if (ImGui::DragInt3("Rotation", angles, 64.0f)) {
            rotateSelection({ angles[0] - rotation.pitch, angles[1] - rotation.yaw, angles[2] - rotation.roll });
        }
        if (ImGui::IsItemActivated() && !history.recording()) {
            history.begin(level, chosen, "Rotate " + std::to_string(chosen.size()) + " actors");
        }
        if (ImGui::IsItemDeactivated() && history.recording()) {
            history.commit();
        }
        ImGui::Checkbox("Editable only", &editableOnly);
        if (!common) {
            return;
        }
        std::map<std::string, std::vector<dx::UProperty*>> categories;
        for (dx::UProperty* property : common->properties) {
            if (editableOnly && !(property->propertyFlags & dx::CPF_Edit)) {
                continue;
            }
            categories[property->category.none() ? std::string("None") : property->category.str()].push_back(property);
        }
        ImGui::BeginChild("properties");
        for (auto& category : categories) {
            if (!ImGui::CollapsingHeader(category.first.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                continue;
            }
            if (!ImGui::BeginTable(category.first.c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
                continue;
            }
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            for (dx::UProperty* property : category.second) {
                for (int32_t index = 0; index < property->arrayDim; index++) {
                    std::string label = property->name.str();
                    if (property->arrayDim > 1) {
                        label += "[" + std::to_string(index) + "]";
                    }
                    const void* key = selected->data + property->offset + static_cast<size_t>(index) * property->elementSize();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(label.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushID(key);
                    std::string value;
                    bool same = dx::actorselect::commonText(chosen, property, index, value);
                    if (editingKey == key && editingObject == nullptr) {
                        ImGui::SetNextItemWidth(-1);
                        if (focusEdit) {
                            ImGui::SetKeyboardFocusHere();
                            focusEdit = false;
                        }
                        if (ImGui::InputText("##edit", editBuffer.data(), editBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                            history.begin(level, chosen, "Set " + property->name.str());
                            if (dx::actorselect::setText(chosen, property, index, editBuffer.data())) {
                                history.commit();
                                status = "set " + property->name.str() + " on " + std::to_string(chosen.size()) + " actors";
                            } else {
                                history.cancel();
                                status = "cannot set " + property->name.str();
                            }
                            brushCache.clear();
                            editingKey = nullptr;
                        } else if (ImGui::IsItemDeactivated()) {
                            editingKey = nullptr;
                        }
                    } else {
                        bool editable = editableType(property) && property->type != dx::PropType::Array && property->type != dx::PropType::FixedArray;
                        std::string shown = same ? value : std::string("(multiple values)");
                        if (!editable || !same) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                        }
                        if (ImGui::Selectable(shown.empty() ? " " : shown.c_str(), false) && editable) {
                            editingKey = key;
                            editingObject = nullptr;
                            focusEdit = true;
                            std::snprintf(editBuffer.data(), editBuffer.size(), "%s", value.c_str());
                        }
                        if (!editable || !same) {
                            ImGui::PopStyleColor();
                        }
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }

    void Editor::selectionEditItems()
    {
        bool any = selected != nullptr;
        if (ImGui::MenuItem("Cut", "Ctrl+X", false, any)) {
            command("cut");
        }
        if (ImGui::MenuItem("Copy", "Ctrl+C", false, any)) {
            command("copy");
        }
        if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            command("paste");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Select All Actors", "Ctrl+A")) {
            command("selectall");
        }
        if (ImGui::MenuItem("Select None")) {
            command("selectnone");
        }
        if (ImGui::MenuItem("Invert Selection")) {
            command("selectinvert");
        }
        std::string ofClass = selected ? "Select All " + selected->cls->name.str() : std::string("Select All of Class");
        if (ImGui::MenuItem(ofClass.c_str(), nullptr, false, any)) {
            command("selectclass " + selected->cls->pathName());
        }
    }

    void Editor::actorMenu()
    {
        if (!ImGui::BeginMenu(withIcon(icons::Actor, "Actor").c_str())) {
            return;
        }
        bool any = selected != nullptr;
        if (ImGui::MenuItem("Hide Selected", nullptr, false, any)) {
            command("hide selected");
        }
        if (ImGui::MenuItem("Hide Unselected", nullptr, false, any)) {
            command("hide unselected");
        }
        if (ImGui::MenuItem("Show All Actors")) {
            command("unhide");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Select All", "Ctrl+A")) {
            command("selectall");
        }
        if (ImGui::MenuItem("Select None")) {
            command("selectnone");
        }
        if (ImGui::MenuItem("Invert Selection")) {
            command("selectinvert");
        }
        if (ImGui::BeginMenu("Select All of Class")) {
            std::map<std::string, dx::UClass*> classes;
            for (dx::UObject* actor : actors) {
                classes[actor->cls->name.str()] = actor->cls;
            }
            for (const auto& [name, cls] : classes) {
                if (ImGui::MenuItem(name.c_str())) {
                    command("selectclass " + cls->pathName());
                }
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, any)) {
            command("duplicate");
        }
        if (ImGui::MenuItem("Delete", "Del", false, any)) {
            command("delete");
        }
        ImGui::EndMenu();
    }

    void Editor::selectionViewItems()
    {
        ImGui::Separator();
        if (ImGui::BeginMenu("Z Clip Top View")) {
            ImGui::Checkbox("Enabled", &zClip);
            ImGui::InputFloat2("Z range", zClipRange, "%.0f");
            if (ImGui::MenuItem("Set from builder brush")) {
                command("zclip brush");
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(withIcon(icons::Move, "Perspective Gizmo").c_str())) {
            if (ImGui::MenuItem("Off", nullptr, gizmoMode == 0)) {
                gizmoMode = 0;
            }
            if (ImGui::MenuItem(withIcon(icons::Move, "Move").c_str(), "G", gizmoMode == 1)) {
                gizmoMode = 1;
            }
            if (ImGui::MenuItem(withIcon(icons::Rotate, "Rotate").c_str(), "R", gizmoMode == 2)) {
                gizmoMode = 2;
            }
            if (ImGui::MenuItem(withIcon(icons::Scale, "Scale").c_str(), "T", gizmoMode == 3)) {
                gizmoMode = 3;
            }
            if (ImGui::MenuItem(withIcon(icons::Extrude, "Extrude").c_str(), "X", gizmoMode == 4, editMode == 3)) {
                gizmoMode = 4;
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Show All Actors")) {
            command("unhide");
        }
    }

    void Editor::t3dFileItems()
    {
        ImGui::Separator();
        if (ImGui::BeginMenu("Import T3D")) {
            ImGui::InputText("File", t3dFile.data(), t3dFile.size());
            if (ImGui::Button("Import") && t3dFile[0]) {
                pendingCommand = std::string("importt3d ") + t3dFile.data();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Export T3D")) {
            ImGui::InputText("File", t3dFile.data(), t3dFile.size());
            if (ImGui::Button("Export Level") && t3dFile[0]) {
                command(std::string("exportt3d ") + t3dFile.data());
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Export Selected") && t3dFile[0]) {
                command(std::string("exportt3d ") + t3dFile.data() + " selected");
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndMenu();
        }
    }
}
