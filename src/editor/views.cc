#include "editor/editor.h"

namespace dx::editor {
    namespace {
        const char* paneKindNames[4] = { "Top", "Front", "Side", "Perspective" };
        const char* paneKindKeys[4] = { "top", "front", "side", "perspective" };
    }

    void Editor::swapPaneCamera(ViewPane& pane)
    {
        std::swap(framebuffer, pane.framebuffer);
        std::swap(colorTexture, pane.colorTexture);
        std::swap(depthBuffer, pane.depthBuffer);
        std::swap(fbWidth, pane.fbWidth);
        std::swap(fbHeight, pane.fbHeight);
        if (!pane.shared) {
            std::swap(cameraLocation, pane.location);
            std::swap(cameraRotation, pane.rotation);
        }
    }

    void Editor::viewLayout(const ImVec2& position, const ImVec2& size)
    {
        for (size_t i = 0; i < panes.size(); i++) {
            viewRects.erase(paneKindKeys[i]);
            viewRects.erase("pane" + std::to_string(i));
        }
        std::array<dx::editorviews::PaneRect, 4> rects = dx::editorviews::paneRects(position.x, position.y, size.x, size.y, maximizedPane);
        for (size_t i = 0; i < panes.size(); i++) {
            const dx::editorviews::PaneRect& rect = rects[i];
            if (rect.width <= 0 || rect.height <= 0) {
                continue;
            }
            ViewPane& pane = panes[i];
            size_t kind = static_cast<size_t>(pane.kind);
            currentPane = i;
            paneTitle = std::string(paneKindNames[kind]) + "##pane" + std::to_string(i);
            paneKey = pane.shared ? paneKindKeys[kind] : "pane" + std::to_string(i);
            ImVec2 panePosition(rect.x, rect.y);
            ImVec2 paneSize(rect.width, rect.height);
            if (pane.kind == dx::editorviews::PaneKind::Perspective) {
                ImVec2 savedOrigin = perspectiveOrigin;
                ImVec2 savedSize = perspectiveSize;
                swapPaneCamera(pane);
                perspectiveMode = pane.mode;
                perspectiveWindow(panePosition, paneSize);
                swapPaneCamera(pane);
                if (!pane.shared) {
                    perspectiveOrigin = savedOrigin;
                    perspectiveSize = savedSize;
                }
            } else {
                orthoWindow(pane.shared ? orthoViews[kind] : pane.ortho, panePosition, paneSize);
            }
            auto found = viewRects.find(paneKey);
            if (found != viewRects.end()) {
                viewRects["pane" + std::to_string(i)] = found->second;
            }
        }
    }

