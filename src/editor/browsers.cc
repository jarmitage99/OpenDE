#include "editor/editor.h"

namespace dx::editor {
    void Editor::actorBrowser()
    {
        ImGui::InputText("Filter", filter.data(), filter.size());
        std::string needle = lower(filter.data());
        std::map<std::string, std::vector<dx::UObject*>> groups;
        for (dx::UObject* actor : actors) {
            std::string className = actor->cls->name.str();
            if (!needle.empty() && lower(className).find(needle) == std::string::npos && lower(actor->name.str()).find(needle) == std::string::npos) {
                continue;
            }
            groups[className].push_back(actor);
        }
        ImGui::BeginChild("actors");
        for (auto& group : groups) {
            std::string header = group.first + " (" + std::to_string(group.second.size()) + ")";
            bool containsSelection = selected && std::find(group.second.begin(), group.second.end(), selected) != group.second.end();
            if (containsSelection && selectionChanged) {
                ImGui::SetNextItemOpen(true);
            }
            if (!ImGui::TreeNode(group.first.c_str(), "%s", header.c_str())) {
                continue;
            }
            for (dx::UObject* actor : group.second) {
                bool isPrimary = actor == selected;
                if (ImGui::Selectable(actor->name.str().c_str(), isSelected(actor))) {
                    if (ImGui::GetIO().KeyCtrl) {
                        toggleSelect(actor);
                    } else {
                        select(actor);
                        focusSelected();
                    }
                }
                if (isPrimary && selectionChanged) {
                    ImGui::SetScrollHereY();
                }
            }
            ImGui::TreePop();
        }
        ImGui::EndChild();
    }

    void Editor::classNode(dx::UClass* cls)
    {
        auto found = classChildren.find(cls);
        bool leaf = found == classChildren.end();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (leaf) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (cls == actorClass) {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }
        if (cls == placeClass) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        bool open = ImGui::TreeNodeEx(cls, flags, "%s", cls->name.str().c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            placeClass = cls;
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && cls->scriptText) {
            command("editscript " + cls->pathName());
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            placeClass = cls;
            classMenuTarget = cls;
            ImGui::OpenPopup(classMenuId);
        }
        if (open && !leaf) {
            for (dx::UClass* child : found->second) {
                classNode(child);
            }
            ImGui::TreePop();
        }
    }

    void Editor::classBrowser()
    {
        ImGui::Checkbox("All classes", &showAllClasses);
        if (placeClass && actorClass && placeClass->isChildOf(actorClass)) {
            std::string label = "Add " + placeClass->name.str() + " here";
            if (ImGui::Button(label.c_str())) {
                command("add " + placeClass->pathName());
            }
            ImGui::SameLine();
            if (ImGui::Button("Show")) {
                std::snprintf(filter.data(), filter.size(), "%s", placeClass->name.str().c_str());
                pendingActorsTab = true;
            }
        }
        classActions();
        ImGui::BeginChild("classes");
        dx::UClass* root = showAllClasses ? dx::objects::objectClass() : actorClass;
        if (root) {
            classNode(root);
        }
        ImGui::EndChild();
    }

