#include "compiler/exporter.h"

#include "engine/package.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace dx::compiler {
    namespace {
        std::string formatFloat(float value)
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%f", static_cast<double>(value));
            if (static_cast<float>(std::atof(buffer)) != value) {
                std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(value));
            }
            return buffer;
        }

        bool isZero(const uint8_t* memory, size_t size)
        {
            for (size_t i = 0; i < size; i++) {
                if (memory[i]) {
                    return false;
                }
            }
            return true;
        }

        std::string trimText(const std::string& text)
        {
            size_t start = 0;
            size_t end = text.size();
            while (start < end && (text[start] == '\r' || text[start] == '\n' || text[start] == ' ')) {
                start++;
            }
            while (end > start && (text[end - 1] == '\r' || text[end - 1] == '\n' || text[end - 1] == ' ')) {
                end--;
            }
            return text.substr(start, end - start);
        }
    }

    bool portable(const UProperty* property)
    {
        return property->elementSize() > 0 && (!property->category.none() || !(property->propertyFlags & (CPF_Transient | CPF_Native))) && property->name != Name("Class");
    }

    bool identical(const UProperty* property, const uint8_t* a, const uint8_t* b)
    {
        switch (property->type) {
        case PropType::Byte:
            return *a == (b ? *b : 0);
        case PropType::Int:
            return *reinterpret_cast<const int32_t*>(a) == (b ? *reinterpret_cast<const int32_t*>(b) : 0);
        case PropType::Bool:
            return (*reinterpret_cast<const uint32_t*>(a) != 0) == (b ? *reinterpret_cast<const uint32_t*>(b) != 0 : false);
        case PropType::Float:
            return *reinterpret_cast<const float*>(a) == (b ? *reinterpret_cast<const float*>(b) : 0.0f);
        case PropType::Object:
        case PropType::Class:
            return *reinterpret_cast<UObject* const*>(a) == (b ? *reinterpret_cast<UObject* const*>(b) : nullptr);
        case PropType::Name:
            return *reinterpret_cast<const uint32_t*>(a) == (b ? *reinterpret_cast<const uint32_t*>(b) : Name().index());
        case PropType::Str:
        case PropType::String:
            return iequals(*reinterpret_cast<const std::string*>(a), b ? *reinterpret_cast<const std::string*>(b) : std::string());
        case PropType::Struct:
            for (UProperty* member : property->structType->fieldOrder()) {
                for (int32_t i = 0; i < member->arrayDim; i++) {
                    size_t at = member->offset + static_cast<size_t>(i) * member->elementSize();
                    if (!identical(member, a + at, b ? b + at : nullptr)) {
                        return false;
                    }
                }
            }
            return true;
        case PropType::FixedArray:
            for (int32_t i = 0; i < property->fixedCount; i++) {
                size_t at = static_cast<size_t>(i) * property->inner->elementSize();
                if (!identical(property->inner, a + at, b ? b + at : nullptr)) {
                    return false;
                }
            }
            return true;
        case PropType::Array:
        case PropType::Map:
        case PropType::Pointer:
            return b ? std::memcmp(a, b, property->elementSize()) == 0 : isZero(a, property->elementSize());
        }
        return true;
    }

    std::string exportTextItem(const UProperty* property, const uint8_t* value, const uint8_t* defaults, bool delimited)
    {
        switch (property->type) {
        case PropType::Byte:
            if (property->enumType && *value < property->enumType->names.size()) {
                return property->enumType->names[*value].str();
            }
            return std::to_string(*value);
        case PropType::Int:
            return std::to_string(*reinterpret_cast<const int32_t*>(value));
        case PropType::Bool:
            return *reinterpret_cast<const uint32_t*>(value) ? "True" : "False";
        case PropType::Float:
            return formatFloat(*reinterpret_cast<const float*>(value));
        case PropType::Object:
        case PropType::Class: {
            const UObject* object = *reinterpret_cast<UObject* const*>(value);
            if (!object) {
                return "None";
            }
            return object->cls->name.str() + "'" + object->pathName() + "'";
        }
        case PropType::Name: {
            std::string text = Name::fromIndex(*reinterpret_cast<const uint32_t*>(value)).str();
            if (text.find_first_of(" ,") != std::string::npos) {
                return "\"" + text + "\"";
            }
            return text;
        }
        case PropType::Str:
        case PropType::String: {
            const std::string& text = *reinterpret_cast<const std::string*>(value);
            return delimited ? "\"" + text + "\"" : text;
        }
        case PropType::Struct: {
            std::string text;
            int count = 0;
            for (UProperty* member : property->structType->fieldOrder()) {
                if (!portable(member)) {
                    continue;
                }
                for (int32_t i = 0; i < member->arrayDim; i++) {
                    size_t at = member->offset + static_cast<size_t>(i) * member->elementSize();
                    const uint8_t* memberDefault = defaults ? defaults + at : nullptr;
                    if (value != defaults && identical(member, value + at, memberDefault)) {
                        continue;
                    }
                    count++;
                    text += count == 1 ? "(" : ",";
                    text += member->name.str();
                    if (member->arrayDim > 1) {
                        text += "[" + std::to_string(i) + "]";
                    }
                    text += "=" + exportTextItem(member, value + at, memberDefault, true);
                }
            }
            if (count > 0) {
                text += ")";
            }
            return text;
        }
        case PropType::FixedArray: {
            std::string text = "(";
            size_t size = property->inner->elementSize();
            for (int32_t i = 0; i < property->fixedCount; i++) {
                if (i > 0) {
                    text += ",";
                }
                size_t at = static_cast<size_t>(i) * size;
                text += exportTextItem(property->inner, value + at, defaults ? defaults + at : nullptr, true);
            }
            return text + ")";
        }
        case PropType::Array:
        case PropType::Map:
        case PropType::Pointer:
            return "";
        }
        return "";
    }

    std::string exportProperties(const UStruct* structType, const uint8_t* data, const UClass* diffClass, const uint8_t* diff, int indent)
    {
        std::string text;
        std::string space(static_cast<size_t>(indent), ' ');
        for (const UStruct* owner = structType; owner; owner = owner->superStruct()) {
            const uint8_t* delta = diffClass && diffClass->isChildOf(owner) ? diff : nullptr;
            for (UField* child = owner->children; child; child = child->next) {
                UProperty* property = cast<UProperty>(child);
                if (!property || !portable(property)) {
                    continue;
                }
                for (int32_t j = 0; j < property->arrayDim; j++) {
                    size_t at = property->offset + static_cast<size_t>(j) * property->elementSize();
                    if (data != delta && identical(property, data + at, delta ? delta + at : nullptr)) {
                        continue;
                    }
                    std::string value = exportTextItem(property, data + at, delta ? delta + at : nullptr, true);
                    if (property->arrayDim == 1) {
                        text += space + " " + property->name.str() + "=" + value + "\r\n";
                    } else {
                        text += space + " " + property->name.str() + "(" + std::to_string(j) + ")=" + value + "\r\n";
                    }
                }
            }
        }
        return text;
    }

    std::vector<uint8_t> defaultsBuffer(UClass* cls)
    {
        std::vector<uint8_t> buffer(std::max<size_t>(cls->size, 1));
        cls->initValue(buffer.data());
        cls->copyValue(buffer.data(), cls->defaultObject->data);
        UClass* root = cls;
        while (root->superClass()) {
            root = root->superClass();
        }
        for (UField* child = root->children; child; child = child->next) {
            UProperty* property = cast<UProperty>(child);
            if (property && (property->propertyFlags & CPF_Native)) {
                for (int32_t i = 0; i < property->arrayDim; i++) {
                    property->clear(buffer.data() + property->offset + static_cast<size_t>(i) * property->elementSize());
                }
            }
        }
        return buffer;
    }

    std::string exportClass(UClass* cls)
    {
        std::string text = trimText(cls->scriptText ? cls->scriptText->text : std::string());
        text += "\r\n\r\ndefaultproperties\r\n{\r\n";
        UClass* parent = cls->superClass();
        std::vector<uint8_t> data = defaultsBuffer(cls);
        std::vector<uint8_t> diff = parent ? defaultsBuffer(parent) : std::vector<uint8_t>();
        text += exportProperties(cls, data.data(), parent, parent ? diff.data() : nullptr, 4);
        cls->destroyValue(data.data());
        if (parent) {
            parent->destroyValue(diff.data());
        }
        text += "}\r\n";
        return text;
    }
}