    void Editor::paneMenu(size_t index)
    {
        if (!ImGui::BeginMenuBar()) {
            return;
        }
        ViewPane& pane = panes[index];
        if (ImGui::BeginMenu(withIcon(icons::Camera, "View").c_str())) {
            for (size_t kind = 0; kind < 4; kind++) {
                if (ImGui::MenuItem(paneKindNames[kind], nullptr, static_cast<size_t>(pane.kind) == kind)) {
                    setPaneKind(index, static_cast<dx::editorviews::PaneKind>(kind));
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Mode", pane.kind == dx::editorviews::PaneKind::Perspective)) {
            for (int mode = 0; mode < dx::RenderModeCount; mode++) {
                if (ImGui::MenuItem(dx::renderModeName(static_cast<dx::RenderMode>(mode)), nullptr, static_cast<int>(pane.mode) == mode)) {
                    pane.mode = static_cast<dx::RenderMode>(mode);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(withIcon(icons::Show, "Show").c_str())) {
            ImGui::MenuItem("Links", nullptr, &showPaths);
            ImGui::MenuItem("Actor Icons", nullptr, &showActorIcons);
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(maximizedPane == static_cast<int>(index) ? withIcon(icons::Restore, "Restore").c_str() : withIcon(icons::Maximize, "Maximize").c_str())) {
            maximizedPane = maximizedPane == static_cast<int>(index) ? -1 : static_cast<int>(index);
        }
        ImGui::EndMenuBar();
    }

    void Editor::setPaneKind(size_t index, dx::editorviews::PaneKind kind)
    {
        ViewPane& pane = panes[index];
        pane.kind = kind;
        pane.shared = static_cast<size_t>(kind) == index;
        if (pane.shared) {
            return;
        }
        if (kind == dx::editorviews::PaneKind::Perspective) {
            pane.location = cameraLocation;
            pane.rotation = cameraRotation;
        } else {
            pane.ortho = orthoViews[static_cast<size_t>(kind)];
        }
    }

    void Editor::syncPanes()
    {
        for (size_t i = 0; i < panes.size(); i++) {
            if (!panes[i].shared) {
                setPaneKind(i, panes[i].kind);
            }
        }
    }

    void Editor::releasePanes()
    {
        for (ViewPane& pane : panes) {
            swapPaneCamera(pane);
            releaseFramebuffer();
            swapPaneCamera(pane);
        }
    }

    bool Editor::viewCommand(const std::string& verb, const std::string& rest)
    {
        std::istringstream values(rest);
        if (verb == "maximize") {
            int index = -1;
            values >> index;
            maximizedPane = index >= 0 && index < 4 ? index : -1;
            return true;
        }
        if (verb == "showpaths" || verb == "showicons") {
            int enabled = 1;
            values >> enabled;
            if (verb == "showpaths") {
                showPaths = enabled != 0;
            } else {
                showActorIcons = enabled != 0;
            }
            return true;
        }
        size_t index = 4;
        std::string name;
        values >> index >> name;
        if (index >= panes.size()) {
            status = verb + " needs a pane 0-3";
            return false;
        }
        if (verb == "viewkind") {
            for (size_t kind = 0; kind < 4; kind++) {
                if (lower(name) == paneKindKeys[kind]) {
                    setPaneKind(index, static_cast<dx::editorviews::PaneKind>(kind));
                    return true;
                }
            }
            status = "unknown view " + name;
            return false;
        }
        if (!dx::renderModeNamed(lower(name), panes[index].mode)) {
            status = "unknown render mode " + name;
            return false;
        }
        return true;
    }

    void Editor::drawOrthoPaths(ImDrawList* list, const OrthoView& view, const ImVec2& origin, const ImVec2& size)
    {
        if (!showPaths || !level) {
            return;
        }
        for (const dx::editorviews::PathLink& link : dx::editorviews::pathLinks(*level)) {
            list->AddLine(orthoToScreen(view, link.start, origin, size), orthoToScreen(view, link.end, origin, size), link.monster ? IM_COL32(40, 90, 255, 255) : IM_COL32(200, 30, 30, 255));
        }
    }

    void Editor::drawPerspectivePaths(ImDrawList* list, const ImVec2& origin, const ImVec2& size)
    {
        if (!showPaths || !level) {
            return;
        }
        for (const dx::editorviews::PathLink& link : dx::editorviews::pathLinks(*level)) {
            drawPerspectiveLine(list, link.start, link.end, origin, size, link.monster ? IM_COL32(40, 90, 255, 255) : IM_COL32(200, 30, 30, 255));
        }
    }

    bool Editor::drawActorIcon(ImDrawList* list, dx::UObject* actor, const ImVec2& point, float zoom)
    {
        if (!showActorIcons) {
            return false;
        }
        dx::UTexture* icon = dx::editorviews::actorIcon(actor);
        const dx::GLTextureInfo* info = icon ? thumbnails.getMasked(icon) : nullptr;
        if (!info) {
            return false;
        }
        float half = dx::editorviews::iconSize(static_cast<float>(icon->width()), dx::actor::DrawScale(actor), zoom) * 0.5f;
        list->AddImage(static_cast<ImTextureID>(info->id), ImVec2(point.x - half, point.y - half), ImVec2(point.x + half, point.y + half));
        if (actor == selected) {
            list->AddRect(ImVec2(point.x - half - 1, point.y - half - 1), ImVec2(point.x + half + 1, point.y + half + 1), IM_COL32(255, 255, 0, 255));
        }
        return true;
    }
}