    void Editor::newClassPopup()
    {
        if (!ImGui::BeginPopup("New Class")) {
            return;
        }
        ImGui::Text("Parent: %s", placeClass->pathName().c_str());
        ImGui::InputText("Package", newPackage.data(), newPackage.size());
        ImGui::InputText("Name", newClassName.data(), newClassName.size());
        if (ImGui::Button("Create") && newPackage[0] && newClassName[0]) {
            pendingCommand = std::string("newclass ") + newPackage.data() + " " + newClassName.data() + " " + placeClass->pathName();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    void Editor::loadTexturePackage(const std::string& packageName)
    {
        dx::Linker* linker = dx::objects::loadPackage(dx::Name(packageName));
        textureList.clear();
        if (!linker) {
            return;
        }
        texturePackage = packageName;
        size_t count = linker->file().exports().size();
        for (size_t i = 0; i < count; i++) {
            dx::UObject* object = linker->object(static_cast<int32_t>(i + 1));
            if (object && object->cls && object->isA(dx::Name("Texture"))) {
                textureList.push_back(object);
            }
        }
        std::sort(textureList.begin(), textureList.end(), [](dx::UObject* a, dx::UObject* b) {
            return a->name.str() < b->name.str();
        });
        textureGroupNames = dx::editorops::textureGroups(textureList);
        textureGroupChoice = -1;
    }

    void Editor::textureBrowser()
    {
        if (ImGui::BeginCombo("Package", texturePackage.empty() ? "(choose)" : texturePackage.c_str())) {
            for (const std::string& name : texturePackages) {
                if (ImGui::Selectable(name.c_str(), name == texturePackage)) {
                    loadTexturePackage(name);
                }
            }
            ImGui::EndCombo();
        }
        textureContentTools();
        std::string groupLabel = textureGroupChoice < 0 ? std::string("(all)") : textureGroupNames[static_cast<size_t>(textureGroupChoice)].empty() ? std::string("(none)")
                                                                                                                                                    : textureGroupNames[static_cast<size_t>(textureGroupChoice)];
        if (ImGui::BeginCombo("Group", groupLabel.c_str())) {
            if (ImGui::Selectable("(all)", textureGroupChoice < 0)) {
                textureGroupChoice = -1;
            }
            for (size_t i = 0; i < textureGroupNames.size(); i++) {
                if (ImGui::Selectable(textureGroupNames[i].empty() ? "(none)" : textureGroupNames[i].c_str(), textureGroupChoice == static_cast<int>(i))) {
                    textureGroupChoice = static_cast<int>(i);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Checkbox("Surface mode", &surfaceMode);
        ImGui::Text("%zu surfaces selected", selectedSurfs.size());
        if (currentTexture) {
            ImGui::Text("Texture: %s", currentTexture->name.str().c_str());
            if (ImGui::Button("Apply to selected surfaces")) {
                command("applytexture " + currentTexture->pathName());
            }
            if (ImGui::Button("Properties...")) {
                command("texprops " + currentTexture->pathName());
            }
            ImGui::SameLine();
            if (ImGui::Button("Select surfaces using it")) {
                command("texusers");
            }
        }
        if (!selectedSurfs.empty() && ImGui::Button("Find texture of selected surface")) {
            command("findtexture");
        }
        ImGui::SliderInt("Thumbnail", &thumbnailSize, 32, 128);
        ImGui::BeginChild("textures");
        float width = ImGui::GetContentRegionAvail().x;
        int columns = std::max(1, static_cast<int>(width / (static_cast<float>(thumbnailSize) + 8.0f)));
        int column = 0;
        for (dx::UObject* texture : textureList) {
            if (textureGroupChoice >= 0 && dx::editorops::textureGroup(texture) != textureGroupNames[static_cast<size_t>(textureGroupChoice)]) {
                continue;
            }
            dx::UTexture* image = dx::cast<dx::UTexture>(texture);
            const dx::GLTextureInfo* info = image ? thumbnails.get(image) : nullptr;
            ImGui::PushID(texture);
            ImGui::BeginGroup();
            bool chosen = texture == currentTexture;
            if (chosen) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.7f, 0.1f, 1.0f));
            }
            float size = static_cast<float>(thumbnailSize);
            bool clicked = info ? ImGui::ImageButton("thumb", static_cast<ImTextureID>(info->id), ImVec2(size, size)) : ImGui::Button("?", ImVec2(size, size));
            if (chosen) {
                ImGui::PopStyleColor();
            }
            if (clicked) {
                currentTexture = texture;
                dx::bsp::setCurrentTexture(texture);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                command("texprops " + texture->pathName());
            }
            if (chosen && scrollToTexture) {
                ImGui::SetScrollHereY();
                scrollToTexture = false;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s %ux%u", texture->name.str().c_str(), info ? info->width : 0u, info ? info->height : 0u);
            }
            ImGui::EndGroup();
            ImGui::PopID();
            column++;
            if (column < columns) {
                ImGui::SameLine();
            } else {
                column = 0;
            }
        }
        ImGui::EndChild();
    }
}
