#include "editor/editor.h"

namespace dx::editor {
    void Editor::surfacePanel()
    {
        dx::UModel* model = level ? level->model : nullptr;
        if (!model || selectedSurfs.empty()) {
            ImGui::TextUnformatted("No surfaces selected (shift+click in the 3D view)");
            if (model && ImGui::Button("Select all surfaces")) {
                command("polyselect all");
            }
            return;
        }
        const dx::BspSurf& surf = model->surfs[static_cast<size_t>(*selectedSurfs.begin())];
        ImGui::Text("%zu surfaces, texture %s", selectedSurfs.size(), surf.texture ? surf.texture->name.str().c_str() : "None");
        if (surf.texture && ImGui::Button("Texture properties")) {
            command("texprops " + surf.texture->pathName());
        }
        ImGui::SameLine();
        if (surf.texture && ImGui::Button("Find in browser")) {
            command("findtexture");
        }
        if (ImGui::CollapsingHeader("Select surfaces")) {
            surfaceSelectMenu();
        }
        ImGui::Text("Pan U %d V %d", surf.panU, surf.panV);
        if (ImGui::Button("Apply current texture") && currentTexture) {
            command("applytexture");
        }
        ImGui::InputInt2("Pan", panStep);
        if (ImGui::Button("Pan")) {
            command("surfpan " + std::to_string(panStep[0]) + " " + std::to_string(panStep[1]));
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset pan")) {
            command("surfpanabs 0 0");
        }
        ImGui::InputFloat2("Scale U V", scaleValues);
        if (ImGui::Button("Scale")) {
            command("surfscale " + std::to_string(1.0f / scaleValues[0]) + " 0 0 " + std::to_string(1.0f / scaleValues[1]));
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotate 90")) {
            command("surfrotate 90");
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotate 45")) {
            command("surfrotate 45");
        }
        ImGui::SameLine();
        if (ImGui::Button("Flip U")) {
            command("surfscale -1 0 0 1 relative");
        }
        ImGui::SameLine();
        if (ImGui::Button("Flip V")) {
            command("surfscale 1 0 0 -1 relative");
        }
        const char* aligns[] = { "default", "floor", "wall", "wallpan", "onetile" };
        for (const char* align : aligns) {
            if (ImGui::Button((std::string("Align ") + align).c_str())) {
                command(std::string("surfalign ") + align);
            }
            ImGui::SameLine();
        }
        ImGui::NewLine();
        const char* details[] = { "high", "normal", "low", "lowest" };
        for (const char* detail : details) {
            if (ImGui::Button((std::string("Light map ") + detail).c_str())) {
                command(std::string("surfdetail ") + detail);
            }
            ImGui::SameLine();
        }
        ImGui::NewLine();
        static const std::pair<uint32_t, const char*> flags[] = {
            { 0x00000001, "Invisible" },
            { 0x00000002, "Masked" },
            { 0x00000004, "Translucent" },
            { 0x00000008, "Not solid" },
            { 0x00000010, "Environment" },
            { 0x00000020, "Semisolid" },
            { 0x00000040, "Modulated" },
            { 0x00000080, "Fake backdrop" },
            { 0x00000100, "Two sided" },
            { 0x00000200, "U pan" },
            { 0x00000400, "V pan" },
            { 0x00000800, "No smooth" },
            { 0x00001000, "Big wavy" },
            { 0x00002000, "Small wavy" },
            { 0x00004000, "Flat" },
            { 0x00010000, "No merge" },
            { 0x00020000, "Cloud wavy" },
            { 0x00040000, "Dirty shadows" },
            { 0x00080000, "Bright corners" },
            { 0x00100000, "Special lit" },
            { 0x00200000, "Gouraud" },
            { 0x00400000, "Unlit" },
            { 0x04000000, "Zone portal" },
            { 0x08000000, "Mirrored" },
        };
        for (const auto& [bit, label] : flags) {
            bool value = (surf.polyFlags & bit) != 0;
            if (ImGui::Checkbox(label, &value)) {
                char text[64];
                std::snprintf(text, sizeof(text), "surfflags 0x%x 0x%x", value ? bit : 0u, value ? 0u : bit);
                command(text);
            }
        }
    }

