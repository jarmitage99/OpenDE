#include "editor/editor.h"

namespace dx::editor {
    bool Editor::editableType(const dx::UProperty* property)
    {
        switch (property->type) {
        case dx::PropType::Byte:
        case dx::PropType::Int:
        case dx::PropType::Bool:
        case dx::PropType::Float:
        case dx::PropType::Name:
        case dx::PropType::Str:
        case dx::PropType::String:
        case dx::PropType::Struct:
        case dx::PropType::Object:
        case dx::PropType::Class:
            return true;
        case dx::PropType::Array:
        case dx::PropType::Map:
        case dx::PropType::FixedArray:
        case dx::PropType::Pointer:
            return false;
        }
        return false;
    }

    void Editor::transformPanel()
    {
        dx::Vector& location = dx::actor::Location(selected);
        dx::Rotator& rotation = dx::actor::Rotation(selected);
        float position[3] = { location.x, location.y, location.z };
        int angles[3] = { rotation.pitch, rotation.yaw, rotation.roll };
        if (ImGui::DragFloat3("Location", position, 4.0f)) {
            location = { position[0], position[1], position[2] };
            noteMoved(selected);
            brushCache.erase(selected);
        }
        trackEdit("Move " + selected->name.str());
        if (ImGui::DragInt3("Rotation", angles, 64.0f)) {
            rotation = { angles[0], angles[1], angles[2] };
            noteMoved(selected);
            brushCache.erase(selected);
        }
        trackEdit("Rotate " + selected->name.str());
    }

    void Editor::trackEdit(const std::string& label)
    {
        if (ImGui::IsItemActivated() && !history.recording()) {
            history.begin(level, { selected }, label);
        }
        if (ImGui::IsItemDeactivated() && history.recording()) {
            history.commit();
        }
    }

