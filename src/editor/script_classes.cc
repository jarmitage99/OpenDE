#include "compiler/exporter.h"
#include "editor/editor.h"

namespace dx::editor {
    void Editor::classActions()
    {
        if (placeClass) {
            ImGui::Text("%s (%s)", placeClass->pathName().c_str(), placeClass->scriptText ? "scripted" : "built-in");
            ImGui::BeginDisabled(placeClass->scriptText == nullptr);
            if (ImGui::Button("Edit Code")) {
                command("editscript " + placeClass->pathName());
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Defaults")) {
                command("classdefaults " + placeClass->pathName());
            }
            ImGui::SameLine();
            if (ImGui::Button("New...")) {
                openNewClass = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                pendingCommand = "deleteclass " + placeClass->pathName();
            }
        }
        if (ImGui::Button("Export")) {
            pendingCommand = "exportscripts changed";
        }
        ImGui::SameLine();
        if (ImGui::Button("Export All")) {
            pendingCommand = "exportscripts all";
        }
        ImGui::SameLine();
        if (ImGui::Button("Load...")) {
            openLoadClass = true;
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!placeClass || !placeClass->outer);
        if (ImGui::Button("Save") && placeClass && placeClass->outer) {
            pendingCommand = "savepackage " + placeClass->outer->name.str();
        }
        ImGui::EndDisabled();
        if (openNewClass && placeClass) {
            std::snprintf(newPackage.data(), newPackage.size(), "%s", placeClass->outer ? placeClass->outer->name.str().c_str() : "MyLevel");
            std::snprintf(newClassName.data(), newClassName.size(), "My%s", placeClass->name.str().c_str());
            ImGui::OpenPopup("New Class");
        }
        openNewClass = false;
        if (placeClass) {
            newClassPopup();
        }
        if (openLoadClass) {
            ImGui::OpenPopup("Load Class");
            openLoadClass = false;
        }
        if (ImGui::BeginPopup("Load Class")) {
            ImGui::TextUnformatted("Load and compile a class from a .uc file");
            ImGui::InputText("File", loadClassFile.data(), loadClassFile.size());
            if (ImGui::Button("Load") && loadClassFile[0]) {
                pendingCommand = std::string("loadclass ") + loadClassFile.data();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        classMenuId = ImGui::GetID("Class Menu");
        if (classMenuRequest) {
            ImGui::OpenPopup(classMenuId);
            classMenuRequest = false;
        }
        if (classMenuTarget && ImGui::BeginPopup("Class Menu")) {
            classContextMenu(classMenuTarget);
            ImGui::EndPopup();
        }
    }

    void Editor::classContextMenu(dx::UClass* cls)
    {
        std::string name = cls->name.str();
        if (ImGui::MenuItem(("Edit " + name + " Script").c_str(), nullptr, false, cls->scriptText != nullptr)) {
            command("editscript " + cls->pathName());
        }
        if (ImGui::MenuItem(("Default " + name + " Properties...").c_str())) {
            command("classdefaults " + cls->pathName());
        }
        if (ImGui::MenuItem(("Create new class below " + name).c_str(), nullptr, false, cls->scriptText != nullptr)) {
            placeClass = cls;
            openNewClass = true;
        }
        if (ImGui::MenuItem(("Export " + name + " to .uc").c_str(), nullptr, false, cls->scriptText != nullptr)) {
            pendingCommand = "exportclass " + cls->pathName();
        }
        if (cls->scriptText && !(cls->flags & dx::RF_Native)) {
            ImGui::Separator();
            if (ImGui::MenuItem(("Delete " + name).c_str())) {
                pendingCommand = "deleteclass " + cls->pathName();
            }
        }
    }

    void Editor::classDefaultsWindow()
    {
        if (!showClassDefaults || !defaultsClass || defaultsClass->destroyed || !defaultsClass->defaultObject) {
            return;
        }
        dx::UClass* cls = defaultsClass;
        ImGui::SetNextWindowSize(ImVec2(460, 560), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(360, 90), ImGuiCond_FirstUseEver);
        std::string title = "Default " + cls->name.str() + " Properties###ClassDefaults";
        if (!ImGui::Begin(title.c_str(), &showClassDefaults)) {
            ImGui::End();
            return;
        }
        ImGui::Text("Class %s", cls->pathName().c_str());
        dx::UClass* parent = cls->superClass();
        const uint8_t* inherited = parent && parent->defaultObject ? parent->defaultObject->data : nullptr;
        std::string before = dx::compiler::exportProperties(cls, cls->defaultObject->data, parent, inherited, 0);
        std::map<std::string, std::vector<dx::UProperty*>> categories;
        for (dx::UProperty* property : cls->properties) {
            if (property->propertyFlags & dx::CPF_Edit) {
                categories[property->category.none() ? std::string("None") : property->category.str()].push_back(property);
            }
        }
        ImGui::BeginChild("defaults");
        for (auto& category : categories) {
            if (!ImGui::CollapsingHeader(category.first.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                continue;
            }
            if (!ImGui::BeginTable(category.first.c_str(), 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) {
                continue;
            }
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            for (dx::UProperty* property : category.second) {
                for (int32_t index = 0; index < property->arrayDim; index++) {
                    objectPropertyRow(cls->defaultObject, property, index, false);
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        if (dx::compiler::exportProperties(cls, cls->defaultObject->data, parent, inherited, 0) != before) {
            scripts.markChanged(cls);
            status = "changed default properties of " + cls->name.str();
        }
        ImGui::End();
    }

    bool Editor::classCommand(const std::string& verb, const std::string& rest)
    {
        if (verb == "loadclass") {
            dx::UClass* cls = scripts.loadClass(rest);
            showScripts = true;
            status = scripts.messages().empty() ? std::string() : scripts.messages().back().text;
            if (!cls) {
                for (const dx::editor::CompileMessage& message : scripts.messages()) {
                    if (message.error && message.cls) {
                        std::ifstream in(rest, std::ios::binary);
                        std::stringstream text;
                        text << in.rdbuf();
                        loadDocumentText(message.cls, message.line, text.str());
                        break;
                    }
                }
                return false;
            }
            refreshClasses();
            placeClass = cls;
            openDocument(cls, 0);
            return true;
        }
        if (verb == "setdefault") {
            std::istringstream fields(rest);
            std::string className;
            std::string propertyName;
            fields >> className >> propertyName;
            std::string value;
            std::getline(fields, value);
            size_t start = value.find_first_not_of(' ');
            value = start == std::string::npos ? std::string() : value.substr(start);
            bool done = scripts.setDefault(dx::objects::findClass(className), propertyName, value);
            status = scripts.messages().back().text;
            return done;
        }
        if (verb == "classmenu") {
            classMenuTarget = dx::objects::findClass(rest);
            classMenuRequest = classMenuTarget != nullptr;
            if (classMenuTarget) {
                placeClass = classMenuTarget;
            }
            pendingTab = "classes";
            return classMenuRequest;
        }
        dx::UClass* cls = rest == "close" ? nullptr : dx::objects::findClass(rest);
        if (verb == "classdefaults") {
            defaultsClass = cls;
            showClassDefaults = cls != nullptr;
            if (cls && !cls->defaultObject) {
                status = cls->name.str() + " has no default properties";
                showClassDefaults = false;
            }
            return showClassDefaults || rest == "close";
        }
        if (verb == "exportclass") {
            bool done = scripts.exportClass(cls);
            showScripts = true;
            status = done ? "exported " + cls->name.str() + ".uc" : scripts.messages().back().text;
            return done;
        }
        if (verb == "deleteclass") {
            bool done = scripts.deleteClass(cls);
            showScripts = true;
            status = scripts.messages().back().text;
            if (!done) {
                return false;
            }
            for (size_t i = documents.size(); i-- > 0;) {
                if (documents[i].cls == cls) {
                    documents.erase(documents.begin() + static_cast<std::ptrdiff_t>(i));
                }
            }
            if (placeClass == cls) {
                placeClass = cls->superClass();
            }
            if (defaultsClass == cls) {
                defaultsClass = nullptr;
            }
            if (classMenuTarget == cls) {
                classMenuTarget = nullptr;
            }
            buildClassTree();
            return true;
        }
        return false;
    }
}
