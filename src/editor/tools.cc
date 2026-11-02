#include "editor/editor.h"

namespace dx::editor {
    namespace {
        const char* const FindLabels[4] = { "Name", "Class", "Tag", "Event" };
    }

    void Editor::objectPropertyTable(dx::UObject* object)
    {
        std::map<std::string, std::vector<dx::UProperty*>> categories;
        for (dx::UProperty* property : object->cls->properties) {
            if (property->propertyFlags & dx::CPF_Edit) {
                categories[property->category.none() ? std::string("None") : property->category.str()].push_back(property);
            }
        }
        ImGui::PushID(object);
        for (auto& category : categories) {
            if (!ImGui::TreeNodeEx(category.first.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                continue;
            }
            if (ImGui::BeginTable(category.first.c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 160.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                for (dx::UProperty* property : category.second) {
                    for (int32_t index = 0; index < property->arrayDim; index++) {
                        objectPropertyRow(object, property, index, true);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    void Editor::runMapCheck()
    {
        checkResults = dx::mapcheck::check(level);
        size_t errors = 0;
        for (const dx::mapcheck::Issue& issue : checkResults) {
            if (issue.severity == dx::mapcheck::Severity::Error) {
                errors++;
            }
            std::fprintf(stderr, "MapCheck: %s: %s: %s\n", dx::mapcheck::severityName(issue.severity), issue.actor ? issue.actor->name.str().c_str() : currentMap.c_str(), issue.message.c_str());
        }
        status = "map check: " + std::to_string(errors) + " errors, " + std::to_string(checkResults.size() - errors) + " warnings";
        std::fprintf(stderr, "%s\n", status.c_str());
        showResults = true;
    }

    void Editor::resultsWindow()
    {
        if (!showResults) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(640, 280), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(360, 560), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Results", &showResults)) {
            ImGui::End();
            return;
        }
        if (ImGui::Button("Validate Level")) {
            runMapCheck();
        }
        ImGui::SameLine();
        ImGui::Text("%zu problems", checkResults.size());
        if (ImGui::BeginTable("results", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Actor", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < checkResults.size(); i++) {
                const dx::mapcheck::Issue& issue = checkResults[i];
                bool error = issue.severity == dx::mapcheck::Severity::Error;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(static_cast<int>(i));
                ImGui::PushStyleColor(ImGuiCol_Text, error ? ImVec4(1.0f, 0.45f, 0.4f, 1.0f) : ImVec4(1.0f, 0.85f, 0.4f, 1.0f));
                if (ImGui::Selectable(dx::mapcheck::severityName(issue.severity), issue.actor && issue.actor == selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    command("result " + std::to_string(i));
                }
                ImGui::PopStyleColor();
                ImGui::PopID();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(issue.actor ? issue.actor->name.str().c_str() : "Level");
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(issue.message.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }

    void Editor::logWindow()
    {
        if (!showLog) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(640, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(360, 120), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Log", &showLog)) {
            ImGui::End();
            return;
        }
        if (ImGui::Button("Clear")) {
            log.clear();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Scroll to end", &logScrollToEnd);
        std::vector<std::string> lines = log.lines();
        float footer = ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("lines", ImVec2(0, -footer), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(lines.size()));
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                ImGui::TextUnformatted(lines[static_cast<size_t>(i)].c_str());
            }
        }
        if (logScrollToEnd && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##command", "Editor command", logCommand.data(), logCommand.size(), ImGuiInputTextFlags_EnterReturnsTrue) && logCommand[0]) {
            std::string line = logCommand.data();
            log.add("> " + line);
            logCommand[0] = 0;
            bool done = command(line);
            log.add(done ? status : "failed: " + status);
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::End();
    }

    void Editor::findWindow()
    {
        if (!showFind) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(360, 420), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(400, 160), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Search for Actors", &showFind)) {
            ImGui::End();
            return;
        }
        for (size_t i = 0; i < findFields.size(); i++) {
            ImGui::InputText(FindLabels[i], findFields[i].data(), findFields[i].size());
        }
        findQuery = { findFields[0].data(), findFields[1].data(), findFields[2].data(), findFields[3].data() };
        if (ImGui::Button("Find Next")) {
            findNext();
        }
        std::vector<dx::UObject*> found = dx::actorsearch::find(actors, findQuery);
        ImGui::SameLine();
        ImGui::Text("%zu actors", found.size());
        ImGui::BeginChild("found", ImVec2(0, 0), ImGuiChildFlags_Borders);
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(found.size()));
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                dx::UObject* actor = found[static_cast<size_t>(i)];
                std::string label = actor->name.str() + "  (" + actor->cls->name.str() + ")";
                ImGui::PushID(actor);
                if (ImGui::Selectable(label.c_str(), actor == selected)) {
                    select(actor);
                    focusSelected();
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }

    bool Editor::findNext()
    {
        dx::UObject* actor = dx::actorsearch::findNext(actors, findQuery, selected);
        if (!actor) {
            status = "no matching actor";
            return false;
        }
        select(actor);
        focusSelected();
        status = "found " + actor->name.str();
        return true;
    }

    void Editor::levelPropertiesWindow()
    {
        if (!showLevelProperties) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(460, 560), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(420, 80), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Level Properties", &showLevelProperties)) {
            ImGui::End();
            return;
        }
        for (dx::UObject* object : dx::propedit::levelPropertyObjects(level)) {
            std::string header = object->cls->name.str() + "  " + object->name.str();
            if (ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                objectPropertyTable(object);
            }
        }
        ImGui::End();
    }

    void Editor::toolsWindows()
    {
        resultsWindow();
        logWindow();
        findWindow();
        levelPropertiesWindow();
        if (ImGui::GetIO().WantTextInput) {
            return;
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F)) {
            showFind = true;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F3, false)) {
            findNext();
        }
    }

    bool Editor::toolsCommand(const std::string& verb, const std::string& rest)
    {
        if (verb == "mapcheck") {
            runMapCheck();
            return true;
        }
        if (verb == "result") {
            size_t index = static_cast<size_t>(std::strtoul(rest.c_str(), nullptr, 10));
            if (index >= checkResults.size() || !checkResults[index].actor) {
                return false;
            }
            select(checkResults[index].actor);
            focusSelected();
            status = checkResults[index].actor->name.str() + ": " + checkResults[index].message;
            return true;
        }
        if (verb == "find") {
            std::istringstream input(rest);
            std::string item;
            for (std::array<char, 64>& field : findFields) {
                field[0] = 0;
            }
            while (input >> item) {
                size_t equals = item.find('=');
                std::string key = lower(item.substr(0, equals));
                std::string value = equals == std::string::npos ? std::string() : item.substr(equals + 1);
                for (size_t i = 0; i < findFields.size(); i++) {
                    if (key == lower(FindLabels[i])) {
                        std::snprintf(findFields[i].data(), findFields[i].size(), "%s", value.c_str());
                    }
                }
            }
            findQuery = { findFields[0].data(), findFields[1].data(), findFields[2].data(), findFields[3].data() };
            showFind = true;
            return findNext();
        }
        if (verb == "findnext") {
            return findNext();
        }
        if (verb == "levelprops") {
            showLevelProperties = rest != "close";
            return true;
        }
        if (verb == "setinfo") {
            std::istringstream input(rest);
            std::string propertyName;
            input >> propertyName;
            std::string value;
            std::getline(input, value);
            size_t start = value.find_first_not_of(' ');
            value = start == std::string::npos ? std::string() : value.substr(start);
            for (dx::UObject* object : dx::propedit::levelPropertyObjects(level)) {
                dx::UProperty* property = object->cls->findProperty(dx::Name(propertyName));
                if (property) {
                    return setProperty(object, property, 0, value);
                }
            }
            status = "no level property " + propertyName;
            return false;
        }
        if (verb == "use") {
            dx::UProperty* property = selected ? selected->cls->findProperty(dx::Name(rest)) : nullptr;
            dx::UObject* current = property ? dx::propedit::pickCurrent(property, { currentTexture, placeClass, selected }) : nullptr;
            if (!current) {
                status = "nothing to use for " + rest;
                return false;
            }
            return setProperty(selected, property, 0, property->toText(reinterpret_cast<const uint8_t*>(&current)));
        }
        if (verb == "log") {
            if (rest == "clear") {
                log.clear();
            }
            showLog = rest != "close";
            return true;
        }
        return false;
    }
}