    bool Editor::surfaceEdit(const std::string& label, const std::function<void(dx::UModel&)>& edit)
    {
        dx::UModel* model = level ? level->model : nullptr;
        if (!model || selectedSurfs.empty()) {
            status = "no surfaces selected";
            return false;
        }
        std::vector<dx::UObject*> touched { model };
        for (int32_t index : selectedSurfs) {
            dx::UObject* owner = model->surfs[static_cast<size_t>(index)].actor;
            dx::UModel* brush = owner && owner->data ? dx::cast<dx::UModel>(dx::actor::Brush(owner)) : nullptr;
            if (brush && brush->polys) {
                touched.push_back(brush->polys);
            }
        }
        history.begin(level, touched, label);
        edit(*model);
        history.commit();
        renderer->flushLevel();
        status = label + " on " + std::to_string(selectedSurfs.size()) + " surfaces";
        return true;
    }

    void Editor::surfaceSelectMenu()
    {
        static const std::pair<const char*, const char*> entries[] = {
            { "Matching Groups", "matching groups" },
            { "Matching Items", "matching items" },
            { "Matching Brush", "matching brush" },
            { "Matching Texture", "matching texture" },
            { nullptr, nullptr },
            { "All Adjacents", "adjacent all" },
            { "Adjacent Coplanars", "adjacent coplanars" },
            { "Adjacent Walls", "adjacent walls" },
            { "Adjacent Floors/Ceils", "adjacent floors" },
            { "Adjacent Slants", "adjacent slants" },
            { nullptr, nullptr },
            { "Reverse", "reverse" },
            { nullptr, nullptr },
            { "Memorize Set", "memory set" },
            { "Recall Memory", "memory recall" },
            { "Or with Memory", "memory or" },
            { "And with Memory", "memory and" },
            { "Xor with Memory", "memory xor" },
            { nullptr, nullptr },
            { "Select All Surfaces", "all" },
            { "Select None", "none" },
        };
        for (const auto& [label, verb] : entries) {
            if (!label) {
                ImGui::Separator();
                continue;
            }
            if (ImGui::MenuItem(label)) {
                command(std::string("polyselect ") + verb);
            }
        }
    }

    void Editor::surfacePopup()
    {
        if (!ImGui::BeginPopup("SurfacePopup")) {
            return;
        }
        dx::UModel* model = level ? level->model : nullptr;
        dx::UObject* texture = model && !selectedSurfs.empty() ? model->surfs[static_cast<size_t>(*selectedSurfs.begin())].texture : nullptr;
        ImGui::TextDisabled("%zu surfaces", selectedSurfs.size());
        if (ImGui::MenuItem("Surface Properties")) {
            pendingTab = "surface";
        }
        if (ImGui::MenuItem("Texture Properties", nullptr, false, texture != nullptr)) {
            command("texprops " + texture->pathName());
        }
        if (ImGui::MenuItem("Find Texture in Browser", nullptr, false, texture != nullptr)) {
            command("findtexture");
        }
        if (ImGui::MenuItem("Apply Current Texture", nullptr, false, currentTexture != nullptr)) {
            command("applytexture");
        }
        if (ImGui::BeginMenu("Select Surfaces")) {
            surfaceSelectMenu();
            ImGui::EndMenu();
        }
        ImGui::Separator();
        addHereItems();
        ImGui::EndPopup();
    }

