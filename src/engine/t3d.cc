#include "engine/t3d.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/bsp.h"
#include "engine/csg.h"
#include "engine/editor_ops.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unordered_map>

namespace dx {
    namespace t3d {
        namespace {
            constexpr uint32_t ImportFlags = RF_Transactional | 0x00070000;

            struct Record {
                UClass* cls = nullptr;
                std::vector<std::string> lines;
                std::string name;
                std::string brushName;
                std::string polyText;
                bool hasBrush = false;
            };

            std::string lower(std::string_view text)
            {
                std::string result(text);
                for (char& c : result) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                return result;
            }

            std::string_view trim(std::string_view text)
            {
                while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
                    text.remove_prefix(1);
                }
                while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
                    text.remove_suffix(1);
                }
                return text;
            }

            bool word(std::string_view& line, std::string_view expected)
            {
                std::string_view text = trim(line);
                if (text.size() < expected.size() || lower(text.substr(0, expected.size())) != lower(expected)) {
                    return false;
                }
                if (text.size() > expected.size() && !std::isspace(static_cast<unsigned char>(text[expected.size()]))) {
                    return false;
                }
                line = trim(text.substr(expected.size()));
                return true;
            }

            std::string field(std::string_view line, std::string_view key)
            {
                std::string text = lower(line);
                size_t found = text.find(lower(key));
                while (found != std::string::npos && found > 0 && !std::isspace(static_cast<unsigned char>(text[found - 1]))) {
                    found = text.find(lower(key), found + 1);
                }
                if (found == std::string::npos) {
                    return {};
                }
                size_t start = found + key.size();
                size_t end = start;
                while (end < line.size() && !std::isspace(static_cast<unsigned char>(line[end]))) {
                    end++;
                }
                return std::string(line.substr(start, end - start));
            }

            bool ported(const UProperty* property)
            {
                if ((property->propertyFlags & (CPF_Transient | CPF_Native)) && property->category.none()) {
                    return false;
                }
                if (property->name == Name("Name") || property->name == Name("Class")) {
                    return false;
                }
                return property->type != PropType::Array && property->type != PropType::Map && property->type != PropType::FixedArray && property->type != PropType::Pointer;
            }

            UModel* exportedModel(const UProperty* property, const uint8_t* memory)
            {
                if (property->type != PropType::Object || !(property->propertyFlags & CPF_ExportObject)) {
                    return nullptr;
                }
                UModel* model = cast<UModel>(*reinterpret_cast<UObject* const*>(memory));
                if (!model) {
                    return nullptr;
                }
                objects::ensureLoaded(model);
                objects::flushLoads();
                if (model->polys) {
                    objects::ensureLoaded(model->polys);
                    objects::flushLoads();
                }
                return model->polys ? model : nullptr;
            }

            void exportBrush(std::string& out, const UModel& model)
            {
                out += "    Begin Brush Name=" + model.name.str() + "\r\n";
                std::string polys = editorops::exportPolys(*model.polys);
                size_t start = 0;
                while (start < polys.size()) {
                    size_t end = polys.find('\n', start);
                    end = end == std::string::npos ? polys.size() : end + 1;
                    out += "       " + polys.substr(start, end - start);
                    start = end;
                }
                out += "    End Brush\r\n";
            }

            void exportActor(std::string& out, UObject* actor)
            {
                out += "Begin Actor Class=" + actor->cls->name.str() + " Name=" + actor->name.str() + "\r\n";
                const uint8_t* defaults = actor->cls->defaultObject ? actor->cls->defaultObject->data : nullptr;
                std::vector<UProperty*> properties = actor->cls->fieldOrder();
                for (size_t p = 0; p < properties.size(); p++) {
                    UProperty* property = properties[p];
                    if (!ported(property)) {
                        continue;
                    }
                    bool shadowed = false;
                    for (size_t earlier = 0; earlier < p; earlier++) {
                        if (properties[earlier]->name == property->name) {
                            shadowed = true;
                        }
                    }
                    if (shadowed) {
                        continue;
                    }
                    for (int32_t index = 0; index < property->arrayDim; index++) {
                        size_t at = property->offset + static_cast<size_t>(index) * property->elementSize();
                        if (defaults && property->equals(actor->data + at, defaults + at)) {
                            continue;
                        }
                        if (UModel* model = exportedModel(property, actor->data + at)) {
                            exportBrush(out, *model);
                        }
                        std::string value = property->toText(actor->data + at);
                        if (property->type == PropType::Str || property->type == PropType::String) {
                            value = "\"" + value + "\"";
                        }
                        out += "    " + property->name.str();
                        if (property->arrayDim > 1) {
                            out += "(" + std::to_string(index) + ")";
                        }
                        out += "=" + value + "\r\n";
                    }
                }
                out += "End Actor\r\n";
            }