    void Editor::propertiesPanel()
    {
        if (!selected) {
            ImGui::TextUnformatted("No actor selected");
            return;
        }
        if (selectionList().size() > 1) {
            multiPropertiesPanel();
            return;
        }
        ImGui::Text("%s", selected->name.str().c_str());
        ImGui::Text("Class %s", selected->cls->pathName().c_str());
        transformPanel();
        ImGui::Checkbox("Editable only", &editableOnly);
        std::map<std::string, std::vector<dx::UProperty*>> categories;
        for (dx::UProperty* property : selected->cls->properties) {
            if (editableOnly && !(property->propertyFlags & dx::CPF_Edit)) {
                continue;
            }
            std::string category = property->category.none() ? std::string("None") : property->category.str();
            categories[category].push_back(property);
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
                    propertyRow(property, index);
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }

    bool Editor::setProperty(dx::UObject* actor, dx::UProperty* property, int32_t index, const std::string& value)
    {
        uint8_t* memory = actor->data + property->offset + static_cast<size_t>(index) * property->elementSize();
        history.begin(level, { actor }, "Set " + property->name.str());
        if (!property->fromText(memory, value)) {
            history.cancel();
            status = "cannot set " + property->name.str() + " to " + value;
            return false;
        }
        history.commit();
        brushCache.erase(actor);
        status = "set " + property->name.str();
        return true;
    }

    void Editor::propertyRow(dx::UProperty* property, int32_t index)
    {
        objectPropertyRow(selected, property, index, true);
    }

    void Editor::valueEditor(dx::UObject* object, dx::UProperty* property, uint8_t* memory, const void* key, bool tracked, const std::function<bool(const std::string&)>& apply)
    {
        ImGui::PushID(key);
        if (editingKey == key && editingObject == object) {
            ImGui::SetNextItemWidth(-1);
            if (focusEdit) {
                ImGui::SetKeyboardFocusHere();
                focusEdit = false;
            }
            if (ImGui::InputText("##edit", editBuffer.data(), editBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                apply(editBuffer.data());
                editingKey = nullptr;
            } else if (ImGui::IsItemDeactivated()) {
                editingKey = nullptr;
            }
        } else if (property->type == dx::PropType::Bool) {
            bool checked = *reinterpret_cast<uint32_t*>(memory) != 0;
            if (ImGui::Checkbox("##bool", &checked)) {
                apply(checked ? "True" : "False");
            }
        } else if (property->type == dx::PropType::Byte && property->enumType) {
            std::string value = property->toText(memory);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::BeginCombo("##enum", value.c_str())) {
                for (size_t i = 0; i < property->enumType->names.size(); i++) {
                    std::string name = property->enumType->names[i].str();
                    if (ImGui::Selectable(name.c_str(), *memory == i)) {
                        apply(name);
                    }
                }
                ImGui::EndCombo();
            }
        } else {
            std::string value = property->toText(memory);
            bool editable = editableType(property);
            if (dx::propedit::isColor(property)) {
                std::array<float, 4> color = dx::propedit::colorValue(property, memory);
                if (ImGui::ColorEdit4("##color", color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf)) {
                    std::vector<uint8_t> edited(property->structType->size);
                    property->structType->initValue(edited.data());
                    property->structType->copyValue(edited.data(), memory);
                    dx::propedit::setColor(property, edited.data(), color);
                    apply(property->toText(edited.data()));
                    property->structType->destroyValue(edited.data());
                }
                ImGui::SameLine();
            }
            if (property->type == dx::PropType::Object || property->type == dx::PropType::Class) {
                dx::UObject* current = dx::propedit::pickCurrent(property, { currentTexture, placeClass, selected });
                ImGui::BeginDisabled(current == nullptr);
                if (ImGui::SmallButton("Use")) {
                    apply(property->toText(reinterpret_cast<const uint8_t*>(&current)));
                }
                ImGui::EndDisabled();
                if (current && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::SetTooltip("Use %s", current->name.str().c_str());
                }
                ImGui::SameLine();
            }
            if (!editable) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            }
            if (ImGui::Selectable(value.empty() ? " " : value.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) && editable) {
                editingKey = key;
                editingObject = object;
                focusEdit = true;
                std::snprintf(editBuffer.data(), editBuffer.size(), "%s", value.c_str());
            }
            if (!editable) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::PopID();
        static_cast<void>(tracked);
    }

    void Editor::objectPropertyRow(dx::UObject* object, dx::UProperty* property, int32_t index, bool tracked)
    {
        uint8_t* memory = object->data + property->offset + static_cast<size_t>(index) * property->elementSize();
        std::string label = property->name.str();
        if (property->arrayDim > 1) {
            label += "[" + std::to_string(index) + "]";
        }
        valueRows(object, property, memory, label, tracked, [&](const std::string& text) {
            if (tracked) {
                return setProperty(object, property, index, text);
            }
            return property->fromText(memory, text);
        });
    }

    bool Editor::memberEdit(dx::UObject* object, const std::string& label, bool tracked, const std::function<bool()>& edit)
    {
        if (tracked) {
            history.begin(level, { object }, label);
        }
        bool done = edit();
        if (tracked) {
            if (done) {
                history.commit();
            } else {
                history.cancel();
            }
        }
        brushCache.erase(object);
        return done;
    }

    void Editor::valueRows(dx::UObject* object, dx::UProperty* property, uint8_t* memory, const std::string& label, bool tracked, const std::function<bool(const std::string&)>& apply)
    {
        ImGui::PushID(label.c_str());
        const void* key = reinterpret_cast<const void*>(static_cast<uintptr_t>(ImGui::GetID(memory)));
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        bool nested = property->type == dx::PropType::Struct || property->type == dx::PropType::Array || property->type == dx::PropType::FixedArray;
        bool open = false;
        if (nested) {
            open = ImGui::TreeNodeEx("##node", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", label.c_str());
        } else {
            ImGui::TextUnformatted(label.c_str());
        }
        ImGui::TableSetColumnIndex(1);
        std::string name = "Set " + property->name.str();
        if (property->type == dx::PropType::Array && property->inner) {
            dx::UProperty* inner = property->inner;
            size_t size = inner->elementSize();
            auto& elements = *reinterpret_cast<std::vector<uint8_t>*>(memory);
            size_t count = size ? elements.size() / size : 0;
            ImGui::Text("%zu elements", count);
            if (!inner->needsConstruct()) {
                ImGui::SameLine();
                if (ImGui::SmallButton("+")) {
                    memberEdit(object, "Add element " + property->name.str(), tracked, [&]() {
                        elements.resize(elements.size() + size, 0);
                        return true;
                    });
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("-") && count > 0) {
                    memberEdit(object, "Remove element " + property->name.str(), tracked, [&]() {
                        elements.resize(elements.size() - size);
                        return true;
                    });
                }
            }
            if (open) {
                count = size ? elements.size() / size : 0;
                for (size_t i = 0; i < count; i++) {
                    uint8_t* element = elements.data() + i * size;
                    valueRows(object, inner, element, "[" + std::to_string(i) + "]", tracked, [&, element](const std::string& text) {
                        return memberEdit(object, name, tracked, [&]() {
                            return inner->fromText(element, text);
                        });
                    });
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
            return;
        }
        if (property->type == dx::PropType::FixedArray && property->inner) {
            dx::UProperty* inner = property->inner;
            ImGui::Text("%d elements", property->fixedCount);
            if (open) {
                for (int32_t i = 0; i < property->fixedCount; i++) {
                    uint8_t* element = memory + static_cast<size_t>(i) * inner->elementSize();
                    valueRows(object, inner, element, "[" + std::to_string(i) + "]", tracked, [&, element](const std::string& text) {
                        return memberEdit(object, name, tracked, [&]() {
                            return inner->fromText(element, text);
                        });
                    });
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
            return;
        }
        valueEditor(object, property, memory, key, tracked, apply);
        if (open) {
            for (dx::UProperty* member : property->structType->properties) {
                for (int32_t i = 0; i < member->arrayDim; i++) {
                    uint8_t* field = memory + member->offset + static_cast<size_t>(i) * member->elementSize();
                    std::string memberLabel = member->name.str();
                    if (member->arrayDim > 1) {
                        memberLabel += "[" + std::to_string(i) + "]";
                    }
                    valueRows(object, member, field, memberLabel, tracked, [&, member, field](const std::string& text) {
                        return memberEdit(object, name, tracked, [&]() {
                            return member->fromText(field, text);
                        });
                    });
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}
