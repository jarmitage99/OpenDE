#include "engine/console.h"

#include "audio/audio.h"
#include "core/config.h"
#include "core/objects.h"
#include "core/url.h"
#include "core/vm.h"
#include "engine/actor.h"
#include "engine/input.h"
#include "engine/resources.h"
#include "engine/savegame.h"
#include "engine/world.h"

#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <vector>

namespace dx {
    namespace console {
        std::function<void(const std::string&, int32_t)> openMapHook;
        std::function<void()> quitHook;
        std::function<std::string()> currentResHook;
        std::function<std::string()> resolutionsHook;
        std::function<void(int, int)> setResHook;
        std::function<void()> toggleFullscreenHook;
        std::function<void()> screenshotHook;

        namespace {
            struct Alias {
                Name name;
                std::string command;
            };

            struct InputState {
                std::array<std::string, 256> bindings;
                std::array<bool, 256> down {};
                std::vector<Alias> aliases;
                std::array<std::string, 256> keyNames;
                uint8_t action = IST_None;
                float delta = 0;
                bool inAlias = false;
                bool loaded = false;
            };

            InputState& input()
            {
                static InputState instance;
                return instance;
            }

            std::string trim(const std::string& text)
            {
                size_t begin = text.find_first_not_of(" \t\r\n");
                if (begin == std::string::npos) {
                    return {};
                }
                size_t end = text.find_last_not_of(" \t\r\n");
                return text.substr(begin, end - begin + 1);
            }

            std::string firstToken(const std::string& text, std::string& rest)
            {
                std::string trimmed = trim(text);
                size_t space = trimmed.find_first_of(" \t");
                if (space == std::string::npos) {
                    rest.clear();
                    return trimmed;
                }
                rest = trim(trimmed.substr(space + 1));
                return trimmed.substr(0, space);
            }

            void loadKeyNames()
            {
                InputState& state = input();
                Linker* engine = objects::loadPackage(Name("Engine"));
                if (!engine) {
                    return;
                }
                int32_t ref = engine->findExport(Name("EInputKey"), Name("Enum"), "*");
                UEnum* keys = ref ? cast<UEnum>(engine->object(ref)) : nullptr;
                objects::ensureLoaded(keys);
                if (!keys) {
                    return;
                }
                for (size_t i = 0; i < keys->names.size() && i < 256; i++) {
                    std::string text = keys->names[i].str();
                    if (text.size() > 3 && text.substr(0, 3) == "IK_") {
                        text = text.substr(3);
                    }
                    state.keyNames[i] = text;
                }
            }

            UObject* currentPlayer()
            {
                return World::current ? World::current->localPlayer : nullptr;
            }

            uint8_t* findInputVariable(UObject* player, const std::string& variable, PropType type)
            {
                if (!player || !player->cls) {
                    return nullptr;
                }
                UProperty* property = player->cls->findProperty(Name(variable));
                if (!property || !(property->propertyFlags & CPF_Input) || property->type != type) {
                    return nullptr;
                }
                return player->data + property->offset;
            }

            bool parseFloatOption(const std::string& text, const char* key, float& value)
            {
                std::string lower = text;
                std::string lowerKey = key;
                for (char& c : lower) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                for (char& c : lowerKey) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                size_t at = lower.find(lowerKey);
                if (at == std::string::npos) {
                    return false;
                }
                value = static_cast<float>(std::atof(text.c_str() + at + lowerKey.size()));
                return true;
            }

            void execInputCommands(const std::string& commands);

