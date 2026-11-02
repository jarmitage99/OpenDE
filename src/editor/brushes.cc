#include "editor/editor.h"

namespace dx::editor {
    void Editor::builderPanel()
    {
        std::vector<dx::UClass*> classes = dx::brushbuilder::builderClasses();
        if (extraBuilderList.empty()) {
            extraBuilderList = dx::brushbuilder::extraBuilders();
        }
        std::string current = "(none)";
        if (builderChoice < classes.size()) {
            current = classes[builderChoice]->name.str();
        } else if (builderChoice - classes.size() < extraBuilderList.size()) {
            current = extraBuilderList[builderChoice - classes.size()].name;
        }
        if (ImGui::BeginCombo("Builder", current.c_str())) {
            for (size_t i = 0; i < classes.size(); i++) {
                if (ImGui::Selectable(classes[i]->name.str().c_str(), i == builderChoice)) {
                    builderChoice = i;
                }
            }
            for (size_t i = 0; i < extraBuilderList.size(); i++) {
                if (ImGui::Selectable(extraBuilderList[i].name.c_str(), classes.size() + i == builderChoice)) {
                    builderChoice = classes.size() + i;
                }
            }
            ImGui::EndCombo();
        }
        if (builderChoice >= classes.size() && builderChoice - classes.size() < extraBuilderList.size()) {
            dx::brushbuilder::ExtraBuilder& extra = extraBuilderList[builderChoice - classes.size()];
            for (dx::brushbuilder::ExtraParameter& parameter : extra.parameters) {
                if (parameter.boolean) {
                    bool value = parameter.value != 0.0;
                    if (ImGui::Checkbox(parameter.name.c_str(), &value)) {
                        parameter.value = value ? 1.0 : 0.0;
                    }
                } else if (parameter.integer) {
                    int value = static_cast<int>(parameter.value);
                    if (ImGui::InputInt(parameter.name.c_str(), &value)) {
                        parameter.value = value;
                    }
                } else {
                    ImGui::InputDouble(parameter.name.c_str(), &parameter.value);
                }
            }
            if (ImGui::Button("Build brush")) {
                std::string line = "brushbuild " + extra.name;
                for (const dx::brushbuilder::ExtraParameter& parameter : extra.parameters) {
                    line += " " + parameter.name + "=" + std::to_string(parameter.value);
                }
                command(line);
            }
        }
        if (builderChoice < classes.size()) {
            dx::UObject* object = dx::brushbuilder::builderFor(classes[builderChoice]);
            if (object && ImGui::BeginTable("builderProperties", 2, ImGuiTableFlags_BordersInnerV)) {
                for (dx::UProperty* property : classes[builderChoice]->properties) {
                    if (!(property->propertyFlags & dx::CPF_Edit)) {
                        continue;
                    }
                    objectPropertyRow(object, property, 0, false);
                }
                ImGui::EndTable();
            }
            if (ImGui::Button("Build brush")) {
                command("brushbuild " + classes[builderChoice]->name.str());
            }
        }
        ImGui::Separator();
        if (ImGui::Button("Add")) {
            command("csgadd");
        }
        ImGui::SameLine();
        if (ImGui::Button("Subtract")) {
            command("csgsubtract");
        }
        ImGui::SameLine();
        if (ImGui::Button("Intersect")) {
            command("intersect");
        }
        ImGui::SameLine();
        if (ImGui::Button("Deintersect")) {
            command("deintersect");
        }
        if (ImGui::BeginCombo("Special", specialPresets[specialChoice].name.c_str())) {
            for (size_t i = 0; i < specialPresets.size(); i++) {
                if (ImGui::Selectable(specialPresets[i].name.c_str(), i == specialChoice)) {
                    specialChoice = i;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Add special")) {
            command("addspecial " + specialPresets[specialChoice].name);
        }
        ImGui::SameLine();
        if (ImGui::Button("Custom...")) {
            command("addspecialdialog 1");
        }
        if (ImGui::Button("2D Shape Editor")) {
            command("shapeeditor 1");
        }
        ImGui::SameLine();
        if (ImGui::Button("Height Map")) {
            command("heightmapdialog 1");
        }
        ImGui::SameLine();
        if (ImGui::Button("Floor Lofter")) {
            command("floorlofter 1");
        }
        std::vector<dx::UClass*> movers = moverClasses();
        std::string moverName = moverChoice < movers.size() ? movers[moverChoice]->name.str() : std::string("Mover");
        if (ImGui::BeginCombo("Mover class", moverName.c_str())) {
            for (size_t i = 0; i < movers.size(); i++) {
                if (ImGui::Selectable(movers[i]->name.str().c_str(), i == moverChoice)) {
                    moverChoice = i;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Add mover")) {
            command("addmover " + (moverChoice < movers.size() ? movers[moverChoice]->pathName() : std::string()));
        }
        ImGui::Separator();
        if (ImGui::Button("Rebuild...")) {
            showRebuild = true;
        }
    }

    std::vector<dx::UClass*> Editor::moverClasses()
    {
        std::vector<dx::UClass*> result;
        dx::UClass* base = dx::objects::findClass("Engine.Mover");
        if (!base) {
            return result;
        }
        std::vector<dx::UClass*> pending { base };
        while (!pending.empty()) {
            dx::UClass* cls = pending.back();
            pending.pop_back();
            if (!(cls->classFlags & dx::CLASS_Abstract)) {
                result.push_back(cls);
            }
            auto found = classChildren.find(cls);
            if (found != classChildren.end()) {
                pending.insert(pending.end(), found->second.begin(), found->second.end());
            }
        }
        std::sort(result.begin(), result.end(), [](dx::UClass* a, dx::UClass* b) {
            return a->name.str() < b->name.str();
        });
        return result;
    }

    bool Editor::runBrushBuilder(const std::string& line)
    {
        std::istringstream input(line);
        std::string name;
        input >> name;
        dx::UObject* builder = builderBrush();
        if (!builder) {
            status = "no builder brush";
            return false;
        }
        std::vector<std::pair<std::string, std::string>> values;
        std::string item;
        while (input >> item) {
            size_t equals = item.find('=');
            if (equals != std::string::npos) {
                values.emplace_back(item.substr(0, equals), item.substr(equals + 1));
            }
        }
        for (dx::UClass* cls : dx::brushbuilder::builderClasses()) {
            if (lower(cls->name.str()) != lower(name)) {
                continue;
            }
            dx::UObject* object = dx::brushbuilder::builderFor(cls);
            for (const auto& [key, value] : values) {
                dx::UProperty* property = cls->findProperty(dx::Name(key));
                if (property) {
                    property->fromText(object->data + property->offset, value);
                }
            }
            dx::brushbuilder::setTarget(builder);
            history.begin(level, { builder, dx::actor::Brush(builder), dx::cast<dx::UModel>(dx::actor::Brush(builder))->polys }, "Build " + cls->name.str());
            bool done = dx::brushbuilder::build(object);
            if (done) {
                history.commit();
            } else {
                history.cancel();
            }
            brushCache.erase(builder);
            status = done ? "built " + cls->name.str() : dx::brushbuilder::lastMessage();
            return done;
        }
        for (dx::brushbuilder::ExtraBuilder extra : dx::brushbuilder::extraBuilders()) {
            if (lower(extra.name) != lower(name)) {
                continue;
            }
            for (const auto& [key, value] : values) {
                for (dx::brushbuilder::ExtraParameter& parameter : extra.parameters) {
                    if (lower(parameter.name) == lower(key)) {
                        parameter.value = std::strtod(value.c_str(), nullptr);
                    }
                }
            }
            dx::UModel* brush = dx::cast<dx::UModel>(dx::actor::Brush(builder));
            history.begin(level, { builder, brush, brush->polys }, "Build " + extra.name);
            std::string error;
            bool done = dx::brushbuilder::buildExtra(extra, builder, &error);
            if (done) {
                history.commit();
            } else {
                history.cancel();
            }
            brushCache.erase(builder);
            status = done ? "built " + extra.name : error;
            return done;
        }
        status = "no brush builder " + name;
        return false;
    }
}
