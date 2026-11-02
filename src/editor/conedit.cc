#include "editor/editor.h"

namespace dx::editor {
    namespace {
        using dx::compiler::ConChoice;
        using dx::compiler::ConConversation;
        using dx::compiler::ConEvent;
        using dx::compiler::ConEventType;
        using dx::compiler::ConFlagRef;

        const char* const CameraTypes[] = { "Predefined", "Speakers", "Actor", "Random" };
        const char* const CameraPositions[] = { "Side Tight", "Side Mid", "Side Above", "Side Above 45", "Shoulder Left", "Shoulder Right", "Head Shot Tight", "Head Shot Mid", "Head Shot Left", "Head Shot Right", "Head Shot Slight Right", "Head Shot Slight Left", "Straight Above Looking Down", "Straight Below Looking Up", "Below Looking Up" };
        const char* const CameraTransitions[] = { "Jump", "Spline" };
        const char* const PersonaTypes[] = { "Credits", "Health", "Skill Points" };
        const char* const Conditions[] = { "<", "<=", "==", ">=", ">" };
        const char* const Fonts[] = { "Normal", "Computer" };
        const std::vector<std::string> Skills = { "SkillComputer", "SkillDemolition", "SkillEnviro", "SkillLockpicking", "SkillMedicine", "SkillSwimming", "SkillTech", "SkillWeaponHeavy", "SkillWeaponLowTech", "SkillWeaponPistol", "SkillWeaponRifle" };

        int textCallback(ImGuiInputTextCallbackData* data)
        {
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                std::string* text = static_cast<std::string*>(data->UserData);
                text->resize(static_cast<size_t>(data->BufTextLen));
                data->Buf = text->data();
            }
            return 0;
        }

        bool editText(const char* label, std::string& text)
        {
            ImGui::SetNextItemWidth(-FLT_MIN);
            return ImGui::InputText(label, text.data(), text.capacity() + 1, ImGuiInputTextFlags_CallbackResize, textCallback, &text);
        }

