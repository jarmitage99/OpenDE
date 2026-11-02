#include "editor/editor.h"

namespace dx::editor {
    void Editor::rebuildWindow()
    {
        if (!showRebuild) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_Appearing);
        if (!ImGui::Begin("Rebuild", &showRebuild)) {
            ImGui::End();
            return;
        }
        ImGui::TextUnformatted("Geometry");
        ImGui::Checkbox("Auto BSP", &rebuildOptions.bsp);
        if (ImGui::Button("Rebuild geometry")) {
            command("rebuild");
        }
        ImGui::Separator();
        ImGui::TextUnformatted("BSP");
        int optimization = static_cast<int>(rebuildOptions.optimization);
        ImGui::RadioButton("Lame", &optimization, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Good", &optimization, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Optimal", &optimization, 2);
        rebuildOptions.optimization = static_cast<dx::bsp::Optimization>(optimization);
        ImGui::SliderInt("Balance", &rebuildOptions.balance, 0, 100);
        ImGui::Checkbox("Build visibility zones", &rebuildOptions.zones);
        ImGui::Checkbox("Optimize geometry", &rebuildOptions.optimizeGeometry);
        if (ImGui::Button("Rebuild BSP")) {
            command("rebuildbsp");
        }
        ImGui::Separator();
        ImGui::TextUnformatted("Lighting");
        ImGui::Checkbox("Selected lights only", &lightSelectedOnly);
        if (ImGui::Button("Rebuild lighting")) {
            command(lightSelectedOnly ? "light selected" : "light");
        }
        ImGui::Separator();
        ImGui::TextUnformatted("Paths");
        if (ImGui::Button("Define paths")) {
            command("paths define");
        }
        ImGui::SameLine();
        ImGui::RadioButton("Low", &pathOptimization, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Medium", &pathOptimization, 1);
        ImGui::SameLine();
        ImGui::RadioButton("High", &pathOptimization, 2);
        if (ImGui::Button("Build paths")) {
            command("paths build " + std::to_string(pathOptimization));
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove paths")) {
            command("paths remove");
        }
        ImGui::SameLine();
        if (ImGui::Button("Show")) {
            command("paths show");
        }
        ImGui::SameLine();
        if (ImGui::Button("Hide")) {
            command("paths hide");
        }
        ImGui::Separator();
        if (ImGui::Button("Rebuild all")) {
            command("rebuild");
            command(lightSelectedOnly ? "light selected" : "light");
            command("paths define");
        }
        ImGui::End();
    }

    bool Editor::pathsCommand(const std::string& line)
    {
        std::istringstream input(line);
        std::string action;
        int optimization = 1;
        input >> action >> optimization;
        restoreRegions(true);
        int count = 0;
        if (action == "define") {
            dx::paths::undefinePaths(level);
            dx::paths::definePaths(level);
            count = static_cast<int>(level->reachSpecs.size());
        } else if (action == "undefine") {
            dx::paths::undefinePaths(level);
        } else if (action == "build") {
            dx::paths::removePaths(level);
            count = dx::paths::buildPaths(level, optimization);
        } else if (action == "remove") {
            count = dx::paths::removePaths(level);
        } else if (action == "show") {
            count = dx::paths::showPaths(level);
        } else if (action == "hide") {
            count = dx::paths::hidePaths(level);
        } else {
            captureRegions();
            status = "unknown paths command " + action;
            return false;
        }
        captureRegions();
        refreshActors();
        status = "paths " + action + ": " + std::to_string(count);
        std::fprintf(stderr, "%s\n", status.c_str());
        return true;
    }
}