    bool Editor::polySelect(const std::string& line)
    {
        dx::UModel* model = level ? level->model : nullptr;
        if (!model) {
            return false;
        }
        std::istringstream words(lower(line));
        std::string verb;
        std::string kind;
        words >> verb >> kind;
        if (verb == "none") {
            selectedSurfs.clear();
        } else if (verb == "all") {
            dx::editorops::selectAllSurfs(*model, selectedSurfs);
        } else if (verb == "reverse") {
            dx::editorops::selectReverse(*model, selectedSurfs);
        } else if (verb == "matching" && kind == "groups") {
            dx::editorops::selectMatchingGroups(*model, selectedSurfs);
        } else if (verb == "matching" && kind == "items") {
            dx::editorops::selectMatchingItems(*model, selectedSurfs);
        } else if (verb == "matching" && kind == "brush") {
            dx::editorops::selectMatchingBrush(*model, selectedSurfs);
        } else if (verb == "matching" && kind == "texture") {
            dx::editorops::selectMatchingTexture(*model, selectedSurfs);
        } else if (verb == "adjacent" && kind == "all") {
            dx::editorops::selectAdjacent(*model, selectedSurfs, dx::editorops::Adjacent::All);
        } else if (verb == "adjacent" && kind == "coplanars") {
            dx::editorops::selectAdjacent(*model, selectedSurfs, dx::editorops::Adjacent::Coplanars);
        } else if (verb == "adjacent" && kind == "walls") {
            dx::editorops::selectAdjacent(*model, selectedSurfs, dx::editorops::Adjacent::Walls);
        } else if (verb == "adjacent" && (kind == "floors" || kind == "ceilings")) {
            dx::editorops::selectAdjacent(*model, selectedSurfs, dx::editorops::Adjacent::Floors);
        } else if (verb == "adjacent" && kind == "slants") {
            dx::editorops::selectAdjacent(*model, selectedSurfs, dx::editorops::Adjacent::Slants);
        } else if (verb == "memory" && kind == "set") {
            surfMemory = selectedSurfs;
        } else if (verb == "memory" && kind == "recall") {
            selectedSurfs = surfMemory;
        } else if (verb == "memory" && kind == "or") {
            dx::editorops::combineWithMemory(selectedSurfs, surfMemory, dx::editorops::MemoryOp::Or);
        } else if (verb == "memory" && kind == "and") {
            dx::editorops::combineWithMemory(selectedSurfs, surfMemory, dx::editorops::MemoryOp::And);
        } else if (verb == "memory" && kind == "xor") {
            dx::editorops::combineWithMemory(selectedSurfs, surfMemory, dx::editorops::MemoryOp::Xor);
        } else {
            status = "unknown surface selection " + line;
            return false;
        }
        status = "selected " + std::to_string(selectedSurfs.size()) + " surfaces";
        return true;
    }

    void Editor::texturePropertiesWindow()
    {
        if (!showTextureProperties || !propertiesTexture) {
            return;
        }
        ImGui::SetNextWindowPos(ImVec2(360, 60), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(420, 560), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Texture Properties", &showTextureProperties)) {
            ImGui::End();
            return;
        }
        dx::UTexture* texture = dx::cast<dx::UTexture>(propertiesTexture);
        std::string group = dx::editorops::textureGroup(propertiesTexture);
        ImGui::Text("Name: %s", propertiesTexture->pathName().c_str());
        ImGui::Text("Group: %s", group.empty() ? "(none)" : group.c_str());
        if (texture && texture->data) {
            dx::UPalette* palette = texture->palette();
            ImGui::Text("Size: %u x %u, %zu mips", texture->width(), texture->height(), texture->mips.size());
            ImGui::Text("Format: %s", dx::editorops::textureFormatName(texture->format()).c_str());
            ImGui::Text("Palette: %s (%zu colors)", palette ? palette->name.str().c_str() : "None", palette ? palette->colors.size() : size_t { 0 });
            dx::UObject* detail = dx::texture::DetailTexture(texture);
            dx::UObject* macro = dx::texture::MacroTexture(texture);
            ImGui::Text("Detail: %s", detail ? detail->pathName().c_str() : "None");
            ImGui::Text("Macro: %s", macro ? macro->pathName().c_str() : "None");
            ImGui::Text("Scale: %.3f", static_cast<double>(dx::texture::DrawScale(texture)));
            static const std::pair<size_t dx::texture::Offsets::*, const char*> flags[] = {
                { &dx::texture::Offsets::bInvisible, "Invisible" },
                { &dx::texture::Offsets::bMasked, "Masked" },
                { &dx::texture::Offsets::bTransparent, "Transparent" },
                { &dx::texture::Offsets::bNotSolid, "NotSolid" },
                { &dx::texture::Offsets::bEnvironment, "Environment" },
                { &dx::texture::Offsets::bSemisolid, "Semisolid" },
                { &dx::texture::Offsets::bModulate, "Modulate" },
                { &dx::texture::Offsets::bFakeBackdrop, "FakeBackdrop" },
                { &dx::texture::Offsets::bTwoSided, "TwoSided" },
                { &dx::texture::Offsets::bAutoUPan, "AutoUPan" },
                { &dx::texture::Offsets::bAutoVPan, "AutoVPan" },
                { &dx::texture::Offsets::bNoSmooth, "NoSmooth" },
                { &dx::texture::Offsets::bBigWavy, "BigWavy" },
                { &dx::texture::Offsets::bSmallWavy, "SmallWavy" },
                { &dx::texture::Offsets::bWaterWavy, "WaterWavy" },
                { &dx::texture::Offsets::bCloudWavy, "CloudWavy" },
                { &dx::texture::Offsets::bUnlit, "Unlit" },
                { &dx::texture::Offsets::bMirrored, "Mirrored" },
                { &dx::texture::Offsets::bRealtime, "Realtime" },
                { &dx::texture::Offsets::bParametric, "Parametric" },
            };
            std::string flagText;
            for (const auto& [offset, label] : flags) {
                if (*reinterpret_cast<uint32_t*>(texture->data + dx::offsets_texture.*offset)) {
                    flagText += flagText.empty() ? label : std::string(" ") + label;
                }
            }
            ImGui::TextWrapped("Flags: %s", flagText.empty() ? "None" : flagText.c_str());
            const dx::GLTextureInfo* info = thumbnails.get(texture);
            if (info) {
                float scale = std::min(1.0f, 256.0f / static_cast<float>(std::max(info->width, info->height)));
                ImGui::Image(static_cast<ImTextureID>(info->id), ImVec2(static_cast<float>(info->width) * scale, static_cast<float>(info->height) * scale));
            }
        }
        if (ImGui::Button("Use as current texture")) {
            currentTexture = propertiesTexture;
            dx::bsp::setCurrentTexture(propertiesTexture);
        }
        ImGui::SameLine();
        if (ImGui::Button("Select surfaces using it")) {
            currentTexture = propertiesTexture;
            command("texusers");
        }
        if (propertiesTexture->data && propertiesTexture->cls) {
            std::vector<uint8_t> before(propertiesTexture->data, propertiesTexture->data + propertiesTexture->cls->size);
            std::map<std::string, std::vector<dx::UProperty*>> categories;
            for (dx::UProperty* property : propertiesTexture->cls->properties) {
                if (property->propertyFlags & dx::CPF_Edit) {
                    categories[property->category.none() ? std::string("None") : property->category.str()].push_back(property);
                }
            }
            ImGui::BeginChild("textureProperties");
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
                        objectPropertyRow(propertiesTexture, property, index, false);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndChild();
            if (std::memcmp(before.data(), propertiesTexture->data, before.size()) != 0) {
                renderer->flushLevel();
                thumbnails.clear();
                status = "edited " + propertiesTexture->name.str();
            }
        }
        ImGui::End();
    }

