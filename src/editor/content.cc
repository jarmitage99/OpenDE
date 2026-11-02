#include "editor/editor.h"

#include "compiler/asset_package.h"
#include "compiler/exec_parse.h"
#include "engine/props_fire.h"

namespace dx::editor {
    namespace {
        std::vector<std::string> packageFiles(const char* folder, const char* extension)
        {
            std::vector<std::string> names;
            for (const std::filesystem::path& directory : { dx::objects::gameRoot() / folder, dx::config::userDirectory() / folder }) {
                std::error_code error;
                if (!std::filesystem::is_directory(directory, error)) {
                    continue;
                }
                for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
                    if (lower(entry.path().extension().string()) == extension) {
                        names.push_back(entry.path().stem().string());
                    }
                }
            }
            std::sort(names.begin(), names.end());
            names.erase(std::unique(names.begin(), names.end()), names.end());
            return names;
        }

        std::string displayName(dx::UObject* object)
        {
            std::string name = object->name.str();
            for (dx::UObject* outer = object->outer; outer && outer->outer; outer = outer->outer) {
                name = outer->name.str() + "." + name;
            }
            return name;
        }

        void addName(std::vector<std::string>& names, const std::string& name)
        {
            if (std::find(names.begin(), names.end(), name) == names.end()) {
                names.push_back(name);
                std::sort(names.begin(), names.end());
            }
        }

        template <size_t N>
        std::string field(const char* key, const std::array<char, N>& value)
        {
            if (!value[0]) {
                return {};
            }
            return std::string(" ") + key + "=\"" + value.data() + "\"";
        }

        template <size_t N>
        void setText(std::array<char, N>& target, const std::string& value)
        {
            std::snprintf(target.data(), target.size(), "%s", value.c_str());
        }

