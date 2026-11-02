#include "engine/property_edit.h"

#include "core/objects.h"
#include "engine/resources.h"

#include <algorithm>
#include <cmath>

namespace dx {
    namespace propedit {
        namespace {
            const char* const ColorChannels[4] = { "R", "G", "B", "A" };
        }

        bool accepts(const UProperty* property, UObject* value)
        {
            if (!value || value->destroyed) {
                return false;
            }
            if (property->type == PropType::Class) {
                UClass* cls = cast<UClass>(value);
                return cls && (!property->metaClass || cls->isChildOf(property->metaClass));
            }
            if (property->type == PropType::Object) {
                return !property->objectClass || value->isA(property->objectClass);
            }
            return false;
        }

        UObject* pickCurrent(const UProperty* property, const std::vector<UObject*>& candidates)
        {
            for (UObject* candidate : candidates) {
                if (accepts(property, candidate)) {
                    return candidate;
                }
            }
            return nullptr;
        }

        bool assignObject(const UProperty* property, uint8_t* memory, UObject* value)
        {
            if (!accepts(property, value)) {
                return false;
            }
            *reinterpret_cast<UObject**>(memory) = value;
            return true;
        }

        bool isColor(const UProperty* property)
        {
            if (property->type != PropType::Struct || !property->structType || property->structType->name != Name("Color")) {
                return false;
            }
            for (const char* channel : ColorChannels) {
                UProperty* member = property->structType->findProperty(Name(channel));
                if (!member || member->type != PropType::Byte) {
                    return false;
                }
            }
            return true;
        }

        std::array<float, 4> colorValue(const UProperty* property, const uint8_t* memory)
        {
            std::array<float, 4> color {};
            for (size_t i = 0; i < 4; i++) {
                UProperty* member = property->structType->findProperty(Name(ColorChannels[i]));
                color[i] = static_cast<float>(memory[member->offset]) / 255.0f;
            }
            return color;
        }

        void setColor(const UProperty* property, uint8_t* memory, const std::array<float, 4>& color)
        {
            for (size_t i = 0; i < 4; i++) {
                UProperty* member = property->structType->findProperty(Name(ColorChannels[i]));
                memory[member->offset] = static_cast<uint8_t>(std::lround(std::clamp(color[i], 0.0f, 1.0f) * 255.0f));
            }
        }

        std::vector<UObject*> levelPropertyObjects(ULevel* level)
        {
            std::vector<UObject*> result;
            if (!level || level->actors.empty() || !level->actors[0]) {
                return result;
            }
            result.push_back(level->actors[0]);
            UClass* deusExInfoClass = objects::findClass("DeusEx.DeusExLevelInfo");
            for (UObject* actor : level->actors) {
                if (actor && !actor->destroyed && actor->data && deusExInfoClass && actor->isA(deusExInfoClass)) {
                    result.push_back(actor);
                }
            }
            return result;
        }
    }
}