            std::vector<Record> parse(std::string_view text)
            {
                std::vector<Record> records;
                bool inActor = false;
                bool inBrush = false;
                bool inPolys = false;
                while (!text.empty()) {
                    size_t end = text.find('\n');
                    std::string_view line = trim(text.substr(0, end));
                    text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
                    std::string_view rest = line;
                    if (!inActor) {
                        if (word(rest, "Begin") && word(rest, "Actor")) {
                            records.emplace_back();
                            records.back().cls = objects::findClass(field(rest, "Class="));
                            records.back().name = field(rest, "Name=");
                            inActor = true;
                        }
                        continue;
                    }
                    Record& record = records.back();
                    if (inPolys) {
                        record.polyText += std::string(line) + "\n";
                        if (word(rest, "End") && word(rest, "PolyList")) {
                            inPolys = false;
                        }
                        continue;
                    }
                    if (inBrush) {
                        if (word(rest, "Begin") && word(rest, "PolyList")) {
                            inPolys = true;
                        }
                        rest = line;
                        if (word(rest, "End") && word(rest, "Brush")) {
                            inBrush = false;
                        }
                        continue;
                    }
                    if (word(rest, "Begin") && word(rest, "Brush")) {
                        inBrush = true;
                        record.hasBrush = true;
                        record.brushName = field(rest, "Name=");
                        continue;
                    }
                    rest = line;
                    if (word(rest, "End") && word(rest, "Actor")) {
                        inActor = false;
                        continue;
                    }
                    record.lines.emplace_back(line);
                }
                return records;
            }

            UObject* reference(const std::string& value, ULevel* level, const std::unordered_map<std::string, UObject*>& remap, bool searchLevel)
            {
                size_t quote = value.find('\'');
                std::string path = quote == std::string::npos ? value : value.substr(quote + 1);
                if (!path.empty() && path.back() == '\'') {
                    path.pop_back();
                }
                size_t dot = path.rfind('.');
                std::string objectName = lower(dot == std::string::npos ? path : path.substr(dot + 1));
                auto found = remap.find(objectName);
                if (found != remap.end()) {
                    return found->second;
                }
                if (searchLevel) {
                    for (UObject* actor : level->actors) {
                        if (actor && !actor->destroyed && lower(actor->name.str()) == objectName) {
                            return actor;
                        }
                    }
                }
                return nullptr;
            }

            bool importedSeparately(const UProperty* property)
            {
                return property->name == Name("Name") || property->name == Name("Level") || property->name == Name("XLevel") || property->name == Name("Region") || property->name == Name("Brush");
            }

            void applyProperties(UObject* actor, const Record& record, ULevel* level, const std::unordered_map<std::string, UObject*>& remap, bool searchLevel)
            {
                for (const std::string& line : record.lines) {
                    size_t equals = line.find('=');
                    if (equals == std::string::npos) {
                        continue;
                    }
                    std::string key(trim(std::string_view(line).substr(0, equals)));
                    std::string value(trim(std::string_view(line).substr(equals + 1)));
                    int32_t index = 0;
                    size_t paren = key.find('(');
                    if (paren != std::string::npos) {
                        index = std::atoi(key.c_str() + paren + 1);
                        key = key.substr(0, paren);
                    }
                    UProperty* property = actor->cls->findProperty(Name(key));
                    if (!property || index < 0 || index >= property->arrayDim || importedSeparately(property)) {
                        continue;
                    }
                    uint8_t* memory = actor->data + property->offset + static_cast<size_t>(index) * property->elementSize();
                    if (property->type == PropType::Object) {
                        UObject* target = reference(value, level, remap, searchLevel);
                        if (target) {
                            *reinterpret_cast<UObject**>(memory) = target;
                            continue;
                        }
                    }
                    property->fromText(memory, value);
                }
            }

