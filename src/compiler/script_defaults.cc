#include "compiler/script_parser.h"

#include "compiler/exporter.h"
#include "core/objects.h"
#include "engine/package.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace dx::compiler::detail {
    namespace {
        int hexDigit(char c)
        {
            if (c >= '0' && c <= '9') {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f') {
                return c - 'a' + 10;
            }
            if (c >= 'A' && c <= 'F') {
                return c - 'A' + 10;
            }
            return 0;
        }

        const char* readToken(const char* buffer, std::string& out, bool dottedNames = false)
        {
            out.clear();
            if (*buffer == '"') {
                buffer++;
                while (*buffer && *buffer != '"' && *buffer != 13 && *buffer != 10) {
                    if (*buffer != '\\') {
                        out += *buffer++;
                    } else if (*++buffer == '\\') {
                        out += '\\';
                        buffer++;
                    } else {
                        out += static_cast<char>(hexDigit(buffer[0]) * 16 + hexDigit(buffer[1]));
                        buffer += 2;
                    }
                }
                if (*buffer++ != '"') {
                    return nullptr;
                }
                return buffer;
            }
            if (isAlnum(*buffer)) {
                while (isAlnum(*buffer) || *buffer == '_' || *buffer == '-' || (dottedNames && *buffer == '.')) {
                    out += *buffer++;
                }
                return buffer;
            }
            return nullptr;
        }

        const char* importText(Lookup& lookup, UProperty* property, const char* buffer, uint8_t* data);

        const char* importStruct(Lookup& lookup, UStruct* structType, const char* buffer, uint8_t* data)
        {
            if (*buffer++ != '(') {
                return nullptr;
            }
            while (*buffer != ')') {
                std::string memberName;
                while (*buffer && *buffer != '=' && *buffer != '[') {
                    memberName += *buffer++;
                }
                int32_t element = 0;
                if (*buffer == '[') {
                    buffer++;
                    const char* begin = buffer;
                    while (*buffer >= '0' && *buffer <= '9') {
                        buffer++;
                    }
                    if (*buffer++ != ']') {
                        return nullptr;
                    }
                    element = std::atoi(begin);
                }
                if (*buffer++ != '=') {
                    return nullptr;
                }
                Name gotName = Name::find(memberName);
                bool parsed = false;
                if (!gotName.none()) {
                    for (UProperty* member : structType->fieldOrder()) {
                        if (member->name == gotName && element >= 0 && element < member->arrayDim && member->elementSize() != 0 && portable(member)) {
                            buffer = importText(lookup, member, buffer, data + member->offset + static_cast<size_t>(element) * member->elementSize());
                            if (!buffer) {
                                return nullptr;
                            }
                            parsed = true;
                        }
                    }
                }
                if (!parsed) {
                    int subCount = 0;
                    while (*buffer && *buffer != 10 && *buffer != 13 && (subCount > 0 || *buffer != ')') && (subCount > 0 || *buffer != ',')) {
                        if (*buffer == '"') {
                            buffer++;
                            while (*buffer && *buffer != '"' && *buffer != 10 && *buffer != 13) {
                                buffer++;
                            }
                            if (*buffer != '"') {
                                return nullptr;
                            }
                        } else if (*buffer == '(') {
                            subCount++;
                        } else if (*buffer == ')') {
                            subCount--;
                        }
                        buffer++;
                    }
                }
                if (*buffer != ',') {
                    break;
                }
                buffer++;
            }
            if (*buffer++ != ')') {
                return nullptr;
            }
            return buffer;
        }

        const char* importText(Lookup& lookup, UProperty* property, const char* buffer, uint8_t* data)
        {
            std::string temp;
            switch (property->type) {
            case PropType::Byte:
                if (property->enumType) {
                    const char* after = readToken(buffer, temp);
                    if (!after) {
                        return nullptr;
                    }
                    Name entry = Name::find(temp);
                    if (!entry.none()) {
                        auto it = std::find(property->enumType->names.begin(), property->enumType->names.end(), entry);
                        if (it != property->enumType->names.end()) {
                            *data = static_cast<uint8_t>(it - property->enumType->names.begin());
                            return after;
                        }
                    }
                }
                if (std::isdigit(static_cast<unsigned char>(*buffer))) {
                    *data = static_cast<uint8_t>(std::atoi(buffer));
                    while (*buffer >= '0' && *buffer <= '9') {
                        buffer++;
                    }
                    return buffer;
                }
                return nullptr;
            case PropType::Int:
                if (*buffer == '-' || (*buffer >= '0' && *buffer <= '9')) {
                    *reinterpret_cast<int32_t*>(data) = std::atoi(buffer);
                }
                while (*buffer == '-' || (*buffer >= '0' && *buffer <= '9')) {
                    buffer++;
                }
                return buffer;
            case PropType::Bool:
                buffer = readToken(buffer, temp);
                if (!buffer) {
                    return nullptr;
                }
                if (iequals(temp, "1") || iequals(temp, "True")) {
                    *reinterpret_cast<uint32_t*>(data) = 1;
                } else if (iequals(temp, "0") || iequals(temp, "False")) {
                    *reinterpret_cast<uint32_t*>(data) = 0;
                } else {
                    return nullptr;
                }
                return buffer;
            case PropType::Float:
                *reinterpret_cast<float*>(data) = static_cast<float>(std::atof(buffer));
                while (*buffer && *buffer != ',' && *buffer != ')' && *buffer != 13 && *buffer != 10) {
                    buffer++;
                }
                return buffer;
            case PropType::Object:
            case PropType::Class: {
                buffer = readToken(buffer, temp, true);
                if (!buffer) {
                    return nullptr;
                }
                UObject*& target = *reinterpret_cast<UObject**>(data);
                if (iequals(temp, "None")) {
                    target = nullptr;
                } else {
                    while (*buffer == ' ') {
                        buffer++;
                    }
                    if (*buffer++ != '\'') {
                        target = lookup.findObject(property->objectClass, temp);
                        if (!target) {
                            return nullptr;
                        }
                    } else {
                        std::string other;
                        buffer = readToken(buffer, other, true);
                        if (!buffer || *buffer++ != '\'') {
                            return nullptr;
                        }
                        UClass* objectClass = lookup.findClass(temp);
                        if (!objectClass) {
                            return nullptr;
                        }
                        target = lookup.findObject(objectClass, other);
                        if (!target && other.find('.') != std::string::npos) {
                            target = objects::loadObject(other, objectClass);
                            lookup.refresh();
                        }
                        if (!target) {
                            return nullptr;
                        }
                    }
                }
                if (property->type == PropType::Class && target) {
                    UClass* cls = cast<UClass>(target);
                    if (!cls || !cls->isChildOf(property->metaClass)) {
                        target = nullptr;
                    }
                }
                return buffer;
            }
            case PropType::Name:
                buffer = readToken(buffer, temp);
                if (!buffer) {
                    return nullptr;
                }
                *reinterpret_cast<uint32_t*>(data) = Name(temp).index();
                return buffer;
            case PropType::Str:
            case PropType::String:
                buffer = readToken(buffer, temp);
                if (!buffer) {
                    return nullptr;
                }
                *reinterpret_cast<std::string*>(data) = temp;
                return buffer;
            case PropType::Struct:
                return importStruct(lookup, property->structType, buffer, data);
            case PropType::FixedArray: {
                if (*buffer++ != '(') {
                    return nullptr;
                }
                size_t size = property->inner->elementSize();
                for (int32_t i = 0; i < property->fixedCount; i++) {
                    buffer = importText(lookup, property->inner, buffer, data + static_cast<size_t>(i) * size);
                    if (!buffer) {
                        return nullptr;
                    }
                    if (*buffer != ',' && i != property->fixedCount - 1) {
                        return nullptr;
                    }
                    buffer++;
                }
                if (*buffer++ != ')') {
                    return nullptr;
                }
                return buffer;
            }
            case PropType::Array:
            case PropType::Map:
            case PropType::Pointer:
                return buffer;
            }
            return nullptr;
        }
    }

    void importDefaults(PackageCompiler::Impl& context, UClass* cls, const std::string& text)
    {
        size_t at = 0;
        while (at < text.size()) {
            size_t end = text.find_first_of("\r\n", at);
            std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
            at = end == std::string::npos ? text.size() : end;
            if (at < text.size() && text[at] == '\r') {
                at++;
            }
            if (at < text.size() && text[at] == '\n') {
                at++;
            }
            const char* str = line.c_str();
            while (*str == ' ' || *str == 9) {
                str++;
            }
            const char* begin = str;
            while (*str && *str != '=' && *str != '(') {
                str++;
            }
            if (!*str) {
                continue;
            }
            std::string token(begin, static_cast<size_t>(str - begin));
            int32_t index = 0;
            if (*str == '(') {
                str++;
                index = std::atoi(str);
                while (*str && *str != ')') {
                    str++;
                }
                if (!*str++) {
                    context.log(cls->pathName() + ": Missing ')' in default properties subscript: " + line);
                    continue;
                }
            }
            if (*str++ != '=') {
                context.log(cls->pathName() + ": Missing '=' in default properties assignment: " + line);
                continue;
            }
            UProperty* property = nullptr;
            Name propertyName = Name::find(token);
            if (!propertyName.none()) {
                for (UProperty* candidate : cls->fieldOrder()) {
                    if (candidate->name == propertyName) {
                        property = candidate;
                        break;
                    }
                }
            }
            if (!property) {
                context.log(cls->pathName() + ": Unknown property in defaults: " + line);
                continue;
            }
            if (index >= property->arrayDim) {
                context.log(cls->pathName() + ": Out of bound array default property");
                continue;
            }
            if (!iequals(property->name.str(), "Name")) {
                importText(context.lookup, property, str, cls->defaultObject->data + property->offset + static_cast<size_t>(index) * property->elementSize());
            }
        }
    }
}