            bool execInput(const std::string& line)
            {
                InputState& state = input();
                std::string rest;
                std::string command = firstToken(line, rest);
                UObject* player = currentPlayer();
                if (iequals(command, "Button")) {
                    std::string variableRest;
                    std::string variable = firstToken(rest, variableRest);
                    uint8_t* button = findInputVariable(player, variable, PropType::Byte);
                    if (button) {
                        if (state.action == IST_Press) {
                            *button += 1;
                        } else if (state.action == IST_Release && *button) {
                            *button -= 1;
                        }
                    }
                    return true;
                }
                if (iequals(command, "Pulse")) {
                    std::string variableRest;
                    uint8_t* button = findInputVariable(player, firstToken(rest, variableRest), PropType::Byte);
                    if (button && state.action == IST_Press) {
                        *button = 1;
                    }
                    return true;
                }
                if (iequals(command, "Toggle")) {
                    std::string variableRest;
                    uint8_t* button = findInputVariable(player, firstToken(rest, variableRest), PropType::Byte);
                    if (button && state.action == IST_Press) {
                        *button ^= 0x80;
                    }
                    return true;
                }
                if (iequals(command, "Axis")) {
                    std::string variableRest;
                    std::string variable = firstToken(rest, variableRest);
                    uint8_t* address = findInputVariable(player, variable, PropType::Float);
                    if (address) {
                        float speed = 1.0f;
                        parseFloatOption(variableRest, "Speed=", speed);
                        float& axis = *reinterpret_cast<float*>(address);
                        if (state.action == IST_Axis) {
                            axis += 0.01f * state.delta * speed;
                        } else if (state.action == IST_Hold) {
                            axis += state.delta * speed;
                        }
                    }
                    return true;
                }
                if (!state.inAlias) {
                    Name alias(command);
                    for (const Alias& entry : state.aliases) {
                        if (entry.name == alias) {
                            state.inAlias = true;
                            execInputCommands(entry.command);
                            state.inAlias = false;
                            return true;
                        }
                    }
                }
                return false;
            }

            void execInputCommands(const std::string& commands)
            {
                InputState& state = input();
                std::stringstream stream(commands);
                std::string line;
                while (std::getline(stream, line, '|')) {
                    line = trim(line);
                    if (line.empty()) {
                        continue;
                    }
                    std::string rest;
                    std::string first = firstToken(line, rest);
                    if (state.action == IST_Press) {
                        if (iequals(first, "OnRelease")) {
                            continue;
                        }
                        if (!execInput(line)) {
                            execute(currentPlayer(), line);
                        }
                    } else if (state.action == IST_Release && iequals(first, "OnRelease")) {
                        if (!execInput(rest)) {
                            execute(currentPlayer(), rest);
                        }
                    } else {
                        execInput(line);
                    }
                }
            }

            bool callExec(UObject* target, const std::string& name, const std::string& arguments, std::string& output)
            {
                if (!target || target->destroyed) {
                    return false;
                }
                UFunction* function = vm::findFunction(target, Name(name));
                if (!function || !(function->functionFlags & FUNC_Exec)) {
                    return false;
                }
                vm::Call call(target, function);
                if (!call.valid()) {
                    return false;
                }
                std::string rest = arguments;
                for (size_t i = 0; i < function->params.size(); i++) {
                    UProperty* param = function->params[i];
                    bool last = i + 1 == function->params.size();
                    std::string token;
                    if ((param->type == PropType::Str || param->type == PropType::String) && last) {
                        token = trim(rest);
                        rest.clear();
                    } else {
                        token = firstToken(rest, rest);
                    }
                    if (token.empty()) {
                        call.skip();
                        continue;
                    }
                    uint8_t* slot = call.parameter(i);
                    param->fromText(slot, token);
                    call.skip();
                }
                call.run();
                if (function->returnParm && call.returnValue()) {
                    output = function->returnParm->toText(call.returnValue());
                }
                return true;
            }

            std::string iniSection(const std::string& path)
            {
                size_t dot = path.rfind('.');
                if (dot == std::string::npos) {
                    return {};
                }
                return config::system().get(path.substr(0, dot), path.substr(dot + 1)).value_or("");
            }

            UObject* findPropertyTarget(const std::string& className)
            {
                UClass* cls = objects::findClass(className);
                if (!cls) {
                    return nullptr;
                }
                return cls->defaultObject;
            }
        }