            UModel* importBrush(const Record& record, UObject* package)
            {
                UModel* model = cast<UModel>(objects::newObject(csg::nativeClass("Model"), package, Name(), ImportFlags));
                UPolys* polys = cast<UPolys>(objects::newObject(csg::nativeClass("Polys"), package, Name(), ImportFlags));
                model->loaded = true;
                polys->loaded = true;
                editorops::importPolys(*polys, record.polyText);
                model->polys = polys;
                csg::updateBounds(*model);
                return model;
            }
        }

        std::string exportActors(const std::vector<UObject*>& actors)
        {
            std::string out = "Begin Map\r\n";
            for (UObject* actor : actors) {
                exportActor(out, actor);
            }
            out += "End Map\r\n";
            return out;
        }

        std::vector<UObject*> importActors(ULevel* level, std::string_view text, bool replaceLevel)
        {
            UClass* actorClass = objects::findClass("Engine.Actor");
            UClass* infoClass = objects::findClass("Engine.LevelInfo");
            UClass* brushClass = objects::findClass("Engine.Brush");
            UObject* package = level->outermost();
            std::vector<Record> records = parse(text);
            std::unordered_map<std::string, UObject*> remap;
            std::vector<UObject*> created;
            std::vector<const Record*> sources;
            for (const Record& record : records) {
                if (!record.cls || !record.cls->isChildOf(actorClass) || (record.cls->classFlags & CLASS_Abstract)) {
                    continue;
                }
                if (!replaceLevel && record.cls->isChildOf(infoClass)) {
                    continue;
                }
                UObject* actor = objects::newObject(record.cls, package, Name(), ImportFlags);
                actor->loaded = true;
                remap[lower(record.name)] = actor;
                if (record.hasBrush) {
                    UModel* model = importBrush(record, package);
                    actor::Brush(actor) = model;
                    remap[lower(record.brushName)] = model;
                }
                created.push_back(actor);
                sources.push_back(&record);
            }
            for (size_t i = 0; i < created.size(); i++) {
                applyProperties(created[i], *sources[i], level, remap, !replaceLevel);
            }
            if (replaceLevel) {
                UObject* info = level->actors.empty() ? nullptr : level->actors[0];
                UObject* builder = level->actors.size() > 1 ? level->actors[1] : nullptr;
                auto firstInfo = std::find_if(created.begin(), created.end(), [&](UObject* actor) {
                    return actor->isA(infoClass);
                });
                auto firstBrush = std::find_if(created.begin(), created.end(), [&](UObject* actor) {
                    return actor->cls == brushClass;
                });
                info = firstInfo != created.end() ? *firstInfo : info;
                builder = firstBrush != created.end() ? *firstBrush : builder;
                std::vector<UObject*> ordered = { info, builder };
                for (UObject* actor : created) {
                    if (actor != info && actor != builder) {
                        ordered.push_back(actor);
                    }
                }
                for (UObject* actor : level->actors) {
                    if (actor && actor != info && actor != builder) {
                        actor->destroyed = true;
                    }
                }
                level->actors = ordered;
            } else {
                level->actors.insert(level->actors.end(), created.begin(), created.end());
            }
            UObject* info = level->actors.empty() ? nullptr : level->actors[0];
            for (size_t i = 0; i < created.size(); i++) {
                UObject* actor = created[i];
                actor::Level(actor) = info;
                region(actor).zone = nullptr;
                if (sources[i]->hasBrush && !actor::bStatic(actor)) {
                    bsp::prepMovingBrush(actor);
                }
            }
            return created;
        }
    }
}