        bool editMultiline(const char* label, std::string& text, float lines)
        {
            return ImGui::InputTextMultiline(label, text.data(), text.capacity() + 1, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * lines + 8.0f), ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_WordWrap, textCallback, &text);
        }

        bool pickText(const char* label, std::string& value, const std::vector<std::string>& options)
        {
            ImGui::PushID(label);
            ImGui::TextUnformatted(label);
            float button = ImGui::GetFrameHeight();
            ImGui::SetNextItemWidth(-button - ImGui::GetStyle().ItemInnerSpacing.x);
            bool changed = ImGui::InputText("##value", value.data(), value.capacity() + 1, ImGuiInputTextFlags_CallbackResize, textCallback, &value);
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            if (ImGui::BeginCombo("##pick", nullptr, ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightLarge)) {
                for (const std::string& option : options) {
                    if (ImGui::Selectable(option.c_str(), option == value)) {
                        value = option;
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            return changed;
        }

        bool labelled(const char* label, const std::function<bool()>& edit)
        {
            ImGui::TextUnformatted(label);
            ImGui::PushID(label);
            ImGui::SetNextItemWidth(-FLT_MIN);
            bool changed = edit();
            ImGui::PopID();
            return changed;
        }

        bool editInt(const char* label, int32_t& value)
        {
            return labelled(label, [&] {
                return ImGui::InputInt("##value", &value);
            });
        }

        bool editFloat(const char* label, float& value)
        {
            return labelled(label, [&] {
                return ImGui::InputFloat("##value", &value, 0.0f, 0.0f, "%.3f");
            });
        }

        bool editCombo(const char* label, int32_t& value, const char* const* items, int count)
        {
            return labelled(label, [&] {
                return ImGui::Combo("##value", &value, items, count);
            });
        }

        bool editTextField(const char* label, std::string& value)
        {
            ImGui::TextUnformatted(label);
            ImGui::PushID(label);
            bool changed = editText("##value", value);
            ImGui::PopID();
            return changed;
        }

        bool editTextArea(const char* label, std::string& value, float lines)
        {
            ImGui::TextUnformatted(label);
            ImGui::PushID(label);
            bool changed = editMultiline("##value", value, lines);
            ImGui::PopID();
            return changed;
        }

        std::vector<std::string> labelsOf(const ConConversation& conversation)
        {
            std::vector<std::string> labels;
            for (const ConEvent& event : conversation.events) {
                if (!event.label.empty()) {
                    labels.push_back(event.label);
                }
            }
            return labels;
        }

        std::vector<std::string> actorsOf(const dx::compiler::ConFile& file)
        {
            std::set<std::string> names = { "JCDenton" };
            for (const dx::compiler::ConActor& actor : file.actors) {
                names.insert(actor.name);
            }
            for (const ConConversation& conversation : file.conversations) {
                if (!conversation.owner.empty()) {
                    names.insert(conversation.owner);
                }
                for (const ConEvent& event : conversation.events) {
                    for (const std::string* name : { &event.speaker, &event.speakingTo, &event.fromName, &event.toName, &event.ownerName }) {
                        if (!name->empty()) {
                            names.insert(*name);
                        }
                    }
                }
            }
            return std::vector<std::string>(names.begin(), names.end());
        }

        std::string flagsText(const std::vector<ConFlagRef>& flags)
        {
            std::string result;
            for (const ConFlagRef& flag : flags) {
                result += (result.empty() ? "" : ", ") + flag.name + "=" + (flag.value ? "True" : "False");
            }
            return result;
        }

        std::string conversationFlags(const ConConversation& conversation)
        {
            std::string result;
            const std::pair<bool, const char*> flags[] = {
                { conversation.dataLink, "InfoLink" },
                { conversation.displayOnce, "Once" },
                { conversation.firstPerson, "1st" },
                { conversation.nonInteractive, "NonInt" },
                { conversation.invokeFrob, "Frob" },
                { conversation.invokeBump, "Bump" },
                { conversation.invokeSight, "Sight" },
                { conversation.invokeRadius, "Radius" },
            };
            for (const auto& [set, name] : flags) {
                if (set) {
                    result += result.empty() ? name : std::string(" ") + name;
                }
            }
            return result;
        }

        std::string summary(const ConEvent& event, int32_t self)
        {
            switch (event.type) {
            case ConEventType::Speech:
                return event.speaker + ": " + event.speech;
            case ConEventType::Choice: {
                std::string result;
                for (const ConChoice& choice : event.choices) {
                    result += (result.empty() ? "" : " | ") + choice.text + " -> " + choice.label;
                }
                return result;
            }
            case ConEventType::SetFlag:
                return flagsText(event.flags);
            case ConEventType::CheckFlag:
                return "if " + flagsText(event.flags) + " -> " + event.setLabel;
            case ConEventType::CheckObject:
                return event.objectName + " else -> " + event.failLabel;
            case ConEventType::TransferObject:
                return std::to_string(event.transferCount) + " " + event.objectName + " " + event.fromName + " -> " + event.toName;
            case ConEventType::MoveCamera:
                return std::string(CameraTypes[std::clamp(event.cameraType, 0, 3)]) + (event.cameraType == 0 ? std::string(" ") + CameraPositions[std::clamp(event.cameraPosition, 0, 14)] : std::string());
            case ConEventType::Animation:
                return event.ownerName + " plays " + event.sequence;
            case ConEventType::Trade:
                return event.ownerName;
            case ConEventType::Jump:
                return "-> " + event.jumpLabel + (event.conId >= 0 && event.conId != self ? " (conversation " + std::to_string(event.conId) + ")" : std::string());
            case ConEventType::RandomLabel: {
                std::string result;
                for (const std::string& label : event.labels) {
                    result += (result.empty() ? "" : ", ") + label;
                }
                return result;
            }
            case ConEventType::Trigger:
                return event.triggerTag;
            case ConEventType::AddGoal:
                return event.goalName + (event.goalCompleted ? " completed" : ": " + event.goalText);
            case ConEventType::AddNote:
                return event.noteText;
            case ConEventType::AddSkillPoints:
                return std::to_string(event.points) + " " + event.awardMessage;
            case ConEventType::AddCredits:
                return std::to_string(event.credits);
            case ConEventType::CheckPersona:
                return std::string(PersonaTypes[std::clamp(event.personaType, 0, 2)]) + " " + Conditions[std::clamp(event.condition, 0, 4)] + " " + std::to_string(event.personaValue) + " -> " + event.jumpLabel;
            case ConEventType::Comment:
                return event.comment;
            case ConEventType::End:
                return std::string();
            }
            return std::string();
        }

        ConEventType eventTypeNamed(const std::string& name, bool& found)
        {
            for (int32_t i = 0; i < dx::compiler::ConEventTypeCount; i++) {
                std::string type = dx::compiler::conEventTypeName(static_cast<ConEventType>(i));
                type.erase(std::remove(type.begin(), type.end(), ' '), type.end());
                if (lower(type) == lower(name)) {
                    found = true;
                    return static_cast<ConEventType>(i);
                }
            }
            found = false;
            return ConEventType::Speech;
        }

        std::string rest(std::istringstream& input)
        {
            std::string text;
            std::getline(input, text);
            size_t first = text.find_first_not_of(' ');
            return first == std::string::npos ? std::string() : text.substr(first);
        }
    }

    bool Editor::conLoad(const std::filesystem::path& file)
    {
        dx::compiler::ConFile loaded;
        std::string error;
        if (!dx::compiler::loadConFile(file, loaded, &error)) {
            status = error;
            conLog.push_back(error);
            return false;
        }
        conFile = std::move(loaded);
        conPath = file;
        std::snprintf(conPathInput.data(), conPathInput.size(), "%s", file.string().c_str());
        std::string package = file.parent_path().filename().string();
        std::snprintf(conCompileInput.data(), conCompileInput.size(), "%s", lower(package) == "deusexconversations" || package.empty() ? "MyConversations" : package.c_str());
        conDirty = false;
        conIssues.clear();
        conTranscript.clear();
        conPreview = {};
        conSelect(conFile.conversations.empty() ? -1 : 0, -1);
        showConEdit = true;
        status = "loaded " + std::to_string(conFile.conversations.size()) + " conversations from " + file.filename().string();
        conLog.push_back(status);
        return true;
    }

    bool Editor::conOpenPackage(const std::string& packageName, const std::string& file)
    {
        std::filesystem::path directory = dx::config::userDirectory() / "ConEdit" / packageName;
        if (!dx::compiler::exportPackageConFile(packageName, file, directory, conLog)) {
            status = "cannot export " + file + " from " + packageName;
            conLog.push_back(status);
            return false;
        }
        return conLoad(directory / file);
    }

    bool Editor::conSave(const std::filesystem::path& file)
    {
        std::string error;
        if (file.empty() || !dx::compiler::saveConFile(file, conFile, &error)) {
            status = file.empty() ? "no file name" : error;
            conLog.push_back(status);
            return false;
        }
        conPath = file;
        std::snprintf(conPathInput.data(), conPathInput.size(), "%s", file.string().c_str());
        conDirty = false;
        conLog.push_back("saved " + file.string());
        status = "saved " + file.filename().string();
        return true;
    }

    bool Editor::conCompile(const std::string& packageName)
    {
        if (packageName.empty() || lower(packageName) == "deusexconversations") {
            status = "choose a mod package name to compile into";
            conLog.push_back(status);
            return false;
        }
        if ((conDirty || !std::filesystem::exists(conPath)) && !conSave(conPath)) {
            return false;
        }
        std::vector<std::string> lines;
        int32_t count = dx::compiler::compileConFile(conPath.parent_path(), conPath.filename().string(), packageName, lines);
        conLog.insert(conLog.end(), lines.begin(), lines.end());
        std::filesystem::path output = conPath.parent_path().parent_path() / "System";
        conLog.push_back("compiled " + std::to_string(count) + " conversations into " + (output / (packageName + "Text.u")).string());
        status = "compiled " + std::to_string(count) + " conversations into " + packageName + "Text.u";
        conTab = "check";
        return count == static_cast<int32_t>(conFile.conversations.size());
    }

    void Editor::conSelect(int conversation, int event)
    {
        conConversation = conversation;
        conEvent = event;
        conLabelEvent = -1;
    }

    void Editor::conAddEvent(dx::compiler::ConEventType type)
    {
        if (conConversation < 0) {
            return;
        }
        ConConversation& conversation = conFile.conversations[static_cast<size_t>(conConversation)];
        size_t index = conEvent < 0 ? conversation.events.size() : static_cast<size_t>(conEvent) + 1;
        if (conEvent < 0 && !conversation.events.empty() && conversation.events.back().type == ConEventType::End) {
            index--;
        }
        ConEvent event = dx::compiler::makeConEvent(type);
        if (type == ConEventType::Speech && !conversation.owner.empty()) {
            event.speaker = conversation.owner;
            event.speakingTo = "JCDenton";
        }
        if (type == ConEventType::Jump) {
            event.conId = conversation.id;
        }
        dx::compiler::insertConEvent(conversation, index, event);
        conSelect(conConversation, static_cast<int>(std::min(index, conversation.events.size() - 1)));
        conDirty = true;
    }

    bool Editor::conFlagList(const char* id, std::vector<dx::compiler::ConFlagRef>& flags)
    {
        bool changed = false;
        ImGui::PushID(id);
        if (ImGui::BeginTable("flags", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Flag", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("Expires", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < flags.size(); i++) {
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                changed |= editText("##name", flags[i].name);
                ImGui::TableSetColumnIndex(1);
                changed |= ImGui::Checkbox("##value", &flags[i].value);
                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(-FLT_MIN);
                changed |= ImGui::InputInt("##expiration", &flags[i].expiration, 0);
                ImGui::TableSetColumnIndex(3);
                if (ImGui::SmallButton("x")) {
                    flags.erase(flags.begin() + static_cast<std::ptrdiff_t>(i));
                    changed = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        if (ImGui::SmallButton("Add Flag")) {
            flags.push_back({});
            flags.back().name = "NewFlag";
            changed = true;
        }
        ImGui::PopID();
        return changed;
    }

    void Editor::conConversationList()
    {
        if (ImGui::Button("Add")) {
            dx::compiler::addConversation(conFile, "NewConversation" + std::to_string(conFile.conversations.size()), "JCDenton");
            conSelect(static_cast<int>(conFile.conversations.size()) - 1, -1);
            conDirty = true;
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(conConversation < 0);
        if (ImGui::Button("Duplicate")) {
            dx::compiler::ConConversation copy = conFile.conversations[static_cast<size_t>(conConversation)];
            copy.id = dx::compiler::addConversation(conFile, copy.name + "Copy", copy.owner).id;
            copy.name += "Copy";
            conFile.conversations.back() = copy;
            conSelect(static_cast<int>(conFile.conversations.size()) - 1, -1);
            conDirty = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            conFile.conversations.erase(conFile.conversations.begin() + conConversation);
            conSelect(std::min(conConversation, static_cast<int>(conFile.conversations.size()) - 1), -1);
            conDirty = true;
        }
        ImGui::EndDisabled();
        if (ImGui::BeginTable("conversations", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, ImGui::GetContentRegionAvail().y * 0.62f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 1.4f);
            ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableHeadersRow();
            for (size_t i = 0; i < conFile.conversations.size(); i++) {
                const ConConversation& conversation = conFile.conversations[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Selectable(conversation.owner.empty() ? "-" : conversation.owner.c_str(), static_cast<int>(i) == conConversation, ImGuiSelectableFlags_SpanAllColumns)) {
                    conSelect(static_cast<int>(i), -1);
                }
                ImGui::PopID();
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(conversation.name.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(conversationFlags(conversation).c_str());
            }
            ImGui::EndTable();
        }
        if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen)) {
            conDirty |= editTextField("Audio package", conFile.audioPackage);
            std::string missions;
            for (int32_t mission : conFile.missions) {
                missions += (missions.empty() ? "" : " ") + std::to_string(mission);
            }
            if (editTextField("Missions", missions)) {
                conFile.missions.clear();
                std::istringstream values(missions);
                int32_t mission = 0;
                while (values >> mission) {
                    conFile.missions.push_back(mission);
                }
                conDirty = true;
            }
            conDirty |= editTextArea("Notes", conFile.notes, 2.0f);
        }
    }

    void Editor::conConversationProperties(dx::compiler::ConConversation& conversation)
    {
        if (!ImGui::CollapsingHeader("Conversation", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }
        ImGui::PushID("conversation");
        if (ImGui::BeginTable("props", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            conDirty |= editTextField("Name", conversation.name);
            ImGui::TableSetColumnIndex(1);
            conDirty |= pickText("Owner", conversation.owner, actorsOf(conFile));
            ImGui::EndTable();
        }
        conDirty |= editTextField("Description", conversation.description);
        const std::pair<bool*, const char*> flags[] = {
            { &conversation.dataLink, "InfoLink" },
            { &conversation.displayOnce, "Display once" },
            { &conversation.firstPerson, "First person" },
            { &conversation.nonInteractive, "Non-interactive" },
            { &conversation.randomCamera, "Random camera" },
            { &conversation.canBeInterrupted, "Can be interrupted" },
            { &conversation.cannotBeInterrupted, "Cannot be interrupted" },
            { &conversation.invokeFrob, "Invoke on frob" },
            { &conversation.invokeBump, "Invoke on bump" },
            { &conversation.invokeSight, "Invoke on sight" },
            { &conversation.invokeRadius, "Invoke in radius" },
        };
        if (ImGui::BeginTable("flags", 3)) {
            for (const auto& [value, name] : flags) {
                ImGui::TableNextColumn();
                conDirty |= ImGui::Checkbox(name, value);
            }
            ImGui::EndTable();
        }
        if (conversation.invokeRadius) {
            conDirty |= editInt("Radius", conversation.radius);
        }
        ImGui::TextUnformatted("Required flags");
        conDirty |= conFlagList("required", conversation.flags);
        ImGui::PopID();
    }

    void Editor::conEventList(dx::compiler::ConConversation& conversation)
    {
        ImGui::SetNextItemWidth(150.0f);
        ImGui::Combo("##type", &conAddType, [](void*, int index) { return dx::compiler::conEventTypeName(static_cast<ConEventType>(index)); }, nullptr, dx::compiler::ConEventTypeCount);
        ImGui::SameLine();
        if (ImGui::Button("Add Event")) {
            conAddEvent(static_cast<ConEventType>(conAddType));
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(conEvent < 0);
        if (ImGui::Button("Duplicate")) {
            dx::compiler::insertConEvent(conversation, static_cast<size_t>(conEvent) + 1, conversation.events[static_cast<size_t>(conEvent)]);
            conversation.events[static_cast<size_t>(conEvent) + 1].label.clear();
            conSelect(conConversation, conEvent + 1);
            conDirty = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            dx::compiler::removeConEvent(conversation, static_cast<size_t>(conEvent));
            conSelect(conConversation, std::min(conEvent, static_cast<int>(conversation.events.size()) - 1));
            conDirty = true;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("up", ImGuiDir_Up) && conEvent > 0 && dx::compiler::moveConEvent(conversation, static_cast<size_t>(conEvent), static_cast<size_t>(conEvent - 1))) {
            conSelect(conConversation, conEvent - 1);
            conDirty = true;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("down", ImGuiDir_Down) && dx::compiler::moveConEvent(conversation, static_cast<size_t>(conEvent), static_cast<size_t>(conEvent + 1))) {
            conSelect(conConversation, conEvent + 1);
            conDirty = true;
        }
        ImGui::EndDisabled();
        if (!ImGui::BeginTable("events", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY)) {
            return;
        }
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 28.0f);
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Summary", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        int previewed = conPreview.conversation == static_cast<size_t>(conConversation) ? conPreview.event : -1;
        for (size_t i = 0; i < conversation.events.size(); i++) {
            const ConEvent& event = conversation.events[i];
            ImGui::TableNextRow();
            if (static_cast<int>(i) == previewed) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(70, 110, 60, 160));
            }
            ImGui::TableSetColumnIndex(0);
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(std::to_string(i).c_str(), static_cast<int>(i) == conEvent, ImGuiSelectableFlags_SpanAllColumns)) {
                conSelect(conConversation, static_cast<int>(i));
            }
            ImGui::PopID();
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(event.label.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(dx::compiler::conEventTypeName(event.type));
            ImGui::TableSetColumnIndex(3);
            std::string text = summary(event, conversation.id);
            ImGui::TextUnformatted(text.c_str(), text.c_str() + std::min<size_t>(text.size(), 160));
        }
        ImGui::EndTable();
    }

    void Editor::conChoiceEditor(dx::compiler::ConConversation& conversation, dx::compiler::ConEvent& event)
    {
        conDirty |= ImGui::Checkbox("Clear screen", &event.clearScreen);
        std::vector<std::string> labels = labelsOf(conversation);
        for (size_t i = 0; i < event.choices.size(); i++) {
            ConChoice& choice = event.choices[i];
            ImGui::PushID(static_cast<int>(i));
            bool open = ImGui::TreeNodeEx("choice", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap, "Choice %zu: %s", i + 1, choice.text.c_str());
            ImGui::SameLine(ImGui::GetContentRegionMax().x - 70.0f);
            bool up = ImGui::SmallButton("^");
            ImGui::SameLine();
            bool down = ImGui::SmallButton("v");
            ImGui::SameLine();
            bool remove = ImGui::SmallButton("x");
            if (open) {
                conDirty |= editTextArea("Text", choice.text, 2.0f);
                conDirty |= pickText("Jump to label", choice.label, labels);
                conDirty |= ImGui::Checkbox("Display as speech", &choice.displayAsSpeech);
                conDirty |= ImGui::Checkbox("Requires skill", &choice.hasSkill);
                if (choice.hasSkill) {
                    conDirty |= pickText("Skill", choice.skill, Skills);
                    conDirty |= editInt("Skill level", choice.skillLevel);
                }
                conDirty |= editTextField("Audio", choice.audio);
                ImGui::TextDisabled("imports %s", dx::compiler::conChoiceAudioPath(conversation, static_cast<size_t>(conEvent), i).c_str());
                ImGui::TextUnformatted("Show only if");
                conDirty |= conFlagList("flags", choice.flags);
                ImGui::TreePop();
            }
            ImGui::PopID();
            if (remove) {
                event.choices.erase(event.choices.begin() + static_cast<std::ptrdiff_t>(i));
                conDirty = true;
                break;
            }
            if (up && i > 0) {
                std::swap(event.choices[i], event.choices[i - 1]);
                conDirty = true;
            }
            if (down && i + 1 < event.choices.size()) {
                std::swap(event.choices[i], event.choices[i + 1]);
                conDirty = true;
            }
        }
        if (ImGui::Button("Add Choice")) {
            ConChoice choice;
            choice.text = "New choice";
            event.choices.push_back(choice);
            conDirty = true;
        }
    }

    void Editor::conEventEditor(dx::compiler::ConConversation& conversation)
    {
        if (conEvent < 0 || static_cast<size_t>(conEvent) >= conversation.events.size()) {
            ImGui::TextDisabled("Select an event");
            return;
        }
        ConEvent& event = conversation.events[static_cast<size_t>(conEvent)];
        ImGui::PushID(conEvent);
        ImGui::Text("%s event %d", dx::compiler::conEventTypeName(event.type), conEvent);
        if (conLabelEvent != conEvent) {
            conLabelBuffer = event.label;
            conLabelEvent = conEvent;
        }
        ImGui::TextUnformatted("Label");
        editText("##label", conLabelBuffer);
        if (ImGui::IsItemDeactivatedAfterEdit() && conLabelBuffer != event.label) {
            if (event.label.empty()) {
                event.label = conLabelBuffer;
            } else {
                dx::compiler::renameConLabel(conversation, event.label, conLabelBuffer);
            }
            conDirty = true;
        }
        ImGui::Separator();
        std::vector<std::string> labels = labelsOf(conversation);
        std::vector<std::string> actorNames = actorsOf(conFile);
        switch (event.type) {
        case ConEventType::Speech: {
            conDirty |= pickText("Speaker", event.speaker, actorNames);
            conDirty |= pickText("Speaking to", event.speakingTo, actorNames);
            conDirty |= editTextArea("Speech", event.speech, 5.0f);
            conDirty |= editTextField("Audio", event.audio);
            ImGui::TextDisabled("imports %s", dx::compiler::conSpeechAudioPath(conversation, static_cast<size_t>(conEvent)).c_str());
            conDirty |= ImGui::Checkbox("Continued", &event.continued);
            ImGui::SameLine();
            conDirty |= ImGui::Checkbox("Bold", &event.bold);
            conDirty |= editCombo("Font", event.font, Fonts, 2);
            break;
        }
        case ConEventType::Choice:
            conChoiceEditor(conversation, event);
            break;
        case ConEventType::SetFlag:
            conDirty |= conFlagList("set", event.flags);
            break;
        case ConEventType::CheckFlag:
            ImGui::TextUnformatted("When all flags match");
            conDirty |= conFlagList("check", event.flags);
            conDirty |= pickText("Jump to label", event.setLabel, labels);
            break;
        case ConEventType::CheckObject:
            conDirty |= editTextField("Object class", event.objectName);
            conDirty |= pickText("Label if missing", event.failLabel, labels);
            break;
        case ConEventType::TransferObject:
            conDirty |= editTextField("Object class", event.objectName);
            conDirty |= editInt("Count", event.transferCount);
            conDirty |= pickText("From", event.fromName, actorNames);
            conDirty |= pickText("To", event.toName, actorNames);
            conDirty |= pickText("Label if it fails", event.failLabel, labels);
            break;
        case ConEventType::MoveCamera:
            conDirty |= editCombo("Camera", event.cameraType, CameraTypes, 4);
            conDirty |= editCombo("Position", event.cameraPosition, CameraPositions, 15);
            conDirty |= editCombo("Transition", event.cameraTransition, CameraTransitions, 2);
            if (event.cameraType == 1) {
                conDirty |= labelled("Rotation", [&] {
                    return ImGui::InputInt3("##value", event.cameraRotation.data());
                });
                conDirty |= editFloat("Height modifier", event.heightModifier);
                conDirty |= editFloat("Center modifier", event.centerModifier);
                conDirty |= editFloat("Distance multiplier", event.distanceMultiplier);
            } else if (event.cameraType == 2) {
                conDirty |= labelled("Offset", [&] {
                    return ImGui::InputFloat3("##value", event.cameraOffset.data());
                });
            }
            break;
        case ConEventType::Animation: {
            conDirty |= pickText("Actor", event.ownerName, actorNames);
            conDirty |= editTextField("Sequence", event.sequence);
            bool finish = (event.animMode & 1) != 0;
            bool loop = (event.animMode & 2) != 0;
            bool changed = ImGui::Checkbox("Wait to finish", &finish);
            changed |= ImGui::Checkbox("Loop", &loop);
            if (changed) {
                event.animMode = (event.animMode & ~3) | (finish ? 1 : 0) | (loop ? 2 : 0);
                conDirty = true;
            }
            break;
        }
        case ConEventType::Trade:
            conDirty |= pickText("Trader", event.ownerName, actorNames);
            break;
        case ConEventType::Jump: {
            std::string target = "This conversation";
            const ConConversation* destination = &conversation;
            for (const ConConversation& other : conFile.conversations) {
                if (event.conId >= 0 && other.id == event.conId) {
                    target = other.name;
                    destination = &other;
                }
            }
            ImGui::TextUnformatted("Conversation");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::BeginCombo("##conversation", target.c_str())) {
                for (const ConConversation& other : conFile.conversations) {
                    if (ImGui::Selectable(other.name.c_str(), other.id == event.conId)) {
                        event.conId = other.id;
                        conDirty = true;
                    }
                }
                ImGui::EndCombo();
            }
            conDirty |= pickText("Jump to label", event.jumpLabel, labelsOf(*destination));
            break;
        }
        case ConEventType::RandomLabel:
            for (size_t i = 0; i < event.labels.size(); i++) {
                ImGui::PushID(static_cast<int>(i));
                conDirty |= pickText("Label", event.labels[i], labels);
                if (ImGui::SmallButton("Remove")) {
                    event.labels.erase(event.labels.begin() + static_cast<std::ptrdiff_t>(i));
                    conDirty = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Label")) {
                event.labels.emplace_back();
                conDirty = true;
            }
            conDirty |= ImGui::Checkbox("Cycle", &event.cycleEvents);
            ImGui::SameLine();
            conDirty |= ImGui::Checkbox("Once", &event.cycleOnce);
            ImGui::SameLine();
            conDirty |= ImGui::Checkbox("Random", &event.cycleRandom);
            break;
        case ConEventType::Trigger:
            conDirty |= editTextField("Event tag", event.triggerTag);
            break;
        case ConEventType::AddGoal:
            conDirty |= editTextField("Goal name", event.goalName);
            conDirty |= ImGui::Checkbox("Completed", &event.goalCompleted);
            if (!event.goalCompleted) {
                conDirty |= ImGui::Checkbox("Primary goal", &event.primaryGoal);
                conDirty |= editTextArea("Goal text", event.goalText, 4.0f);
            }
            break;
        case ConEventType::AddNote:
            conDirty |= editTextArea("Note", event.noteText, 6.0f);
            break;
        case ConEventType::AddSkillPoints:
            conDirty |= editInt("Points", event.points);
            conDirty |= editTextField("Award message", event.awardMessage);
            break;
        case ConEventType::AddCredits:
            conDirty |= editInt("Credits", event.credits);
            break;
        case ConEventType::CheckPersona:
            conDirty |= editCombo("Value", event.personaType, PersonaTypes, 3);
            conDirty |= editCombo("Condition", event.condition, Conditions, 5);
            conDirty |= editInt("Compare to", event.personaValue);
            conDirty |= pickText("Jump to label", event.jumpLabel, labels);
            break;
        case ConEventType::Comment:
            conDirty |= editTextArea("Comment", event.comment, 8.0f);
            break;
        case ConEventType::End:
            ImGui::TextDisabled("Ends the conversation");
            break;
        }
        ImGui::PopID();
    }

    void Editor::conPreviewPanel()
    {
        if (conConversation < 0) {
            ImGui::TextDisabled("Select a conversation");
            return;
        }
        if (ImGui::Button("Start")) {
            dx::compiler::startConPreview(conFile, static_cast<size_t>(conConversation), conPreview);
            conTranscript.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset State")) {
            conPreview = {};
            conTranscript.clear();
        }
        ImGui::SameLine();
        ImGui::Text("Credits %d  Skill points %d", conPreview.credits, conPreview.skillPoints);
        ImGui::BeginChild("transcript", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.55f), ImGuiChildFlags_Borders);
        for (const std::string& line : conTranscript) {
            ImGui::TextWrapped("%s", line.c_str());
        }
        ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
        bool active = conPreview.event >= 0 && conPreview.conversation < conFile.conversations.size() && static_cast<size_t>(conPreview.event) < conFile.conversations[conPreview.conversation].events.size();
        if (!active) {
            ImGui::TextDisabled("Not running");
        } else {
            const ConConversation& conversation = conFile.conversations[conPreview.conversation];
            const ConEvent& event = conversation.events[static_cast<size_t>(conPreview.event)];
            ImGui::Text("%s / %d: %s", conversation.name.c_str(), conPreview.event, dx::compiler::conEventTypeName(event.type));
            if (event.type == ConEventType::Choice) {
                for (size_t i = 0; i < event.choices.size(); i++) {
                    ImGui::PushID(static_cast<int>(i));
                    ImGui::BeginDisabled(!dx::compiler::conChoiceAvailable(event.choices[i], conPreview));
                    if (ImGui::Button(event.choices[i].text.c_str(), ImVec2(-FLT_MIN, 0))) {
                        pendingCommand = "conedit step " + std::to_string(i);
                    }
                    ImGui::EndDisabled();
                    ImGui::PopID();
                }
            } else if (ImGui::Button("Step")) {
                pendingCommand = "conedit step";
            }
        }
        if (ImGui::CollapsingHeader("Flags")) {
            for (auto& [name, value] : conPreview.flags) {
                ImGui::Checkbox(name.c_str(), &value);
            }
        }
    }

    void Editor::conCheckPanel()
    {
        if (ImGui::Button("Validate")) {
            pendingCommand = "conedit check";
        }
        ImGui::SameLine();
        ImGui::Text("%zu problems", conIssues.size());
        ImGui::BeginChild("issues", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), ImGuiChildFlags_Borders);
        for (const std::string& issue : conIssues) {
            ImGui::TextWrapped("%s", issue.c_str());
        }
        ImGui::EndChild();
        ImGui::TextUnformatted("Log");
        ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        for (const std::string& line : conLog) {
            ImGui::TextUnformatted(line.c_str());
        }
        ImGui::EndChild();
    }

    void Editor::conversationWindow()
    {
        if (!showConEdit) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(1180, 680), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(200, 90), ImGuiCond_FirstUseEver);
        std::string title = "Conversation Editor - " + (conPath.empty() ? std::string("untitled") : conPath.filename().string()) + (conDirty ? " *" : "") + "###conedit";
        if (!ImGui::Begin(title.c_str(), &showConEdit)) {
            ImGui::End();
            return;
        }
        if (conPackageInput[0] == 0) {
            std::snprintf(conPackageInput.data(), conPackageInput.size(), "DeusExConversations");
        }
        if (ImGui::Button("New")) {
            pendingCommand = "conedit new";
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(420.0f);
        ImGui::InputTextWithHint("##path", ".con file", conPathInput.data(), conPathInput.size());
        ImGui::SameLine();
        if (ImGui::Button("Open")) {
            conLoad(conPathInput.data());
        }
        ImGui::SameLine();
        if (ImGui::Button("Save")) {
            conSave(conPathInput.data());
        }
        ImGui::SameLine();
        ImGui::TextUnformatted("Compile into");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("##compile", conCompileInput.data(), conCompileInput.size());
        ImGui::SameLine();
        ImGui::BeginDisabled(conPath.empty() && conPathInput[0] == 0);
        if (ImGui::Button("Compile")) {
            pendingCommand = "conedit compile " + std::string(conCompileInput.data());
        }
        ImGui::EndDisabled();
        ImGui::TextUnformatted("Package");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        ImGui::InputText("##package", conPackageInput.data(), conPackageInput.size());
        ImGui::SameLine();
        if (ImGui::Button("List")) {
            conPackageFiles = dx::compiler::packageConFiles(conPackageInput.data());
            conPackageFile = 0;
            status = std::to_string(conPackageFiles.size()) + " conversation files in " + conPackageInput.data();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::BeginCombo("##files", conPackageFiles.empty() ? "" : conPackageFiles[static_cast<size_t>(conPackageFile)].c_str())) {
            for (size_t i = 0; i < conPackageFiles.size(); i++) {
                if (ImGui::Selectable(conPackageFiles[i].c_str(), static_cast<int>(i) == conPackageFile)) {
                    conPackageFile = static_cast<int>(i);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(conPackageFiles.empty());
        if (ImGui::Button("Open From Package")) {
            pendingCommand = "conedit package " + std::string(conPackageInput.data()) + " " + conPackageFiles[static_cast<size_t>(conPackageFile)];
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::BeginTable("layout", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV, ImGui::GetContentRegionAvail())) {
            ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.9f);
            ImGui::TableSetupColumn("middle", ImGuiTableColumnFlags_WidthStretch, 1.6f);
            ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch, 1.1f);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("left");
            conConversationList();
            ImGui::EndChild();
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("middle");
            if (conConversation >= 0 && static_cast<size_t>(conConversation) < conFile.conversations.size()) {
                ConConversation& conversation = conFile.conversations[static_cast<size_t>(conConversation)];
                conConversationProperties(conversation);
                conEventList(conversation);
            } else {
                ImGui::TextDisabled("Open a .con file or a conversation package");
            }
            ImGui::EndChild();
            ImGui::TableSetColumnIndex(2);
            ImGui::BeginChild("right");
            if (ImGui::BeginTabBar("conTabs")) {
                if (ImGui::BeginTabItem("Event", nullptr, conTab == "event" ? ImGuiTabItemFlags_SetSelected : 0)) {
                    if (conConversation >= 0 && static_cast<size_t>(conConversation) < conFile.conversations.size()) {
                        conEventEditor(conFile.conversations[static_cast<size_t>(conConversation)]);
                    }
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Preview", nullptr, conTab == "preview" ? ImGuiTabItemFlags_SetSelected : 0)) {
                    conPreviewPanel();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Check", nullptr, conTab == "check" ? ImGuiTabItemFlags_SetSelected : 0)) {
                    conCheckPanel();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            conTab.clear();
            ImGui::EndChild();
            ImGui::EndTable();
        }
        ImGui::End();
    }

    bool Editor::conCommand(const std::string& line)
    {
        std::istringstream input(line);
        std::string verb;
        input >> verb;
        showConEdit = true;
        if (verb == "show" || verb.empty()) {
            return true;
        }
        if (verb == "tab") {
            input >> conTab;
            return true;
        }
        if (verb == "new") {
            conFile = {};
            conFile.audioPackage = "Mod";
            dx::compiler::addConversation(conFile, "NewConversation", "JCDenton");
            conPath = dx::config::userDirectory() / "MyConversations" / "MyConversations.con";
            std::snprintf(conPathInput.data(), conPathInput.size(), "%s", conPath.string().c_str());
            std::snprintf(conCompileInput.data(), conCompileInput.size(), "MyConversations");
            conSelect(0, -1);
            conDirty = true;
            return true;
        }
        if (verb == "open") {
            return conLoad(rest(input));
        }
        if (verb == "package") {
            std::string packageName;
            std::string file;
            input >> packageName >> file;
            return conOpenPackage(packageName, file);
        }
        if (verb == "save") {
            std::string path = rest(input);
            return conSave(path.empty() ? conPath : std::filesystem::path(path));
        }
        if (verb == "compile") {
            std::string packageName = rest(input);
            return conCompile(packageName.empty() ? std::string(conCompileInput.data()) : packageName);
        }
        if (verb == "check") {
            conIssues = dx::compiler::checkConFile(conFile);
            status = std::to_string(conIssues.size()) + " conversation problems";
            conTab = "check";
            return true;
        }
        if (verb == "select") {
            int conversation = -1;
            int event = -1;
            input >> conversation >> event;
            if (conversation < 0 || static_cast<size_t>(conversation) >= conFile.conversations.size() || event >= static_cast<int>(conFile.conversations[static_cast<size_t>(conversation)].events.size())) {
                status = "no such conversation or event";
                return false;
            }
            conSelect(conversation, event);
            return true;
        }
        if (verb == "addconversation") {
            std::string name;
            std::string owner;
            input >> name >> owner;
            dx::compiler::addConversation(conFile, name, owner);
            conSelect(static_cast<int>(conFile.conversations.size()) - 1, -1);
            conDirty = true;
            return true;
        }
        if (conConversation < 0 || static_cast<size_t>(conConversation) >= conFile.conversations.size()) {
            status = "no conversation selected";
            return false;
        }
        ConConversation& conversation = conFile.conversations[static_cast<size_t>(conConversation)];
        if (verb == "preview") {
            dx::compiler::startConPreview(conFile, static_cast<size_t>(conConversation), conPreview);
            conTranscript.clear();
            conTab = "preview";
            return true;
        }
        if (verb == "step") {
            int choice = -1;
            input >> choice;
            if (conPreview.event < 0 || conPreview.conversation >= conFile.conversations.size()) {
                status = "preview is not running";
                return false;
            }
            const ConConversation& current = conFile.conversations[conPreview.conversation];
            const ConEvent& event = current.events[static_cast<size_t>(conPreview.event)];
            std::string entry = event.type == ConEventType::Speech ? event.speaker + ": " + event.speech : "[" + std::string(dx::compiler::conEventTypeName(event.type)) + "] " + summary(event, current.id);
            if (event.type == ConEventType::Choice && choice >= 0 && static_cast<size_t>(choice) < event.choices.size()) {
                entry = "JCDenton chose: " + event.choices[static_cast<size_t>(choice)].text;
            }
            if (!dx::compiler::advanceConPreview(conFile, conPreview, choice)) {
                status = "choose an available choice";
                return false;
            }
            conTranscript.push_back(entry);
            if (conPreview.event < 0) {
                conTranscript.push_back("(conversation ended)");
            }
            status = "preview at event " + std::to_string(conPreview.event);
            conTab = "preview";
            return true;
        }
        if (verb == "addevent") {
            bool found = false;
            ConEventType type = eventTypeNamed(rest(input), found);
            if (!found) {
                status = "unknown event type";
                return false;
            }
            conAddEvent(type);
            conTab = "event";
            return true;
        }
        if (verb == "conversation") {
            std::string field;
            input >> field;
            std::string value = rest(input);
            if (field == "name") {
                conversation.name = value;
            } else if (field == "owner") {
                conversation.owner = value;
            } else if (field == "description") {
                conversation.description = value;
            } else {
                status = "unknown conversation field";
                return false;
            }
            conDirty = true;
            return true;
        }
        if (conEvent < 0 || static_cast<size_t>(conEvent) >= conversation.events.size()) {
            status = "no event selected";
            return false;
        }
        ConEvent& event = conversation.events[static_cast<size_t>(conEvent)];
        conDirty = true;
        if (verb == "delete") {
            dx::compiler::removeConEvent(conversation, static_cast<size_t>(conEvent));
            conSelect(conConversation, std::min(conEvent, static_cast<int>(conversation.events.size()) - 1));
            return true;
        }
        if (verb == "up" || verb == "down") {
            int target = verb == "up" ? conEvent - 1 : conEvent + 1;
            if (target < 0 || !dx::compiler::moveConEvent(conversation, static_cast<size_t>(conEvent), static_cast<size_t>(target))) {
                return false;
            }
            conSelect(conConversation, target);
            return true;
        }
        if (verb == "label") {
            std::string label = rest(input);
            if (event.label.empty()) {
                event.label = label;
            } else {
                dx::compiler::renameConLabel(conversation, event.label, label);
            }
            conLabelEvent = -1;
            return true;
        }
        if (verb == "speaker") {
            input >> event.speaker >> event.speakingTo;
            return true;
        }
        if (verb == "text") {
            std::string text = rest(input);
            std::string* targets[] = { &event.speech, &event.noteText, &event.goalText, &event.comment, &event.awardMessage };
            const ConEventType types[] = { ConEventType::Speech, ConEventType::AddNote, ConEventType::AddGoal, ConEventType::Comment, ConEventType::AddSkillPoints };
            for (size_t i = 0; i < 5; i++) {
                if (event.type == types[i]) {
                    *targets[i] = text;
                    return true;
                }
            }
            status = "event has no text";
            return false;
        }
        if (verb == "target") {
            std::string label = rest(input);
            std::string* targets[] = { &event.setLabel, &event.failLabel, &event.failLabel, &event.jumpLabel, &event.jumpLabel };
            const ConEventType types[] = { ConEventType::CheckFlag, ConEventType::CheckObject, ConEventType::TransferObject, ConEventType::Jump, ConEventType::CheckPersona };
            for (size_t i = 0; i < 5; i++) {
                if (event.type == types[i]) {
                    *targets[i] = label;
                    return true;
                }
            }
            status = "event has no jump label";
            return false;
        }
        if (verb == "flag") {
            std::string name;
            std::string value;
            input >> name >> value;
            ConFlagRef flag;
            flag.name = name;
            flag.value = lower(value) != "false";
            event.flags.push_back(flag);
            return true;
        }
        if (verb == "addchoice" && event.type == ConEventType::Choice) {
            ConChoice choice;
            input >> choice.label;
            choice.text = rest(input);
            event.choices.push_back(choice);
            return true;
        }
        status = "unknown conedit command " + verb;
        return false;
    }
}