        void applyAudioSettings()
        {
            std::string section = config::system().get("Engine.Engine", "AudioDevice").value_or("");
            auto volume = [&section](const char* key, float fallback) {
                std::optional<std::string> value = config::system().get(section, key);
                return value ? static_cast<float>(std::atof(value->c_str())) / 255.0f : fallback;
            };
            audio::setVolumes(volume("SoundVolume", 1.0f), volume("MusicVolume", 1.0f), volume("SpeechVolume", 1.0f));
        }

        std::string execute(UObject* context, const std::string& command)
        {
            std::string rest;
            std::string word = firstToken(command, rest);
            if (word.empty()) {
                return {};
            }
            World* world = World::current;
            if (iequals(word, "open") || iequals(word, "start") || iequals(word, "servertravel")) {
                if (openMapHook) {
                    int32_t travelType = iequals(word, "open") ? TRAVEL_Partial : (iequals(word, "start") ? TRAVEL_Absolute : TRAVEL_Relative);
                    openMapHook(rest, travelType);
                }
                return {};
            }
            if (iequals(word, "exit") || iequals(word, "quit")) {
                if (quitHook) {
                    quitHook();
                }
                return {};
            }
            if (iequals(word, "GetCurrentRes")) {
                if (currentResHook) {
                    return currentResHook();
                }
                return {};
            }
            if (iequals(word, "GetRes")) {
                if (resolutionsHook) {
                    return resolutionsHook();
                }
                return {};
            }
            if (iequals(word, "SetRes")) {
                int width = 0;
                int height = 0;
                if (setResHook && std::sscanf(rest.c_str(), "%dx%d", &width, &height) == 2 && width > 0 && height > 0) {
                    setResHook(width, height);
                }
                return {};
            }
            if (iequals(word, "ToggleFullscreen")) {
                if (toggleFullscreenHook) {
                    toggleFullscreenHook();
                }
                return {};
            }
            if (iequals(word, "Shot")) {
                if (screenshotHook) {
                    screenshotHook();
                }
                return {};
            }
            if (iequals(word, "RMODE") || iequals(word, "RELAUNCH") || iequals(word, "CANCEL")) {
                return {};
            }
            if (iequals(word, "GETPING") || iequals(word, "GETLOSS")) {
                return "0";
            }
            if (iequals(word, "FLUSH")) {
                return {};
            }
            if (iequals(word, "DeleteGame")) {
                savegame::deleteGame(std::atoi(rest.c_str()));
                return {};
            }
            if (iequals(word, "set")) {
                std::string propertyRest;
                std::string className = firstToken(rest, propertyRest);
                std::string valueRest;
                std::string propertyName = firstToken(propertyRest, valueRest);
                if (iequals(className, "Input") || iequals(className, "InputExt")) {
                    uint8_t key = keyFromName(propertyName);
                    if (key) {
                        input().bindings[key] = valueRest;
                        config::user().set("Engine.Input", propertyName, valueRest);
                        config::user().save();
                    }
                    return {};
                }
                if (className.size() > 4 && iequals(className.substr(0, 4), "ini:")) {
                    std::string section = iniSection(className.substr(4));
                    if (section.empty()) {
                        return "Unrecognized ini key " + className;
                    }
                    config::system().set(section, propertyName, valueRest);
                    config::system().save();
                    applyAudioSettings();
                    return {};
                }
                UClass* cls = objects::findClass(className);
                if (!cls) {
                    return "Unrecognized class " + className;
                }
                UProperty* property = cls->findProperty(Name(propertyName));
                if (!property) {
                    return "Unrecognized property " + propertyName;
                }
                property->fromText(cls->defaultObject->data + property->offset, valueRest);
                for (UObject* object : objects::all()) {
                    if (object && object != cls->defaultObject && !object->destroyed && object->data && object->isA(cls)) {
                        property->fromText(object->data + property->offset, valueRest);
                    }
                }
                if (property->propertyFlags & (CPF_Config | CPF_GlobalConfig)) {
                    config::saveObjectConfig(cls->defaultObject, cls);
                }
                return {};
            }
            if (iequals(word, "get")) {
                std::string propertyRest;
                std::string className = firstToken(rest, propertyRest);
                std::string unused;
                std::string propertyName = firstToken(propertyRest, unused);
                if (className.size() > 4 && iequals(className.substr(0, 4), "ini:")) {
                    std::string section = iniSection(className.substr(4));
                    return section.empty() ? std::string() : config::system().get(section, propertyName).value_or("");
                }
                UObject* target = findPropertyTarget(className);
                if (!target) {
                    return "Unrecognized class " + className;
                }
                UProperty* property = target->cls->findProperty(Name(propertyName));
                if (!property) {
                    return "Unrecognized property " + propertyName;
                }
                return property->toText(target->data + property->offset);
            }
            if (iequals(word, "musicstatus")) {
                audio::MusicStatus status = audio::musicStatus();
                std::string name = status.music ? status.music->name.str() : "None";
                return "music " + name + " section " + std::to_string(status.section) + " order " + std::to_string(status.order) + " row " + std::to_string(status.row) + " pending " + std::to_string(status.pending ? 1 : 0);
            }
            if (iequals(word, "soundlist")) {
                std::string text;
                for (const audio::VoiceStatus& voice : audio::voices()) {
                    char line[256];
                    std::snprintf(line, sizeof(line), "%s actor %s slot %d ambient %d gain %.2f %.2f\n", voice.sound ? voice.sound->name.c_str() : "None", voice.actor ? voice.actor->name.c_str() : "None", voice.slot, voice.ambient ? 1 : 0, static_cast<double>(voice.gainLeft), static_cast<double>(voice.gainRight));
                    text += line;
                }
                return text;
            }
            if (iequals(word, "actorlist") && world) {
                UClass* cls = objects::findClass(rest);
                std::string text;
                for (UObject* a : world->actors) {
                    if (!a || a->destroyed || !cls || !a->isA(cls)) {
                        continue;
                    }
                    Vector location = actor::Location(a);
                    char line[256];
                    std::snprintf(line, sizeof(line), "%s %.0f %.0f %.0f\n", a->name.c_str(), static_cast<double>(location.x), static_cast<double>(location.y), static_cast<double>(location.z));
                    text += line;
                }
                return text;
            }
            if (iequals(word, "mapname") && world) {
                return world->level ? world->level->outermost()->name.str() : std::string();
            }
            if (iequals(word, "touch") && world) {
                UObject* player = currentPlayer();
                for (UObject* a : world->actors) {
                    if (a && !a->destroyed && player && iequals(a->name.str(), trim(rest))) {
                        vm::Call(a, Name("Touch")).obj(player).run();
                        return "touched " + a->name.str();
                    }
                }
                return "no actor " + rest;
            }
            if (iequals(word, "trigger") && world) {
                UObject* player = currentPlayer();
                for (UObject* a : world->actors) {
                    if (a && !a->destroyed && player && iequals(a->name.str(), trim(rest))) {
                        vm::Call(a, Name("Trigger")).obj(player).obj(player).run();
                        return "triggered " + a->name.str();
                    }
                }
                return "no actor " + rest;
            }
            if (iequals(word, "giveitem") && world) {
                UObject* player = currentPlayer();
                UClass* cls = objects::findClass(trim(rest));
                if (!player || !cls) {
                    return "cannot give " + rest;
                }
                UObject* item = world->spawn(cls, nullptr, Name(), actor::Location(player), actor::Rotation(player), nullptr, true);
                if (!item) {
                    return "spawn failed " + rest;
                }
                vm::Call(item, Name("GiveTo")).obj(player).run();
                return "gave " + item->name.str();
            }
            if (iequals(word, "invlist") && world) {
                UObject* player = currentPlayer();
                std::string text;
                int guard = 0;
                for (UObject* item = player ? actor::Inventory(player) : nullptr; item && guard < 200; item = actor::Inventory(item), guard++) {
                    text += item->cls->name.str() + " ";
                }
                return text;
            }
            if (iequals(word, "actorstats") && world) {
                std::map<std::string, int> counts;
                for (UObject* a : world->actors) {
                    if (a && !a->destroyed) {
                        counts[a->cls->name.str()]++;
                    }
                }
                std::string text;
                for (const auto& [name, count] : counts) {
                    text += name + " " + std::to_string(count) + "\n";
                }
                return text;
            }
            if (iequals(word, "inspect") && world) {
                std::string text;
                for (UObject* a : world->actors) {
                    if (!a || a->destroyed || a->name != Name(rest)) {
                        continue;
                    }
                    for (UProperty* property : a->cls->properties) {
                        for (int32_t i = 0; i < property->arrayDim; i++) {
                            size_t at = property->offset + static_cast<size_t>(i) * property->elementSize();
                            if (property->equals(a->data + at, a->cls->defaultObject->data + at)) {
                                continue;
                            }
                            text += property->name.str();
                            if (property->arrayDim > 1) {
                                text += "[" + std::to_string(i) + "]";
                            }
                            text += "=" + property->toText(a->data + at) + " ";
                        }
                    }
                    text += "state=" + vm::stateName(a).str() + "\n";
                }
                return text;
            }
            if (iequals(word, "lookat_actor") && world) {
                UObject* player = currentPlayer();
                for (UObject* a : world->actors) {
                    if (player && a && !a->destroyed && a->name == Name(rest)) {
                        Vector eye = actor::Location(player) + Vector { 0, 0, pawn::EyeHeight(player) };
                        Vector target = actor::Location(a) + Vector { 0, 0, actor::CollisionHeight(a) * 0.5f };
                        Rotator rotation = vectorRotation(target - eye);
                        pawn::ViewRotation(player) = rotation;
                        actor::Rotation(player).yaw = rotation.yaw;
                    }
                }
                return {};
            }
            if (iequals(word, "lookat") && world) {
                UObject* player = currentPlayer();
                Vector target;
                if (player && std::sscanf(rest.c_str(), "%f %f %f", &target.x, &target.y, &target.z) == 3) {
                    Vector eye = actor::Location(player) + Vector { 0, 0, pawn::EyeHeight(player) };
                    Rotator rotation = vectorRotation(target - eye);
                    pawn::ViewRotation(player) = rotation;
                    actor::Rotation(player).yaw = rotation.yaw;
                }
                return {};
            }
            if (iequals(word, "teleport") && world) {
                UObject* player = currentPlayer();
                float x = 0;
                float y = 0;
                float z = 0;
                int yaw = 0;
                if (player && std::sscanf(rest.c_str(), "%f %f %f %d", &x, &y, &z, &yaw) >= 3) {
                    world->farMoveActor(player, { x, y, z }, false, false);
                    Rotator rotation { 0, yaw, 0 };
                    actor::Rotation(player) = rotation;
                    pawn::ViewRotation(player) = rotation;
                }
                return {};
            }
            if (iequals(word, "keyname")) {
                return keyName(static_cast<uint8_t>(std::atoi(rest.c_str())));
            }
            if (iequals(word, "keybinding")) {
                return binding(keyFromName(rest));
            }
            if (iequals(word, "set_binding") || iequals(word, "setbind")) {
                std::string commandRest;
                std::string keyText = firstToken(rest, commandRest);
                uint8_t key = keyFromName(keyText);
                if (key) {
                    input().bindings[key] = commandRest;
                    config::user().set("Engine.Input", keyText, commandRest);
                }
                return {};
            }
            std::string output;
            std::vector<UObject*> targets;
            targets.push_back(context);
            UObject* scriptConsole = world && world->viewport ? player::Console(world->viewport) : nullptr;
            if (scriptConsole && scriptConsole != context) {
                targets.push_back(scriptConsole);
            }
            UObject* player = currentPlayer();
            if (player && player != context) {
                targets.push_back(player);
            }
            if (player) {
                targets.push_back(pawn::Weapon(player));
                targets.push_back(playerpawn::myHUD(player));
            }
            if (world) {
                targets.push_back(world->gameInfo);
            }
            for (UObject* target : targets) {
                if (callExec(target, word, rest, output)) {
                    return output;
                }
            }
            if (!execInput(command)) {
                std::fprintf(stderr, "unrecognized command: %s\n", command.c_str());
            }
            return {};
        }