        bool packageCombo(const char* label, const std::vector<std::string>& names, const std::string& current, std::string& chosen)
        {
            bool changed = false;
            if (ImGui::BeginCombo(label, current.empty() ? "(choose)" : current.c_str())) {
                for (const std::string& name : names) {
                    if (ImGui::Selectable(name.c_str(), name == current)) {
                        chosen = name;
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            return changed;
        }

        const int textureSizes[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
    }

    std::vector<dx::UObject*> Editor::contentObjects(const std::string& packageName, const std::vector<std::string>& classNames)
    {
        std::vector<dx::UObject*> result;
        auto wanted = [&classNames](const std::string& className) {
            return std::find(classNames.begin(), classNames.end(), className) != classNames.end();
        };
        if (dx::Linker* linker = dx::objects::loadPackage(dx::Name(packageName))) {
            for (size_t i = 0; i < linker->file().exports().size(); i++) {
                int32_t ref = static_cast<int32_t>(i + 1);
                if (!wanted(linker->file().classOf(ref))) {
                    continue;
                }
                dx::UObject* object = linker->object(ref);
                if (object && !object->destroyed) {
                    result.push_back(object);
                }
            }
        }
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        for (dx::UObject* object : dx::objects::all()) {
            if (!object || object->destroyed || !object->cls || object == package || object->outermost() != package || !wanted(object->cls->name.str())) {
                continue;
            }
            if (std::find(result.begin(), result.end(), object) == result.end()) {
                result.push_back(object);
            }
        }
        std::sort(result.begin(), result.end(), [](dx::UObject* a, dx::UObject* b) {
            return lower(displayName(a)) < lower(displayName(b));
        });
        return result;
    }

    void Editor::showTexturePackage(const std::string& packageName)
    {
        addName(texturePackages, packageName);
        if (dx::objects::loadPackage(dx::Name(packageName))) {
            loadTexturePackage(packageName);
        } else {
            texturePackage = packageName;
            textureList.clear();
        }
        dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
        for (dx::UObject* object : dx::objects::all()) {
            if (object && !object->destroyed && object->cls && object->outermost() == package && object->isA(dx::Name("Texture")) && std::find(textureList.begin(), textureList.end(), object) == textureList.end()) {
                textureList.push_back(object);
            }
        }
        std::sort(textureList.begin(), textureList.end(), [](dx::UObject* a, dx::UObject* b) {
            return a->name.str() < b->name.str();
        });
        textureGroupNames = dx::editorops::textureGroups(textureList);
        textureGroupChoice = -1;
    }

    void Editor::showSoundPackage(const std::string& packageName)
    {
        if (soundPackages.empty()) {
            soundPackages = packageFiles("Sounds", ".uax");
        }
        addName(soundPackages, packageName);
        soundPackage = packageName;
        soundList = contentObjects(packageName, { "Sound" });
    }

    void Editor::showMusicPackage(const std::string& packageName)
    {
        if (musicPackages.empty()) {
            musicPackages = packageFiles("Music", ".umx");
        }
        addName(musicPackages, packageName);
        musicPackage = packageName;
        musicList = contentObjects(packageName, { "Music" });
        currentMusic = musicList.empty() ? nullptr : musicList.front();
    }

    void Editor::showMeshPackage(const std::string& packageName)
    {
        if (meshPackages.empty()) {
            meshPackages = packageFiles("System", ".u");
        }
        addName(meshPackages, packageName);
        meshPackage = packageName;
        meshList = contentObjects(packageName, { "Mesh", "LodMesh" });
    }

    void Editor::viewMesh(dx::UObject* object)
    {
        dx::objects::ensureLoaded(object);
        dx::objects::flushLoads();
        currentMesh = dx::cast<dx::UMesh>(object);
        if (!currentMesh) {
            return;
        }
        meshSkins.clear();
        for (dx::UObject* candidate : dx::objects::all()) {
            dx::UClass* cls = dx::cast<dx::UClass>(candidate);
            if (!cls || cls->destroyed || !cls->defaultObject || !actorClass || !cls->isChildOf(actorClass) || dx::actor::Mesh(cls->defaultObject) != currentMesh) {
                continue;
            }
            for (int i = 0; i < 8; i++) {
                dx::UObject* skin = dx::actor::MultiSkins(cls->defaultObject, i);
                meshSkins.push_back(skin ? skin : dx::actor::Skin(cls->defaultObject));
            }
            break;
        }
        meshSequence = currentMesh->sequences.empty() ? dx::Name() : currentMesh->sequences.front().name;
        meshFrame = 0;
        meshYaw = 0;
        meshPitch = 0.2f;
        meshZoom = 1;
        showMeshViewer = true;
    }

    bool Editor::ensureAudio()
    {
        if (!audioReady) {
            audioReady = dx::audio::initialize();
        }
        return audioReady;
    }

    void Editor::contentPackagesMenu()
    {
        std::vector<std::string> names = contentModified;
        for (const std::string& name : { texturePackage, soundPackage, musicPackage, meshPackage }) {
            if (!name.empty() && std::find(names.begin(), names.end(), name) == names.end()) {
                names.push_back(name);
            }
        }
        if (!ImGui::BeginMenu("Save Content Package", !names.empty())) {
            return;
        }
        for (const std::string& name : names) {
            bool changed = std::find(contentModified.begin(), contentModified.end(), name) != contentModified.end();
            if (ImGui::MenuItem(name.c_str(), changed ? "modified" : nullptr)) {
                pendingCommand = "savecontent " + name;
            }
        }
        ImGui::EndMenu();
    }

    void Editor::openContentDialog(const char* name)
    {
        if (contentDialog == name) {
            ImGui::OpenPopup(name);
            contentDialog.clear();
        }
    }

    void Editor::textureContentTools()
    {
        if (ImGui::Button("Import...")) {
            contentDialog = "Import Texture";
            setText(importPackage, texturePackage.empty() ? std::string("MyTextures") : texturePackage);
        }
        ImGui::SameLine();
        if (ImGui::Button("New...")) {
            contentDialog = "New Texture";
            setText(importPackage, texturePackage.empty() ? std::string("MyTextures") : texturePackage);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Package") && !texturePackage.empty()) {
            pendingCommand = "savecontent " + texturePackage;
        }
        openContentDialog("Import Texture");
        if (ImGui::BeginPopup("Import Texture")) {
            ImGui::InputText("File (.pcx/.bmp)", importFile.data(), importFile.size());
            ImGui::InputText("Package", importPackage.data(), importPackage.size());
            ImGui::InputText("Group", importGroup.data(), importGroup.size());
            ImGui::InputText("Name", importName.data(), importName.size());
            ImGui::Checkbox("Generate mipmaps", &importMips);
            ImGui::SameLine();
            ImGui::Checkbox("Masked", &importMasked);
            ImGui::InputText("Detail texture", importDetail.data(), importDetail.size());
            if (currentTexture && ImGui::Button("Use current as detail")) {
                setText(importDetail, currentTexture->pathName());
            }
            if (ImGui::Button("Import") && importFile[0] && importPackage[0]) {
                std::string line = "textureimport" + field("FILE", importFile) + field("PACKAGE", importPackage) + field("GROUP", importGroup) + field("NAME", importName);
                line += std::string(" MIPS=") + (importMips ? "1" : "0");
                if (importMasked) {
                    line += " FLAGS=2";
                }
                line += field("DETAIL", importDetail);
                pendingCommand = line;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        openContentDialog("New Texture");
        if (ImGui::BeginPopup("New Texture")) {
            if (textureClasses.empty()) {
                dx::objects::findClass("Fire.FireTexture");
                dx::UClass* base = dx::objects::findClass("Engine.Texture");
                for (dx::UObject* object : dx::objects::all()) {
                    dx::UClass* cls = dx::cast<dx::UClass>(object);
                    if (cls && !cls->destroyed && base && cls->isChildOf(base) && !(cls->classFlags & dx::CLASS_Abstract)) {
                        textureClasses.push_back(cls);
                    }
                }
                std::sort(textureClasses.begin(), textureClasses.end(), [](dx::UClass* a, dx::UClass* b) {
                    return a->name.str() < b->name.str();
                });
                for (size_t i = 0; i < textureClasses.size(); i++) {
                    if (textureClasses[i] == base) {
                        newTextureClass = static_cast<int>(i);
                    }
                }
            }
            ImGui::InputText("Package", importPackage.data(), importPackage.size());
            ImGui::InputText("Group", importGroup.data(), importGroup.size());
            ImGui::InputText("Name", importName.data(), importName.size());
            dx::UClass* chosen = newTextureClass >= 0 && static_cast<size_t>(newTextureClass) < textureClasses.size() ? textureClasses[static_cast<size_t>(newTextureClass)] : nullptr;
            if (ImGui::BeginCombo("Class", chosen ? chosen->name.str().c_str() : "(choose)")) {
                for (size_t i = 0; i < textureClasses.size(); i++) {
                    if (ImGui::Selectable(textureClasses[i]->name.str().c_str(), static_cast<int>(i) == newTextureClass)) {
                        newTextureClass = static_cast<int>(i);
                    }
                }
                ImGui::EndCombo();
            }
            for (int axis = 0; axis < 2; axis++) {
                int& choice = axis == 0 ? newUSize : newVSize;
                std::string current = std::to_string(textureSizes[choice]);
                if (ImGui::BeginCombo(axis == 0 ? "Width" : "Height", current.c_str())) {
                    for (int i = 0; i < static_cast<int>(std::size(textureSizes)); i++) {
                        if (ImGui::Selectable(std::to_string(textureSizes[i]).c_str(), i == choice)) {
                            choice = i;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            if (ImGui::Button("Create") && chosen && importPackage[0] && importName[0]) {
                pendingCommand = "texturenew" + field("NAME", importName) + " CLASS=" + chosen->pathName() + field("PACKAGE", importPackage) + field("GROUP", importGroup) + " USIZE=" + std::to_string(textureSizes[newUSize]) + " VSIZE=" + std::to_string(textureSizes[newVSize]);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        fractalPaintPanel();
    }

    bool Editor::paintFractal(int32_t x, int32_t y, bool left, bool right)
    {
        dx::UTexture* texture = dx::cast<dx::UTexture>(currentTexture);
        if (!texture || !dx::fractalpaint::paintable(texture)) {
            return false;
        }
        dx::UFireTexture* fire = dx::cast<dx::UFireTexture>(texture);
        int32_t before = fire ? dx::firetexture::NumSparks(fire) : dx::watertexture::NumDrops(texture);
        std::vector<dx::FireSpark> sparks = fire ? fire->sparks : std::vector<dx::FireSpark>();
        dx::fractalpaint::paint(texture, paintState, x, y, left, right);
        int32_t after = fire ? dx::firetexture::NumSparks(fire) : dx::watertexture::NumDrops(texture);
        bool changed = before != after || (fire && sparks.size() == fire->sparks.size() && !std::equal(sparks.begin(), sparks.end(), fire->sparks.begin(), [](const dx::FireSpark& a, const dx::FireSpark& b) {
            return std::memcmp(&a, &b, sizeof(a)) == 0;
        }));
        if (changed) {
            thumbnails.resetProcedural(texture);
            addName(contentModified, texture->outermost()->name.str());
        }
        return changed;
    }

    void Editor::fractalPaintPanel()
    {
        dx::UTexture* texture = dx::cast<dx::UTexture>(currentTexture);
        if (!texture || !dx::fractalpaint::paintable(texture) || !ImGui::CollapsingHeader("Procedural Paint", ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }
        dx::UFireTexture* fire = dx::cast<dx::UFireTexture>(texture);
        dx::UProperty* typeProperty = texture->cls->findProperty(dx::Name(fire ? "SparkType" : "DropType"));
        uint8_t& type = fire ? dx::firetexture::SparkType(texture) : dx::watertexture::DropType(texture);
        if (typeProperty && typeProperty->enumType) {
            const std::vector<dx::Name>& names = typeProperty->enumType->names;
            if (ImGui::BeginCombo(fire ? "Spark" : "Drop", type < names.size() ? names[type].str().c_str() : "?")) {
                for (size_t i = 0; i < names.size(); i++) {
                    if (ImGui::Selectable(names[i].str().c_str(), i == type)) {
                        type = static_cast<uint8_t>(i);
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::Text("%d %s, left paints, right erases", fire ? dx::firetexture::NumSparks(texture) : dx::watertexture::NumDrops(texture), fire ? "sparks" : "drops");
        const dx::GLTextureInfo* info = thumbnails.get(texture);
        float side = std::min(ImGui::GetContentRegionAvail().x, 256.0f);
        float width = static_cast<float>(std::max(texture->width(), 1u));
        float height = static_cast<float>(std::max(texture->height(), 1u));
        ImVec2 size(side, side * height / width);
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(0, 0, 0, 255));
        if (info) {
            ImGui::Image(static_cast<ImTextureID>(info->id), size);
        } else {
            ImGui::Dummy(size);
        }
        if (ImGui::IsItemHovered()) {
            ImVec2 mouse = ImGui::GetIO().MousePos;
            int32_t x = static_cast<int32_t>((mouse.x - origin.x) / size.x * width);
            int32_t y = static_cast<int32_t>((mouse.y - origin.y) / size.y * height);
            paintFractal(x, y, ImGui::IsMouseDown(ImGuiMouseButton_Left), ImGui::IsMouseDown(ImGuiMouseButton_Right));
        }
    }

    void Editor::soundBrowser()
    {
        if (soundPackages.empty()) {
            soundPackages = packageFiles("Sounds", ".uax");
        }
        std::string chosen;
        if (packageCombo("Package", soundPackages, soundPackage, chosen)) {
            showSoundPackage(chosen);
        }
        if (ImGui::Button("Import...")) {
            contentDialog = "Import Sound";
            setText(importPackage, soundPackage.empty() ? std::string("MySounds") : soundPackage);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Package") && !soundPackage.empty()) {
            pendingCommand = "savecontent " + soundPackage;
        }
        openContentDialog("Import Sound");
        if (ImGui::BeginPopup("Import Sound")) {
            ImGui::InputText("File (.wav)", importFile.data(), importFile.size());
            ImGui::InputText("Package", importPackage.data(), importPackage.size());
            ImGui::InputText("Group", importGroup.data(), importGroup.size());
            ImGui::InputText("Name", importName.data(), importName.size());
            if (ImGui::Button("Import") && importFile[0] && importPackage[0]) {
                pendingCommand = "audioimport" + field("FILE", importFile) + field("PACKAGE", importPackage) + field("GROUP", importGroup) + field("NAME", importName);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        dx::USound* sound = dx::cast<dx::USound>(currentSound);
        if (sound) {
            ImGui::Text("%s", displayName(sound).c_str());
            ImGui::Text("%s, %zu bytes, %.2f s", sound->format.str().c_str(), sound->bytes.size(), static_cast<double>(dx::audio::duration(sound)));
            if (ImGui::Button("Play")) {
                command("playsound " + sound->pathName());
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                command("stopsound");
            }
        }
        ImGui::BeginChild("sounds");
        for (dx::UObject* object : soundList) {
            ImGui::PushID(object);
            if (ImGui::Selectable(displayName(object).c_str(), object == currentSound, ImGuiSelectableFlags_AllowDoubleClick)) {
                dx::objects::ensureLoaded(object);
                dx::objects::flushLoads();
                currentSound = object;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    command("playsound " + object->pathName());
                }
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }

    void Editor::musicBrowser()
    {
        if (musicPackages.empty()) {
            musicPackages = packageFiles("Music", ".umx");
        }
        std::string chosen;
        if (packageCombo("Package", musicPackages, musicPackage, chosen)) {
            showMusicPackage(chosen);
        }
        if (currentMusic) {
            ImGui::Text("%s", displayName(currentMusic).c_str());
            if (ImGui::Button("Play")) {
                command("playmusic " + currentMusic->pathName());
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                command("stopmusic");
            }
            dx::audio::MusicStatus playing = dx::audio::musicStatus();
            if (playing.music) {
                ImGui::Text("Playing %s order %d row %d", playing.music->name.str().c_str(), playing.order, playing.row);
            }
        }
        ImGui::BeginChild("music");
        for (const std::string& name : musicPackages) {
            if (ImGui::Selectable(name.c_str(), name == musicPackage, ImGuiSelectableFlags_AllowDoubleClick)) {
                showMusicPackage(name);
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && currentMusic) {
                    command("playmusic " + currentMusic->pathName());
                }
            }
        }
        ImGui::EndChild();
    }

    void Editor::meshBrowser()
    {
        if (meshPackages.empty()) {
            meshPackages = packageFiles("System", ".u");
        }
        std::string chosen;
        if (packageCombo("Package", meshPackages, meshPackage, chosen)) {
            showMeshPackage(chosen);
        }
        if (ImGui::Button("Import...")) {
            contentDialog = "Import Mesh";
            setText(importPackage, meshPackage.empty() ? std::string("MyMeshes") : meshPackage);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Package") && !meshPackage.empty()) {
            pendingCommand = "savecontent " + meshPackage;
        }
        if (currentMesh) {
            ImGui::SameLine();
            if (ImGui::Button("View")) {
                showMeshViewer = true;
            }
        }
        openContentDialog("Import Mesh");
        if (ImGui::BeginPopup("Import Mesh")) {
            ImGui::InputText("Name", importName.data(), importName.size());
            ImGui::InputText("Package", importPackage.data(), importPackage.size());
            ImGui::InputText("Anim file (_a.3d)", importFile.data(), importFile.size());
            ImGui::InputText("Data file (_d.3d)", importData.data(), importData.size());
            ImGui::Checkbox("Level of detail", &importLod);
            ImGui::InputFloat3("Scale", importScale.data());
            if (ImGui::Button("Import") && importFile[0] && importData[0] && importName[0] && importPackage[0]) {
                char scale[96];
                std::snprintf(scale, sizeof(scale), " X=%g Y=%g Z=%g", static_cast<double>(importScale[0]), static_cast<double>(importScale[1]), static_cast<double>(importScale[2]));
                pendingCommand = "meshimport" + field("MESH", importName) + field("PACKAGE", importPackage) + field("ANIVFILE", importFile) + field("DATAFILE", importData) + (importLod ? " MLOD=1" : " MLOD=0") + scale;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::BeginChild("meshes");
        for (dx::UObject* object : meshList) {
            ImGui::PushID(object);
            if (ImGui::Selectable(displayName(object).c_str(), object == currentMesh)) {
                viewMesh(object);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
    }

    void Editor::drawMesh(ImDrawList* list, const ImVec2& origin, const ImVec2& size)
    {
        dx::UMesh* mesh = currentMesh;
        const dx::AnimSequence* sequence = mesh->findSequence(meshSequence);
        std::vector<dx::Vector> pose;
        mesh->framePose(sequence, meshFrame, pose);
        if (pose.empty()) {
            return;
        }
        dx::Vector low = pose.front();
        dx::Vector high = pose.front();
        for (const dx::Vector& point : pose) {
            low = { std::min(low.x, point.x), std::min(low.y, point.y), std::min(low.z, point.z) };
            high = { std::max(high.x, point.x), std::max(high.y, point.y), std::max(high.z, point.z) };
        }
        dx::Vector center = (low + high) * 0.5f;
        dx::Vector extent = (high - low) * 0.5f;
        float radius = std::max(1.0f, std::sqrt(extent.x * extent.x + extent.y * extent.y + extent.z * extent.z));
        float scale = 0.48f * std::min(size.x, size.y) / radius * meshZoom;
        float cy = std::cos(meshYaw);
        float sy = std::sin(meshYaw);
        float cp = std::cos(meshPitch);
        float sp = std::sin(meshPitch);
        std::vector<dx::Vector> view(pose.size());
        for (size_t i = 0; i < pose.size(); i++) {
            dx::Vector d = pose[i] - center;
            float x = d.x * cy + d.y * sy;
            float y = -d.x * sy + d.y * cy;
            view[i] = { x * cp + d.z * sp, y, -x * sp + d.z * cp };
        }
        ImVec2 middle(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);
        auto screen = [&](const dx::Vector& p) {
            return ImVec2(middle.x - p.y * scale, middle.y - p.z * scale);
        };
        std::vector<dx::MeshTriangle> triangles = mesh->triangles();
        std::vector<std::pair<float, size_t>> order;
        for (size_t i = 0; i < triangles.size(); i++) {
            const dx::MeshTriangle& triangle = triangles[i];
            bool valid = (triangle.polyFlags & dx::bsp::PF_Invisible) == 0;
            for (int32_t vertex : triangle.vertex) {
                valid = valid && vertex >= 0 && static_cast<size_t>(vertex) < view.size();
            }
            if (valid) {
                float depth = view[static_cast<size_t>(triangle.vertex[0])].x + view[static_cast<size_t>(triangle.vertex[1])].x + view[static_cast<size_t>(triangle.vertex[2])].x;
                order.emplace_back(depth, i);
            }
        }
        std::sort(order.begin(), order.end());
        for (const auto& entry : order) {
            const dx::MeshTriangle& triangle = triangles[entry.second];
            const dx::Vector& a = view[static_cast<size_t>(triangle.vertex[0])];
            const dx::Vector& b = view[static_cast<size_t>(triangle.vertex[1])];
            const dx::Vector& c = view[static_cast<size_t>(triangle.vertex[2])];
            dx::Vector normal = (b - a).cross(c - a);
            float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            float shade = 0.35f + 0.65f * (length > 0 ? std::fabs(normal.x) / length : 0.0f);
            uint32_t bright = static_cast<uint32_t>(std::clamp(shade, 0.0f, 1.0f) * 255.0f);
            ImU32 color = IM_COL32(bright, bright, bright, 255);
            dx::UTexture* skin = nullptr;
            if (triangle.textureIndex >= 0 && static_cast<size_t>(triangle.textureIndex) < mesh->textures.size()) {
                skin = dx::cast<dx::UTexture>(mesh->textures[static_cast<size_t>(triangle.textureIndex)]);
            }
            if (!skin && triangle.textureIndex >= 0 && static_cast<size_t>(triangle.textureIndex) < meshSkins.size()) {
                dx::objects::ensureLoaded(meshSkins[static_cast<size_t>(triangle.textureIndex)]);
                skin = dx::cast<dx::UTexture>(meshSkins[static_cast<size_t>(triangle.textureIndex)]);
            }
            const dx::GLTextureInfo* info = skin && !meshWire ? thumbnails.get(skin) : nullptr;
            ImVec2 points[3] = { screen(a), screen(b), screen(c) };
            if (meshWire) {
                list->AddTriangle(points[0], points[1], points[2], IM_COL32(120, 200, 255, 255));
                continue;
            }
            float width = info ? static_cast<float>(std::max(info->width, 1u)) : 256.0f;
            float height = info ? static_cast<float>(std::max(info->height, 1u)) : 256.0f;
            list->PushTextureID(static_cast<ImTextureID>(info ? info->id : thumbnails.white()));
            list->PrimReserve(3, 3);
            for (int k = 0; k < 3; k++) {
                list->PrimVtx(points[k], ImVec2(triangle.u[k] / width, triangle.v[k] / height), info ? color : IM_COL32(bright, bright * 3 / 4, bright / 2, 255));
            }
            list->PopTextureID();
        }
    }

    void Editor::meshViewerWindow()
    {
        if (!showMeshViewer || !currentMesh) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(680, 520), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(360, 120), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Mesh Viewer", &showMeshViewer)) {
            ImGui::End();
            return;
        }
        dx::UMesh* mesh = currentMesh;
        ImGui::Text("%s  %s  %d verts  %d frames  %zu triangles", mesh->pathName().c_str(), mesh->cls->name.str().c_str(), mesh->frameVerts, mesh->animFrames, mesh->triangles().size());
        ImGui::BeginChild("sequences", ImVec2(180, 0), true);
        for (const dx::AnimSequence& sequence : mesh->sequences) {
            std::string label = sequence.name.str() + " (" + std::to_string(sequence.numFrames) + ")";
            if (ImGui::Selectable(label.c_str(), sequence.name == meshSequence)) {
                meshSequence = sequence.name;
                meshFrame = 0;
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("meshCanvas");
        const dx::AnimSequence* sequence = mesh->findSequence(meshSequence);
        if (ImGui::Button(meshPlaying ? "Pause" : "Play")) {
            meshPlaying = !meshPlaying;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Wireframe", &meshWire);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(180);
        ImGui::SliderFloat("Position", &meshFrame, 0.0f, 0.999f);
        if (sequence) {
            ImGui::SameLine();
            int frames = std::max(sequence->numFrames, 1);
            ImGui::Text("Frame %d/%d  Rate %.1f", static_cast<int>(meshFrame * static_cast<float>(frames)), frames, static_cast<double>(sequence->rate));
        }
        if (meshPlaying && sequence && sequence->numFrames > 0) {
            meshFrame += ImGui::GetIO().DeltaTime * sequence->rate / static_cast<float>(sequence->numFrames);
            meshFrame -= std::floor(meshFrame);
        }
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        size.x = std::max(size.x, 32.0f);
        size.y = std::max(size.y, 32.0f);
        ImGui::InvisibleButton("meshView", size);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            meshYaw += delta.x * 0.01f;
            meshPitch = std::clamp(meshPitch + delta.y * 0.01f, -1.5f, 1.5f);
        }
        if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
            meshZoom = std::clamp(meshZoom * (ImGui::GetIO().MouseWheel > 0 ? 1.1f : 1.0f / 1.1f), 0.1f, 20.0f);
        }
        ImDrawList* list = ImGui::GetWindowDrawList();
        list->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
        list->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), IM_COL32(32, 32, 40, 255));
        drawMesh(list, origin, size);
        list->PopClipRect();
        ImGui::EndChild();
        ImGui::End();
    }

    bool Editor::runContentImport(const std::string& rest, const std::string& execCommand, std::string& packageName)
    {
        if (!dx::compiler::parseString(rest, "PACKAGE=", packageName) || packageName.empty()) {
            status = "missing PACKAGE=";
            return false;
        }
        std::vector<std::string> messages;
        std::error_code error;
        std::filesystem::path directory = std::filesystem::current_path(error);
        bool ok = dx::compiler::runAssetCommand(dx::objects::packageObject(dx::Name(packageName)), directory, execCommand, messages);
        for (const std::string& line : messages) {
            std::fprintf(stderr, "%s\n", line.c_str());
        }
        if (ok) {
            addName(contentModified, packageName);
        }
        status = ok ? execCommand.substr(0, execCommand.find(' ', execCommand.find(' ') + 1)) + " into " + packageName : (messages.empty() ? std::string("import failed") : messages.front());
        return ok;
    }

    bool Editor::contentCommand(const std::string& verb, const std::string& rest, bool& result)
    {
        std::string packageName;
        if (verb == "textureimport" || verb == "texturenew") {
            result = runContentImport(rest, (verb == "textureimport" ? "TEXTURE IMPORT " : "TEXTURE NEW ") + rest, packageName);
            if (result) {
                showTexturePackage(packageName);
                pendingTab = "textures";
                std::string name;
                std::string file;
                if (!dx::compiler::parseString(rest, "NAME=", name) && dx::compiler::parseString(rest, "FILE=", file)) {
                    name = std::filesystem::path(file).stem().string();
                }
                for (dx::UObject* texture : textureList) {
                    if (lower(texture->name.str()) == lower(name)) {
                        currentTexture = texture;
                        dx::bsp::setCurrentTexture(texture);
                        if (verb == "texturenew") {
                            propertiesTexture = texture;
                            showTextureProperties = true;
                        }
                    }
                }
            }
            return true;
        }
        if (verb == "audioimport") {
            result = runContentImport(rest, "AUDIO IMPORT " + rest, packageName);
            if (result) {
                showSoundPackage(packageName);
                pendingTab = "sounds";
            }
            return true;
        }
        if (verb == "meshimport") {
            std::string meshName;
            dx::compiler::parseString(rest, "MESH=", meshName);
            result = runContentImport(rest, "MESH IMPORT " + rest, packageName);
            dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
            dx::UMesh* mesh = nullptr;
            for (dx::UObject* object : result ? contentObjects(packageName, { "Mesh", "LodMesh" }) : std::vector<dx::UObject*>()) {
                if (lower(object->name.str()) == lower(meshName)) {
                    mesh = dx::cast<dx::UMesh>(object);
                }
            }
            if (mesh) {
                std::vector<std::string> messages;
                if (mesh->sequences.empty()) {
                    dx::compiler::runAssetCommand(package, {}, "MESH SEQUENCE MESH=" + meshName + " SEQ=All STARTFRAME=0 NUMFRAMES=" + std::to_string(std::max(mesh->animFrames, 1)), messages);
                }
                float x = 1;
                if (dx::compiler::parseFloat(rest, "X=", x)) {
                    std::string scale = rest.substr(rest.find("X="));
                    dx::compiler::runAssetCommand(package, {}, "MESHMAP SCALE MESHMAP=" + meshName + " " + scale, messages);
                }
                showMeshPackage(packageName);
                pendingTab = "meshes";
                viewMesh(mesh);
            }
            return true;
        }
        if (verb == "assetexec") {
            std::istringstream fields(rest);
            fields >> packageName;
            std::string line;
            std::getline(fields, line);
            result = runContentImport("PACKAGE=" + packageName, line, packageName);
            return true;
        }
        if (verb == "savecontent") {
            std::istringstream fields(rest);
            std::string target;
            fields >> packageName >> target;
            dx::UObject* package = dx::objects::packageObject(dx::Name(packageName));
            std::filesystem::path file = target.empty() ? dx::compiler::assetPackagePath(package) : std::filesystem::path(target);
            std::string error;
            result = dx::compiler::saveAssetPackage(package, file, &error);
            status = result ? "saved " + file.string() : "save failed: " + error;
            std::fprintf(stderr, "%s\n", status.c_str());
            if (result) {
                contentModified.erase(std::remove(contentModified.begin(), contentModified.end(), packageName), contentModified.end());
            }
            return true;
        }
        if (verb == "browsetextures" || verb == "browsesounds" || verb == "browsemusic" || verb == "browsemeshes") {
            if (verb == "browsetextures") {
                showTexturePackage(rest);
                pendingTab = "textures";
            } else if (verb == "browsesounds") {
                showSoundPackage(rest);
                pendingTab = "sounds";
            } else if (verb == "browsemusic") {
                showMusicPackage(rest);
                pendingTab = "music";
            } else {
                showMeshPackage(rest);
                pendingTab = "meshes";
            }
            result = true;
            return true;
        }
        if (verb == "playsound") {
            dx::USound* sound = dx::cast<dx::USound>(dx::objects::loadObject(rest, nullptr));
            dx::objects::flushLoads();
            if (sound) {
                currentSound = sound;
            }
            if (sound && soundVoice) {
                dx::audio::stopSound(soundVoice);
            }
            soundVoice = sound && ensureAudio() ? dx::audio::playSound(sound, nullptr, dx::audio::SlotNone, cameraLocation, 1.0f, 4096.0f, 1.0f, false) : 0;
            result = soundVoice != 0;
            status = result ? "playing " + sound->name.str() : "cannot play " + rest;
            return true;
        }
        if (verb == "stopsound") {
            if (soundVoice) {
                dx::audio::stopSound(soundVoice);
            }
            soundVoice = 0;
            result = true;
            return true;
        }
        if (verb == "playmusic") {
            dx::UMusic* music = dx::cast<dx::UMusic>(dx::objects::loadObject(rest, nullptr));
            dx::objects::flushLoads();
            result = music && ensureAudio();
            if (result) {
                currentMusic = music;
                dx::audio::playMusic(music, 0, 0);
            }
            status = result ? "playing " + music->name.str() : "cannot play " + rest;
            return true;
        }
        if (verb == "stopmusic") {
            if (audioReady) {
                dx::audio::playMusic(nullptr, 0, 0);
            }
            result = true;
            return true;
        }
        if (verb == "showmesh") {
            dx::UObject* object = dx::objects::loadObject(rest, nullptr);
            dx::objects::flushLoads();
            viewMesh(object);
            result = currentMesh != nullptr && currentMesh == object;
            return true;
        }
        if (verb == "meshseq" || verb == "meshframe" || verb == "meshplay" || verb == "meshview") {
            std::istringstream values(rest);
            if (verb == "meshseq") {
                meshSequence = dx::Name(rest);
                meshFrame = 0;
            } else if (verb == "meshframe") {
                values >> meshFrame;
            } else if (verb == "meshplay") {
                meshPlaying = rest != "0";
            } else {
                values >> meshYaw >> meshPitch >> meshZoom;
            }
            result = currentMesh != nullptr;
            return true;
        }
        if (verb == "paintfractal") {
            std::istringstream values(rest);
            int32_t x = 0;
            int32_t y = 0;
            int32_t toX = -1;
            int32_t toY = -1;
            std::string button;
            values >> x >> y >> button >> toX >> toY;
            bool right = button == "right";
            result = paintFractal(x, y, !right, right);
            if (toX >= 0 && toY >= 0) {
                result = paintFractal(toX, toY, !right, right) || result;
            }
            paintFractal(toX >= 0 ? toX : x, toY >= 0 ? toY : y, false, false);
            return true;
        }
        if (verb == "contentdialog") {
            const std::pair<const char*, const char*> dialogs[] = { { "textureimport", "Import Texture" }, { "texturenew", "New Texture" }, { "soundimport", "Import Sound" }, { "meshimport", "Import Mesh" } };
            for (const auto& [key, title] : dialogs) {
                if (rest == key) {
                    contentDialog = title;
                    pendingTab = rest == "soundimport" ? "sounds" : rest == "meshimport" ? "meshes"
                                                                                         : "textures";
                    const std::string& current = rest == "soundimport" ? soundPackage : rest == "meshimport" ? meshPackage
                                                                                                             : texturePackage;
                    setText(importPackage, current.empty() ? std::string("MyPackage") : current);
                }
            }
            result = !contentDialog.empty();
            return true;
        }
        return false;
    }
}
