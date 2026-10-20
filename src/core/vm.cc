#include "core/vm.h"

#include "core/objects.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace dx {
    namespace {
        struct NativeRegistry {
            std::vector<NativeFunction> byIndex = std::vector<NativeFunction>(4096, nullptr);
            std::vector<UFunction*> indexFunctions = std::vector<UFunction*>(4096, nullptr);
            std::unordered_map<std::string, NativeFunction> byName;
            std::set<std::string> warned;
            UFunction* current = nullptr;
            Frame* frame = nullptr;
            int depth = 0;
            uint32_t assertionFailures = 0;
            int32_t runaway = 0;
        };

        NativeRegistry& natives()
        {
            static NativeRegistry instance;
            return instance;
        }

        const char* probeNames[] = {
            "Spawned",
            "Destroyed",
            "GainedChild",
            "LostChild",
            "Probe4",
            "Probe5",
            "Trigger",
            "UnTrigger",
            "Timer",
            "HitWall",
            "Falling",
            "Landed",
            "ZoneChange",
            "Touch",
            "UnTouch",
            "Bump",
            "BeginState",
            "EndState",
            "BaseChange",
            "Attach",
            "Detach",
            "ActorEntered",
            "ActorLeaving",
            "KillCredit",
            "AnimEnd",
            "EndedRotation",
            "InterpolateEnd",
            "EncroachingOn",
            "EncroachedBy",
            "FootZoneChange",
            "HeadZoneChange",
            "PainTimer",
            "SpeechTimer",
            "MayFall",
            "Probe34",
            "Die",
            "Tick",
            "PlayerTick",
            "Expired",
            "Probe39",
            "SeePlayer",
            "EnemyNotVisible",
            "HearNoise",
            "UpdateEyeHeight",
            "SeeMonster",
            "SeeFriend",
            "SpecialHandling",
            "BotDesireability",
            "Probe48",
            "Probe49",
            "Probe50",
            "Probe51",
            "Probe52",
            "Probe53",
            "Probe54",
            "Probe55",
            "Probe56",
            "Probe57",
            "Probe58",
            "Probe59",
            "Probe60",
            "Probe61",
            "Probe62",
            "All",
        };

        std::string lowerKey(std::string_view text)
        {
            std::string key(text);
            for (char& c : key) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return key;
        }

        void evaluateDiscard(Frame& stack, UProperty* property)
        {
            if (!property) {
                stack.step(stack.object, nullptr);
                return;
            }
            std::vector<uint8_t> temp(std::max<size_t>(property->elementSize(), 16) + 16);
            uint8_t* memory = temp.data();
            property->construct(memory);
            stack.step(stack.object, memory);
            property->destroy(memory);
        }

        void unimplementedNative(UObject*, Frame& stack, void* result)
        {
            UFunction* function = natives().current;
            if (!function) {
                while (*stack.code != vm::EX_EndFunctionParms) {
                    stack.step(stack.object, nullptr);
                }
                stack.code++;
                return;
            }
            std::string key = function->outer ? function->outer->name.str() + "." + function->name.str() : function->name.str();
            if (natives().warned.insert(key).second) {
                std::fprintf(stderr, "unimplemented native %s (%d)\n", key.c_str(), function->nativeIndex);
            }
            for (UProperty* param : function->params) {
                if (*stack.code == vm::EX_EndFunctionParms) {
                    break;
                }
                if (*stack.code == vm::EX_Skip) {
                    stack.code++;
                    stack.readWord();
                }
                evaluateDiscard(stack, param);
            }
            vm::finish(stack);
            if (result && function->returnParm) {
                function->returnParm->clear(static_cast<uint8_t*>(result));
            }
            if (function->functionFlags & FUNC_Iterator) {
                uint16_t endOffset = stack.readWord();
                stack.jump(static_cast<uint16_t>(endOffset + 1));
            }
        }

        bool tracing()
        {
            static bool enabled = std::getenv("DX_TRACE") != nullptr;
            return enabled;
        }

        void traceCall(const char* kind, UObject* context, UFunction* function)
        {
            if (!tracing() || !function) {
                return;
            }
            std::fprintf(stderr, "%*s%s %s.%s\n", natives().depth * 2, "", kind, context ? context->name.c_str() : "None", function->pathName().c_str());
        }

        void callNative(Frame& stack, UObject* context, UFunction* function, NativeFunction native, void* result)
        {
            traceCall("native", context, function);
            natives().current = function;
            native(context, stack, result);
        }

        std::string floatText(float value)
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%f", static_cast<double>(value));
            return buffer;
        }

        bool parseVector(const std::string& text, Vector& out)
        {
            return std::sscanf(text.c_str(), "%f,%f,%f", &out.x, &out.y, &out.z) == 3;
        }
    }

    uint8_t Frame::readByte()
    {
        return *code++;
    }

    uint16_t Frame::readWord()
    {
        uint16_t value;
        std::memcpy(&value, code, 2);
        code += 2;
        return value;
    }

    int32_t Frame::readInt()
    {
        int32_t value;
        std::memcpy(&value, code, 4);
        code += 4;
        return value;
    }

    float Frame::readFloat()
    {
        float value;
        std::memcpy(&value, code, 4);
        code += 4;
        return value;
    }

    UObject* Frame::readObject()
    {
        return objects::fromHandle(static_cast<uint32_t>(readInt()));
    }

    Name Frame::readName()
    {
        return Name::fromIndex(static_cast<uint32_t>(readInt()));
    }

    void Frame::jump(uint16_t offset)
    {
        code = node->script.data() + offset;
    }

    void Frame::step(UObject* context, void* result)
    {
        uint8_t token = *code++;
        switch (token) {
        case 0x00:
        case 0x29: {
            UProperty* property = cast<UProperty>(readObject());
            uint8_t* address = locals + property->offset;
            propAddr = address;
            prop = property;
            if (result) {
                property->copy(static_cast<uint8_t*>(result), address);
            }
            return;
        }
        case 0x01: {
            UProperty* property = cast<UProperty>(readObject());
            if (!context->data) {
                objects::ensureLoaded(context);
            }
            uint8_t* address = context->data + property->offset;
            propAddr = address;
            prop = property;
            if (result) {
                property->copy(static_cast<uint8_t*>(result), address);
            }
            return;
        }
        case 0x02: {
            UProperty* property = cast<UProperty>(readObject());
            uint8_t* address = context->cls->defaultObject->data + property->offset;
            propAddr = address;
            prop = property;
            if (result) {
                property->copy(static_cast<uint8_t*>(result), address);
            }
            return;
        }
        case 0x04: {
            UFunction* function = cast<UFunction>(node);
            if (function && function->returnParm && *code != vm::EX_Nothing) {
                step(object, locals + function->returnParm->offset);
            } else {
                step(object, nullptr);
            }
            code = nullptr;
            return;
        }
        case 0x05: {
            uint8_t size = readByte();
            bool isString = size == 0;
            uint8_t switchBuffer[256] = {};
            std::string switchString;
            step(object, isString ? static_cast<void*>(&switchString) : switchBuffer);
            size_t compare = std::max<size_t>(size, 8);
            while (true) {
                code++;
                uint16_t next = readWord();
                if (next == 0xffff) {
                    break;
                }
                bool match;
                if (isString) {
                    std::string value;
                    step(object, &value);
                    match = iequals(value, switchString);
                } else {
                    uint8_t valueBuffer[256] = {};
                    step(object, valueBuffer);
                    match = std::memcmp(switchBuffer, valueBuffer, std::min<size_t>(compare, 256)) == 0;
                }
                if (match) {
                    break;
                }
                jump(next);
            }
            return;
        }
        case 0x06:
            if (++natives().runaway > 10000000) {
                vm::critical(*this, "Runaway loop detected (over 10000000 iterations)");
                natives().runaway = 0;
            }
            jump(readWord());
            return;
        case 0x07: {
            if (++natives().runaway > 10000000) {
                vm::critical(*this, "Runaway loop detected (over 10000000 iterations)");
                natives().runaway = 0;
            }
            uint16_t offset = readWord();
            uint32_t value = 0;
            step(object, &value);
            if (!value) {
                jump(offset);
            }
            return;
        }
        case 0x08:
            code = nullptr;
            return;
        case 0x09: {
            uint16_t line = readWord();
            uint32_t value = 0;
            step(object, &value);
            if (!value) {
                natives().assertionFailures++;
                vm::critical(*this, "Assertion failed, line " + std::to_string(line));
            }
            return;
        }
        case 0x0A: {
            uint16_t next = readWord();
            if (next != 0xffff) {
                step(object, nullptr);
            }
            return;
        }
        case 0x0B:
        case 0x15:
            return;
        case 0x0D: {
            Name label;
            step(object, &label);
            vm::gotoLabel(object, label);
            return;
        }
        case 0x0E: {
            std::string value;
            step(object, &value);
            return;
        }
        case 0x0F:
        case 0x14: {
            propAddr = nullptr;
            prop = nullptr;
            step(object, nullptr);
            uint8_t* target = propAddr;
            UProperty* targetProp = prop;
            if (!target || !targetProp) {
                step(object, nullptr);
                return;
            }
            nullContext = nullptr;
            step(object, target);
            if (nullContext == target) {
                targetProp->clear(target);
            }
            nullContext = nullptr;
            return;
        }
        case 0x11: {
            UObject* outerObject = context;
            Name objectName;
            int32_t objectFlags = 0;
            UObject* classObject = nullptr;
            step(object, &outerObject);
            step(object, &objectName);
            step(object, &objectFlags);
            step(object, &classObject);
            UClass* newClass = cast<UClass>(classObject);
            UObject* created = nullptr;
            if (newClass) {
                created = objects::newObject(newClass, outerObject ? outerObject : objects::packageObject(Name("Transient")), objectName, static_cast<uint32_t>(objectFlags) & 0x704005u);
            }
            vm::ret<UObject*>(result, created);
            return;
        }
        case 0x12: {
            UObject* classObject = nullptr;
            step(object, &classObject);
            UClass* contextClass = cast<UClass>(classObject);
            if (contextClass) {
                objects::ensureLoaded(contextClass);
                contextClass->linkClass();
                code += 3;
                step(contextClass->defaultObject, result);
            } else {
                vm::warn(*this, "accessed null class context");
                uint16_t skip = readWord();
                readByte();
                code += skip;
                nullContext = result;
                propAddr = nullptr;
            }
            return;
        }
        case 0x13: {
            UClass* target = cast<UClass>(readObject());
            UObject* value = nullptr;
            step(object, &value);
            UClass* valueClass = cast<UClass>(value);
            vm::ret<UObject*>(result, (valueClass && target && valueClass->isChildOf(target)) ? valueClass : nullptr);
            return;
        }
        case 0x16:
            code--;
            return;
        case 0x17:
            vm::ret<UObject*>(result, context);
            return;
        case 0x18:
            readWord();
            step(object, result);
            return;
        case 0x19: {
            UObject* newContext = nullptr;
            step(context, &newContext);
            if (newContext) {
                code += 3;
                step(newContext, result);
            } else {
                vm::warn(*this, "accessed None");
                uint16_t skip = readWord();
                readByte();
                code += skip;
                nullContext = result;
                propAddr = nullptr;
            }
            return;
        }
        case 0x1A: {
            int32_t index = 0;
            step(object, &index);
            propAddr = nullptr;
            prop = nullptr;
            step(context, nullptr);
            UProperty* property = prop;
            uint8_t* base = propAddr;
            if (!property || !base) {
                return;
            }
            if (index < 0 || index >= property->arrayDim) {
                vm::warn(*this, "array index out of bounds " + std::to_string(index) + "/" + std::to_string(property->arrayDim));
                index = std::clamp(index, 0, property->arrayDim - 1);
            }
            uint8_t* address = base + static_cast<size_t>(index) * property->elementSize();
            propAddr = address;
            prop = property;
            if (result) {
                property->copy(static_cast<uint8_t*>(result), address);
            }
            return;
        }
        case 0x1B: {
            Name functionName = readName();
            UFunction* function = vm::findFunction(context, functionName);
            if (!function) {
                vm::warn(*this, "function not found: " + functionName.str());
            }
            vm::callFunction(*this, context, function, result);
            return;
        }
        case 0x1C: {
            UFunction* function = cast<UFunction>(readObject());
            vm::callFunction(*this, context, function, result);
            return;
        }
        case 0x38: {
            Name functionName = readName();
            UFunction* function = vm::findFunction(context, functionName, true);
            vm::callFunction(*this, context, function, result);
            return;
        }
        case 0x1D:
            vm::ret<int32_t>(result, readInt());
            return;
        case 0x1E:
            vm::ret<float>(result, readFloat());
            return;
        case 0x1F: {
            const char* text = reinterpret_cast<const char*>(code);
            size_t length = std::strlen(text);
            code += length + 1;
            if (result) {
                static_cast<std::string*>(result)->assign(text, length);
            }
            return;
        }
        case 0x34: {
            std::string text;
            while (true) {
                uint16_t c = readWord();
                if (c == 0) {
                    break;
                }
                text.push_back(c < 256 ? static_cast<char>(c) : '?');
            }
            if (result) {
                *static_cast<std::string*>(result) = text;
            }
            return;
        }
        case 0x20:
            vm::ret<UObject*>(result, readObject());
            return;
        case 0x21:
            vm::ret<Name>(result, readName());
            return;
        case 0x22: {
            Rotator value;
            value.pitch = readInt();
            value.yaw = readInt();
            value.roll = readInt();
            vm::ret<Rotator>(result, value);
            return;
        }
        case 0x23: {
            Vector value;
            value.x = readFloat();
            value.y = readFloat();
            value.z = readFloat();
            vm::ret<Vector>(result, value);
            return;
        }
        case 0x24:
            vm::ret<uint8_t>(result, readByte());
            return;
        case 0x25:
            vm::ret<int32_t>(result, 0);
            return;
        case 0x26:
            vm::ret<int32_t>(result, 1);
            return;
        case 0x27:
            vm::ret<uint32_t>(result, 1);
            return;
        case 0x28:
            vm::ret<uint32_t>(result, 0);
            return;
        case 0x2A:
            vm::ret<UObject*>(result, nullptr);
            return;
        case 0x2B:
            readByte();
            step(object, result);
            return;
        case 0x2C:
            vm::ret<int32_t>(result, static_cast<int32_t>(readByte()));
            return;
        case 0x2D:
            step(context, result);
            return;
        case 0x2E: {
            UClass* target = cast<UClass>(readObject());
            UObject* value = nullptr;
            step(object, &value);
            vm::ret<UObject*>(result, (value && target && value->isA(target)) ? value : nullptr);
            return;
        }
        case 0x2F:
            step(object, result);
            return;
        case 0x30:
        case 0x31:
            return;
        case 0x32:
        case 0x33: {
            UStruct* structType = cast<UStruct>(readObject());
            std::vector<uint8_t> a(structType->size + 16);
            std::vector<uint8_t> b(structType->size + 16);
            structType->initValue(a.data());
            structType->initValue(b.data());
            step(object, a.data());
            step(object, b.data());
            bool equal = structType->equalsValue(a.data(), b.data());
            structType->destroyValue(a.data());
            structType->destroyValue(b.data());
            vm::retBool(result, token == 0x32 ? equal : !equal);
            return;
        }
        case 0x36: {
            UProperty* member = cast<UProperty>(readObject());
            UStruct* structType = cast<UStruct>(member->outer);
            std::vector<uint8_t> buffer(structType->size + 16);
            structType->initValue(buffer.data());
            propAddr = nullptr;
            step(context, buffer.data());
            uint8_t* address = propAddr ? propAddr + member->offset : nullptr;
            if (result) {
                member->copy(static_cast<uint8_t*>(result), buffer.data() + member->offset);
            }
            structType->destroyValue(buffer.data());
            propAddr = address;
            prop = member;
            return;
        }
        case 0x39: {
            Rotator value;
            step(object, &value);
            vm::ret<Vector>(result, rotationVector(value));
            return;
        }
        case 0x3A: {
            uint8_t value = 0;
            step(object, &value);
            vm::ret<int32_t>(result, value);
            return;
        }
        case 0x3B: {
            uint8_t value = 0;
            step(object, &value);
            vm::retBool(result, value != 0);
            return;
        }
        case 0x3C: {
            uint8_t value = 0;
            step(object, &value);
            vm::ret<float>(result, static_cast<float>(value));
            return;
        }
        case 0x3D: {
            int32_t value = 0;
            step(object, &value);
            vm::ret<uint8_t>(result, static_cast<uint8_t>(value));
            return;
        }
        case 0x3E: {
            int32_t value = 0;
            step(object, &value);
            vm::retBool(result, value != 0);
            return;
        }
        case 0x3F: {
            int32_t value = 0;
            step(object, &value);
            vm::ret<float>(result, static_cast<float>(value));
            return;
        }
        case 0x40: {
            uint32_t value = 0;
            step(object, &value);
            vm::ret<uint8_t>(result, value ? 1 : 0);
            return;
        }
        case 0x41: {
            uint32_t value = 0;
            step(object, &value);
            vm::ret<int32_t>(result, value ? 1 : 0);
            return;
        }
        case 0x42: {
            uint32_t value = 0;
            step(object, &value);
            vm::ret<float>(result, value ? 1.0f : 0.0f);
            return;
        }
        case 0x43: {
            float value = 0;
            step(object, &value);
            vm::ret<uint8_t>(result, static_cast<uint8_t>(static_cast<int32_t>(value)));
            return;
        }
        case 0x44: {
            float value = 0;
            step(object, &value);
            vm::ret<int32_t>(result, static_cast<int32_t>(value));
            return;
        }
        case 0x45: {
            float value = 0;
            step(object, &value);
            vm::retBool(result, value != 0);
            return;
        }
        case 0x46: {
            UObject* value = nullptr;
            step(object, &value);
            vm::ret<int32_t>(result, value ? 1 : 0);
            return;
        }
        case 0x47: {
            UObject* value = nullptr;
            step(object, &value);
            vm::retBool(result, value != nullptr);
            return;
        }
        case 0x48: {
            Name value;
            step(object, &value);
            vm::retBool(result, !value.none());
            return;
        }
        case 0x49: {
            std::string value;
            step(object, &value);
            vm::ret<uint8_t>(result, static_cast<uint8_t>(std::atoi(value.c_str())));
            return;
        }
        case 0x4A: {
            std::string value;
            step(object, &value);
            vm::ret<int32_t>(result, std::atoi(value.c_str()));
            return;
        }
        case 0x4B: {
            std::string value;
            step(object, &value);
            vm::retBool(result, iequals(value, "true") || std::atoi(value.c_str()) != 0);
            return;
        }
        case 0x4C: {
            std::string value;
            step(object, &value);
            vm::ret<float>(result, static_cast<float>(std::atof(value.c_str())));
            return;
        }
        case 0x4D: {
            std::string value;
            step(object, &value);
            Vector parsed;
            parseVector(value, parsed);
            vm::ret<Vector>(result, parsed);
            return;
        }
        case 0x4E: {
            std::string value;
            step(object, &value);
            Rotator parsed;
            std::sscanf(value.c_str(), "%d,%d,%d", &parsed.pitch, &parsed.yaw, &parsed.roll);
            vm::ret<Rotator>(result, parsed);
            return;
        }
        case 0x4F: {
            Vector value;
            step(object, &value);
            vm::retBool(result, !value.zero());
            return;
        }
        case 0x50: {
            Vector value;
            step(object, &value);
            vm::ret<Rotator>(result, vectorRotation(value));
            return;
        }
        case 0x51: {
            Rotator value;
            step(object, &value);
            vm::retBool(result, value.pitch != 0 || value.yaw != 0 || value.roll != 0);
            return;
        }
        case 0x52: {
            uint8_t value = 0;
            step(object, &value);
            vm::ret<std::string>(result, std::to_string(value));
            return;
        }
        case 0x53: {
            int32_t value = 0;
            step(object, &value);
            vm::ret<std::string>(result, std::to_string(value));
            return;
        }
        case 0x54: {
            uint32_t value = 0;
            step(object, &value);
            vm::ret<std::string>(result, value ? "True" : "False");
            return;
        }
        case 0x55: {
            float value = 0;
            step(object, &value);
            vm::ret<std::string>(result, floatText(value));
            return;
        }
        case 0x56: {
            UObject* value = nullptr;
            step(object, &value);
            vm::ret<std::string>(result, value ? value->pathName() : "None");
            return;
        }
        case 0x57: {
            Name value;
            step(object, &value);
            vm::ret<std::string>(result, value.str());
            return;
        }
        case 0x58: {
            Vector value;
            step(object, &value);
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "%f,%f,%f", static_cast<double>(value.x), static_cast<double>(value.y), static_cast<double>(value.z));
            vm::ret<std::string>(result, buffer);
            return;
        }
        case 0x59: {
            Rotator value;
            step(object, &value);
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "%d,%d,%d", value.pitch, value.yaw, value.roll);
            vm::ret<std::string>(result, buffer);
            return;
        }
        case 0x5A: {
            std::string value;
            step(object, &value);
            vm::ret<Name>(result, Name(value));
            return;
        }
        default:
            break;
        }

        int32_t nativeIndex = -1;
        if (token >= 0x70) {
            nativeIndex = token;
        } else if (token >= 0x60) {
            nativeIndex = ((token - 0x60) << 8) | readByte();
        }
        if (nativeIndex < 0) {
            throw std::runtime_error("unknown bytecode token " + std::to_string(token) + " in " + (node ? node->pathName() : std::string("?")));
        }
        NativeRegistry& registry = natives();
        NativeFunction native = registry.byIndex[static_cast<size_t>(nativeIndex)];
        UFunction* function = registry.indexFunctions[static_cast<size_t>(nativeIndex)];
        if (!native) {
            native = unimplementedNative;
        }
        callNative(*this, context, function, native, result);
    }

    namespace vm {
        void registerNative(int32_t index, NativeFunction function)
        {
            natives().byIndex[static_cast<size_t>(index)] = function;
        }

        void registerNative(std::string_view className, std::string_view functionName, NativeFunction function)
        {
            natives().byName[lowerKey(className) + "." + lowerKey(functionName)] = function;
        }

        NativeFunction bindNative(UFunction* function)
        {
            NativeRegistry& registry = natives();
            if (function->nativeIndex > 0 && function->nativeIndex < 4096) {
                registry.indexFunctions[function->nativeIndex] = function;
                NativeFunction native = registry.byIndex[function->nativeIndex];
                if (native) {
                    return native;
                }
            }
            UObject* owner = function->outer;
            while (owner && !cast<UClass>(owner)) {
                owner = owner->outer;
            }
            if (owner) {
                auto it = registry.byName.find(lowerKey(owner->name.str()) + "." + lowerKey(function->name.str()));
                if (it != registry.byName.end()) {
                    if (function->nativeIndex > 0 && function->nativeIndex < 4096) {
                        registry.byIndex[function->nativeIndex] = it->second;
                    }
                    return it->second;
                }
            }
            return unimplementedNative;
        }

        uint32_t assertionFailures()
        {
            return natives().assertionFailures;
        }

        CriticalHandler& criticalHandler()
        {
            static CriticalHandler handler = [](const std::string& message) {
                std::fprintf(stderr, "Critical Error: %s\n", message.c_str());
                std::fflush(stderr);
                std::_Exit(1);
            };
            return handler;
        }

        CriticalHandler setCriticalHandler(CriticalHandler handler)
        {
            CriticalHandler previous = criticalHandler();
            criticalHandler() = std::move(handler);
            return previous;
        }

        void critical(const Frame& frame, const std::string& message)
        {
            std::string text = frame.object ? frame.object->fullName() : std::string("None");
            if (frame.node) {
                bool state = cast<UState>(frame.node) && !cast<UFunction>(frame.node);
                char offset[16];
                long position = frame.code ? static_cast<long>(frame.code - frame.node->script.data()) : 0;
                std::snprintf(offset, sizeof(offset), "%04lX", static_cast<unsigned long>(position));
                text += std::string(" (") + (state ? "State " : "Function ") + frame.node->pathName() + ":" + offset + ")";
            }
            criticalHandler()(text + " " + message);
        }

        void fatal(const std::string& message)
        {
            criticalHandler()(message);
        }

        void resetRunaway()
        {
            natives().runaway = 0;
            natives().depth = 0;
        }

        void warn(const Frame& frame, const std::string& message)
        {
            static int count = 0;
            if (count++ > 2000) {
                return;
            }
            std::string where = frame.node ? frame.node->pathName() : std::string("?");
            std::string who = frame.object ? frame.object->name.str() : std::string("?");
            std::fprintf(stderr, "script warning: %s (%s in %s)\n", message.c_str(), who.c_str(), where.c_str());
        }

        void execute(Frame& frame)
        {
            const uint8_t* end = frame.node->script.data() + frame.node->script.size();
            Frame* saved = natives().frame;
            natives().frame = &frame;
            while (frame.code && frame.code < end) {
                frame.step(frame.object, nullptr);
            }
            natives().frame = saved;
        }

        void callFunction(Frame& stack, UObject* context, UFunction* function, void* result)
        {
            if (!function) {
                while (*stack.code != EX_EndFunctionParms) {
                    stack.step(stack.object, nullptr);
                }
                stack.code++;
                return;
            }
            if (function->functionFlags & FUNC_Native) {
                if (!function->native) {
                    function->native = bindNative(function);
                }
                callNative(stack, context, function, function->native, result);
                return;
            }
            objects::ensureLoaded(function);
            function->link();

            NativeRegistry& registry = natives();
            if (registry.depth > 250) {
                critical(stack, "Infinite script recursion (250 calls) detected");
                while (*stack.code != EX_EndFunctionParms) {
                    stack.step(stack.object, nullptr);
                }
                stack.code++;
                return;
            }

            std::unique_ptr<uint8_t[]> locals(new uint8_t[function->size + 16]);
            function->initValue(locals.get());

            struct OutParm {
                uint8_t* address;
                UProperty* property;
            };

            OutParm outs[32];
            size_t outCount = 0;
            for (UProperty* param : function->params) {
                if (*stack.code == EX_EndFunctionParms) {
                    break;
                }
                uint8_t* slot = locals.get() + param->offset;
                stack.nullContext = nullptr;
                if (param->propertyFlags & CPF_OutParm) {
                    stack.propAddr = nullptr;
                    stack.step(stack.object, slot);
                    if (stack.propAddr && outCount < 32) {
                        outs[outCount++] = { stack.propAddr, param };
                    }
                } else {
                    stack.step(stack.object, slot);
                }
                if (stack.nullContext == slot) {
                    param->clear(slot);
                }
                stack.nullContext = nullptr;
            }
            if (*stack.code == EX_EndFunctionParms) {
                stack.code++;
            } else {
                warn(stack, "missing end of function parms calling " + function->pathName());
            }

            traceCall("call", context, function);
            uint32_t singular = function->functionFlags & FUNC_Singular;
            if (context && isProbing(context, function->name) && !(singular && context->singular)) {
                Frame frame;
                frame.object = context;
                frame.node = function;
                frame.code = function->script.data();
                frame.locals = locals.get();
                frame.previous = &stack;
                if (singular) {
                    context->singular = true;
                }
                registry.depth++;
                try {
                    execute(frame);
                } catch (...) {
                    registry.depth--;
                    if (singular) {
                        context->singular = false;
                    }
                    function->destroyValue(locals.get());
                    throw;
                }
                registry.depth--;
                if (singular) {
                    context->singular = false;
                }
            }

            for (size_t i = outCount; i > 0; i--) {
                outs[i - 1].property->copy(outs[i - 1].address, locals.get() + outs[i - 1].property->offset);
            }
            if (result && function->returnParm) {
                function->returnParm->copy(static_cast<uint8_t*>(result), locals.get() + function->returnParm->offset);
            }
            function->destroyValue(locals.get());
        }

        int32_t probeIndex(Name eventName)
        {
            static std::unordered_map<Name, int32_t> lookup = [] {
                std::unordered_map<Name, int32_t> table;
                for (int32_t i = 0; i < 64; i++) {
                    table[Name(probeNames[i])] = i;
                }
                return table;
            }();
            auto it = lookup.find(eventName);
            return it == lookup.end() ? -1 : it->second;
        }

        StateFrame& stateFrame(UObject* object)
        {
            if (!object->stateFrame) {
                object->stateFrame = std::make_unique<StateFrame>();
                object->stateFrame->object = object;
                object->stateFrame->node = object->cls;
            }
            return *object->stateFrame;
        }

        UFunction* findFunction(UObject* object, Name functionName, bool global)
        {
            if (!object || !object->cls) {
                return nullptr;
            }
            if (!global && object->stateFrame && object->stateFrame->state) {
                UFunction* function = object->stateFrame->state->findStateFunction(functionName);
                if (function) {
                    return function;
                }
            }
            return object->cls->findFunction(functionName);
        }

        bool isProbing(UObject* object, Name functionName)
        {
            int32_t probe = probeIndex(functionName);
            if (probe < 0 || !object->stateFrame) {
                return true;
            }
            StateFrame& frame = *object->stateFrame;
            if (frame.disabledProbes & (1ull << probe)) {
                return false;
            }
            return !frame.state || (frame.state->ignoreMask & (1ull << probe));
        }

        UFunction* findEvent(UObject* object, Name eventName)
        {
            if (!object || object->destroyed || !isProbing(object, eventName)) {
                return nullptr;
            }
            return findFunction(object, eventName);
        }

        Name stateName(UObject* object)
        {
            if (!object->stateFrame || !object->stateFrame->state) {
                return Name();
            }
            return object->stateFrame->state->name;
        }

        bool gotoLabel(UObject* object, Name label)
        {
            StateFrame& frame = stateFrame(object);
            for (UState* source = frame.state; source; source = cast<UState>(source->superField)) {
                objects::ensureLoaded(source);
                if (source->labelTableOffset == 0xffff || source->labelTableOffset >= source->script.size()) {
                    continue;
                }
                const uint8_t* entry = source->script.data() + source->labelTableOffset;
                while (true) {
                    uint32_t labelName;
                    uint32_t offset;
                    std::memcpy(&labelName, entry, 4);
                    std::memcpy(&offset, entry + 4, 4);
                    entry += 8;
                    if (labelName == 0) {
                        break;
                    }
                    if (Name::fromIndex(labelName) == label) {
                        frame.node = source;
                        frame.code = source->script.data() + offset;
                        frame.latentAction = 0;
                        return true;
                    }
                }
            }
            return false;
        }

        bool gotoState(UObject* object, Name newStateName, Name label)
        {
            StateFrame& frame = stateFrame(object);
            UState* newState = nullptr;
            if (newStateName == Name("Auto")) {
                for (UClass* cls = object->cls; cls && !newState; cls = cls->superClass()) {
                    for (UField* child = cls->children; child; child = child->next) {
                        UState* state = cast<UState>(child);
                        if (state && !cast<UFunction>(child) && (state->stateFlags & STATE_Auto)) {
                            newState = object->cls->findState(state->name);
                            break;
                        }
                    }
                }
            } else if (!newStateName.none()) {
                newState = object->cls->findState(newStateName);
            }
            frame.latentAction = 0;
            UState* oldState = frame.state;
            if (oldState && oldState != newState && !frame.inEndState) {
                frame.stateChanged = false;
                frame.inEndState = true;
                event(object, Name("EndState"));
                frame.inEndState = false;
                if (frame.stateChanged) {
                    return false;
                }
            }
            frame.state = newState;
            frame.node = newState ? static_cast<UStruct*>(newState) : static_cast<UStruct*>(object->cls);
            frame.code = nullptr;
            frame.disabledProbes = 0;
            if (newState && newState != oldState) {
                frame.stateChanged = false;
                event(object, Name("BeginState"));
                if (frame.stateChanged) {
                    return false;
                }
            }
            if (!newState) {
                return false;
            }
            frame.stateChanged = true;
            gotoLabel(object, label.none() ? Name("Begin") : label);
            return true;
        }

        void runStateCode(UObject* object)
        {
            if (!object->stateFrame) {
                return;
            }
            StateFrame& frame = *object->stateFrame;
            if (!frame.code || frame.latentAction != 0 || frame.running || object->destroyed) {
                return;
            }
            frame.running = true;
            Frame* saved = natives().frame;
            natives().frame = &frame;
            UState* lastState = frame.state;
            int32_t stateChanges = 0;
            try {
                while (frame.code && frame.latentAction == 0 && !object->destroyed) {
                    const uint8_t* end = frame.node->script.data() + frame.node->script.size();
                    if (frame.code >= end) {
                        frame.code = nullptr;
                        break;
                    }
                    frame.step(object, nullptr);
                    if (frame.state != lastState) {
                        lastState = frame.state;
                        if (++stateChanges > 4) {
                            break;
                        }
                    }
                }
            } catch (const std::exception& error) {
                std::fprintf(stderr, "script error in state code of %s: %s\n", object->name.c_str(), error.what());
                frame.code = nullptr;
            }
            natives().frame = saved;
            frame.running = false;
        }

        void restoreState(UObject* object, UState* state, UStruct* node, int32_t offset, uint64_t)
        {
            if (!state) {
                return;
            }
            StateFrame& frame = stateFrame(object);
            frame.state = cast<UClass>(state) ? nullptr : state;
            frame.node = node ? node : state;
            frame.code = nullptr;
            if (node && offset >= 0 && static_cast<size_t>(offset) < node->script.size()) {
                frame.code = node->script.data() + offset;
            }
        }

        void disable(UObject* object, Name eventName)
        {
            int32_t probe = probeIndex(eventName);
            if (probe >= 0) {
                stateFrame(object).disabledProbes |= 1ull << probe;
            }
        }

        void enable(UObject* object, Name eventName)
        {
            int32_t probe = probeIndex(eventName);
            if (probe >= 0) {
                stateFrame(object).disabledProbes &= ~(1ull << probe);
            }
        }

        bool getBool(Frame& stack)
        {
            uint32_t value = 0;
            stack.step(stack.object, &value);
            return value != 0;
        }

        bool getBoolOpt(Frame& stack, bool fallback)
        {
            uint32_t value = fallback ? 1 : 0;
            if (*stack.code == EX_EndFunctionParms) {
                return fallback;
            }
            stack.step(stack.object, &value);
            return value != 0;
        }

        uint16_t getSkip(Frame& stack)
        {
            if (*stack.code == EX_Skip) {
                stack.code++;
                return stack.readWord();
            }
            return 0;
        }

        void finish(Frame& stack)
        {
            if (*stack.code == EX_EndFunctionParms) {
                stack.code++;
            }
        }

        void iterate(Frame& stack, const std::function<bool()>& next)
        {
            uint16_t endOffset = stack.readWord();
            const uint8_t* start = stack.code;
            while (true) {
                if (!next()) {
                    stack.jump(static_cast<uint16_t>(endOffset + 1));
                    return;
                }
                uint8_t token = 0;
                while (stack.code && (token = *stack.code) != EX_IteratorPop && token != EX_IteratorNext) {
                    stack.step(stack.object, nullptr);
                }
                if (!stack.code) {
                    return;
                }
                stack.code++;
                if (token == EX_IteratorPop) {
                    return;
                }
                stack.code = start;
            }
        }

        Call::Call(UObject* target, Name functionName)
            : Call(target, findEvent(target, functionName))
        {
        }

        Call::Call(UObject* target, UFunction* callee)
            : object(target)
            , function(callee)
        {
            if (!callee || !target || target->destroyed) {
                function = nullptr;
                return;
            }
            objects::ensureLoaded(callee);
            callee->link();
            buffer.resize(callee->size + 16);
            callee->initValue(buffer.data());
        }

        Call::~Call()
        {
            if (function) {
                function->destroyValue(buffer.data());
            }
        }

        bool Call::valid() const
        {
            return function != nullptr;
        }

        UProperty* Call::nextParam()
        {
            if (!function || argument >= function->params.size()) {
                return nullptr;
            }
            return function->params[argument++];
        }

        uint8_t* Call::parameter(size_t index)
        {
            if (!function || index >= function->params.size()) {
                return nullptr;
            }
            return buffer.data() + function->params[index]->offset;
        }

        uint8_t* Call::returnValue()
        {
            if (!function || !function->returnParm) {
                return nullptr;
            }
            return buffer.data() + function->returnParm->offset;
        }

        Call& Call::obj(UObject* value)
        {
            if (UProperty* param = nextParam()) {
                *reinterpret_cast<UObject**>(buffer.data() + param->offset) = value;
            }
            return *this;
        }

        Call& Call::f(float value)
        {
            if (UProperty* param = nextParam()) {
                *reinterpret_cast<float*>(buffer.data() + param->offset) = value;
            }
            return *this;
        }

        Call& Call::i(int32_t value)
        {
            if (UProperty* param = nextParam()) {
                if (param->type == PropType::Byte) {
                    buffer[param->offset] = static_cast<uint8_t>(value);
                } else {
                    *reinterpret_cast<int32_t*>(buffer.data() + param->offset) = value;
                }
            }
            return *this;
        }

        Call& Call::b(bool value)
        {
            if (UProperty* param = nextParam()) {
                *reinterpret_cast<uint32_t*>(buffer.data() + param->offset) = value ? 1 : 0;
            }
            return *this;
        }

        Call& Call::byte(uint8_t value)
        {
            if (UProperty* param = nextParam()) {
                buffer[param->offset] = value;
            }
            return *this;
        }

        Call& Call::n(Name value)
        {
            if (UProperty* param = nextParam()) {
                *reinterpret_cast<Name*>(buffer.data() + param->offset) = value;
            }
            return *this;
        }

        Call& Call::s(const std::string& value)
        {
            if (UProperty* param = nextParam()) {
                *reinterpret_cast<std::string*>(buffer.data() + param->offset) = value;
            }
            return *this;
        }

        Call& Call::v(const Vector& value)
        {
            if (UProperty* param = nextParam()) {
                *reinterpret_cast<Vector*>(buffer.data() + param->offset) = value;
            }
            return *this;
        }

        Call& Call::r(const Rotator& value)
        {
            if (UProperty* param = nextParam()) {
                *reinterpret_cast<Rotator*>(buffer.data() + param->offset) = value;
            }
            return *this;
        }

        Call& Call::skip()
        {
            nextParam();
            return *this;
        }

        void Call::run()
        {
            if (!function || !object || object->destroyed) {
                return;
            }
            Frame stub;
            stub.object = object;
            stub.node = function;
            stub.locals = buffer.data();
            stub.code = function->stub.data();
            if (function->functionFlags & FUNC_Native) {
                if (!function->native) {
                    function->native = bindNative(function);
                }
                callNative(stub, object, function, function->native, returnValue());
                return;
            }
            uint32_t singular = function->functionFlags & FUNC_Singular;
            if (!isProbing(object, function->name) || (singular && object->singular)) {
                return;
            }
            traceCall("event", object, function);
            Frame frame;
            frame.object = object;
            frame.node = function;
            frame.code = function->script.data();
            frame.locals = buffer.data();
            frame.previous = natives().frame;
            NativeRegistry& registry = natives();
            if (registry.depth > 250) {
                critical(frame, "Infinite script recursion (250 calls) detected");
                return;
            }
            if (singular) {
                object->singular = true;
            }
            registry.depth++;
            try {
                execute(frame);
            } catch (const std::exception& error) {
                std::fprintf(stderr, "script error in %s: %s\n", function->pathName().c_str(), error.what());
            }
            registry.depth--;
            if (singular) {
                object->singular = false;
            }
        }

        bool event(UObject* object, Name eventName)
        {
            Call call(object, eventName);
            if (!call.valid()) {
                return false;
            }
            call.run();
            return true;
        }
    }
}