    bool Editor::textureCommand(const std::string& verb, const std::string& rest)
    {
        dx::UModel* model = level ? level->model : nullptr;
        if (verb == "texprops") {
            if (!rest.empty() && !command("texture " + rest)) {
                return false;
            }
            if (!currentTexture) {
                return false;
            }
            propertiesTexture = currentTexture;
            showTextureProperties = true;
            return true;
        }
        if (verb == "texusers") {
            if (!model || !currentTexture) {
                return false;
            }
            selectedSurfs = dx::editorops::surfsUsingTexture(*model, currentTexture);
            status = "selected " + std::to_string(selectedSurfs.size()) + " surfaces using " + currentTexture->name.str();
            return true;
        }
        if (verb == "findtexture") {
            if (!model || selectedSurfs.empty()) {
                return false;
            }
            dx::UObject* texture = model->surfs[static_cast<size_t>(*selectedSurfs.begin())].texture;
            if (!texture) {
                status = "surface has no texture";
                return false;
            }
            dx::UObject* package = texture;
            while (package->outer) {
                package = package->outer;
            }
            if (package->name.str() != texturePackage) {
                loadTexturePackage(package->name.str());
            }
            command("texgroup " + dx::editorops::textureGroup(texture));
            currentTexture = texture;
            dx::bsp::setCurrentTexture(texture);
            scrollToTexture = true;
            pendingTab = "textures";
            status = "found " + texture->pathName();
            return true;
        }
        if (verb == "texgroup") {
            auto found = std::find(textureGroupNames.begin(), textureGroupNames.end(), rest == "(none)" ? std::string() : rest);
            textureGroupChoice = found == textureGroupNames.end() || rest == "(all)" ? -1 : static_cast<int>(found - textureGroupNames.begin());
            return true;
        }
        return false;
    }
}