        void loadBindings()
        {
            InputState& state = input();
            loadKeyNames();
            state.aliases.clear();
            for (std::string& text : state.bindings) {
                text.clear();
            }
            for (const auto& [key, value] : config::user().section("Engine.Input")) {
                if (key.size() > 8 && iequals(key.substr(0, 8), "Aliases[")) {
                    std::string text = value;
                    size_t commandStart = text.find("Command=\"");
                    size_t aliasStart = text.find("Alias=");
                    if (commandStart == std::string::npos || aliasStart == std::string::npos) {
                        continue;
                    }
                    size_t commandEnd = text.find('"', commandStart + 9);
                    std::string commandText = text.substr(commandStart + 9, commandEnd - commandStart - 9);
                    size_t aliasEnd = text.find_first_of(",)", aliasStart);
                    std::string aliasText = trim(text.substr(aliasStart + 6, aliasEnd - aliasStart - 6));
                    if (!aliasText.empty()) {
                        state.aliases.push_back({ Name(aliasText), commandText });
                    }
                    continue;
                }
                uint8_t code = keyFromName(key);
                if (code) {
                    state.bindings[code] = value;
                }
            }
            state.loaded = true;
        }

        bool key(uint8_t code, uint8_t action, float delta)
        {
            InputState& state = input();
            if (action == IST_Press) {
                if (state.down[code]) {
                    return false;
                }
                state.down[code] = true;
            } else if (action == IST_Release) {
                if (!state.down[code]) {
                    return false;
                }
                state.down[code] = false;
            }
            if (state.bindings[code].empty()) {
                return false;
            }
            uint8_t savedAction = state.action;
            float savedDelta = state.delta;
            state.action = action;
            state.delta = delta;
            execInputCommands(state.bindings[code]);
            state.action = savedAction;
            state.delta = savedDelta;
            return true;
        }

        void readInput(UObject* player, float deltaSeconds)
        {
            InputState& state = input();
            if (!player || !player->cls) {
                return;
            }
            if (deltaSeconds != -1.0f) {
                for (int i = 0; i < 255; i++) {
                    if (state.down[static_cast<size_t>(i)]) {
                        key(static_cast<uint8_t>(i), IST_Hold, deltaSeconds);
                    }
                }
            }
            float scale = deltaSeconds != -1.0f ? 20.0f / deltaSeconds : 0.0f;
            for (UProperty* property : player->cls->properties) {
                if ((property->propertyFlags & CPF_Input) && property->type == PropType::Float) {
                    *reinterpret_cast<float*>(player->data + property->offset) *= scale;
                }
            }
        }

        void resetInput(UObject* player)
        {
            InputState& state = input();
            state.down.fill(false);
            if (!player || !player->cls) {
                return;
            }
            for (UProperty* property : player->cls->properties) {
                if (!(property->propertyFlags & CPF_Input)) {
                    continue;
                }
                if (property->type == PropType::Float) {
                    *reinterpret_cast<float*>(player->data + property->offset) = 0;
                } else if (property->type == PropType::Byte) {
                    player->data[property->offset] = 0;
                }
            }
        }

        std::string binding(uint8_t code)
        {
            return input().bindings[code];
        }

        std::string keyName(uint8_t code)
        {
            if (input().keyNames[code].empty()) {
                loadKeyNames();
            }
            return input().keyNames[code];
        }

        uint8_t keyFromName(const std::string& name)
        {
            InputState& state = input();
            if (state.keyNames[1].empty()) {
                loadKeyNames();
            }
            for (size_t i = 1; i < 256; i++) {
                if (!state.keyNames[i].empty() && iequals(state.keyNames[i], name)) {
                    return static_cast<uint8_t>(i);
                }
            }
            return 0;
        }
    }
}
