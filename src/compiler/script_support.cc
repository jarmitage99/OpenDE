#include "compiler/script_parser.h"

#include "core/objects.h"
#include "core/vm.h"
#include "engine/package.h"

#include <cctype>

namespace dx::compiler::detail {
    [[noreturn]] void fail(const std::string& message)
    {
        throw CompileError(message);
    }

    const Names& names()
    {
        static const Names instance;
        return instance;
    }

    uint32_t getConversion(const PropertyBase& dest, const PropertyBase& source)
    {
        constexpr uint32_t M = ExMax;
        constexpr uint32_t AC = AutoConvert;
        constexpr uint32_t TAC = Truncate | AutoConvert;
        static const uint32_t conversions[CptMax][CptMax] = {
            { M, M, M, M, M, M, M, M, M, M, M, M, M, M },
            { M, M, ExIntToByte | TAC, ExBoolToByte, ExFloatToByte | TAC, M, M, M, M, M, M, M, M, ExStringToByte },
            { M, ExByteToInt | AC, M, ExBoolToInt, ExFloatToInt | TAC, M, M, M, M, M, M, M, M, ExStringToInt },
            { M, ExByteToBool, ExIntToBool, M, ExFloatToBool, ExObjectToBool, ExNameToBool, M, M, M, M, ExVectorToBool, ExRotatorToBool, ExStringToBool },
            { M, ExByteToFloat | AC, ExIntToFloat | AC, ExBoolToFloat, M, M, M, M, M, M, M, M, M, ExStringToFloat },
            { M, M, M, M, M, M, M, M, M, M, M, M, M, M },
            { M, M, M, M, M, M, M, M, M, M, M, M, M, M },
            { M, M, M, M, M, M, M, M, M, M, M, M, M, M },
            { M, M, M, M, M, M, M, M, M, M, M, M, M, M },
            { M, M, M, M, M, M, M, M, M, M, M, M, M, M },
            { M, M, M, M, M, M, M, M, M, M, M, M, M, M },
            { M, M, M, M, M, M, M, M, M, M, M, M, ExRotatorToVector, ExStringToVector },
            { M, M, M, M, M, M, M, M, M, M, M, ExVectorToRotator, M, ExStringToRotator },
            { M, ExByteToString, ExIntToString, ExBoolToString, ExFloatToString, ExObjectToString, ExNameToString, M, M, M, M, ExVectorToString, ExRotatorToString, M },
        };
        int destType = dest.isVector() ? CptVector : dest.isRotator() ? CptRotation
                                                                      : dest.type;
        int sourceType = source.isVector() ? CptVector : source.isRotator() ? CptRotation
                                                                            : source.type;
        return conversions[destType][sourceType];
    }

    const char* const cppTags[96] = { "Spc", "Not", "DoubleQuote", "Pound", "Concat", "Percent", "And", "SingleQuote", "OpenParen", "CloseParen", "Multiply", "Add", "Comma", "Subtract", "Dot", "Divide", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "Colon", "Semicolon", "Less", "Equal", "Greater", "Question", "At", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "OpenBracket", "Backslash", "CloseBracket", "Xor", "_", "Not", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "OpenBrace", "Or", "CloseBrace", "Complement", "Or" };

    const char* cptTypeName(int type)
    {
        switch (type) {
        case CptByte:
            return "Byte";
        case CptInt:
            return "Int";
        case CptBool:
            return "Bool";
        case CptFloat:
            return "Float";
        case CptObject:
            return "Object";
        case CptName:
            return "Name";
        case CptStruct:
            return "Struct";
        case CptVector:
            return "Vector";
        case CptRotation:
            return "Rotator";
        case CptString:
            return "Str";
        default:
            return "None";
        }
    }

    bool isAnsi(const std::string& text)
    {
        for (char c : text) {
            if (static_cast<unsigned char>(c) >= 0x80) {
                return false;
            }
        }
        return true;
    }

    UClass* intrinsic(const char* name)
    {
        return objects::intrinsicClass(Name(name));
    }

    UProperty* newProperty(PropType type, UObject* outer, Name name, uint32_t flags)
    {
        const char* className = "ByteProperty";
        switch (type) {
        case PropType::Byte:
            className = "ByteProperty";
            break;
        case PropType::Int:
            className = "IntProperty";
            break;
        case PropType::Bool:
            className = "BoolProperty";
            break;
        case PropType::Float:
            className = "FloatProperty";
            break;
        case PropType::Object:
            className = "ObjectProperty";
            break;
        case PropType::Class:
            className = "ClassProperty";
            break;
        case PropType::Name:
            className = "NameProperty";
            break;
        case PropType::Str:
            className = "StrProperty";
            break;
        case PropType::String:
            className = "StringProperty";
            break;
        case PropType::Struct:
            className = "StructProperty";
            break;
        case PropType::Array:
            className = "ArrayProperty";
            break;
        case PropType::Map:
            className = "MapProperty";
            break;
        case PropType::FixedArray:
            className = "FixedArrayProperty";
            break;
        case PropType::Pointer:
            className = "PointerProperty";
            break;
        }
        return createField(new UProperty(type), outer, name, className, flags);
    }

    UProperty* cloneParameter(UProperty* source, UObject* outer)
    {
        UProperty* property = newProperty(source->type, outer, source->name, RF_Public);
        property->enumType = source->enumType;
        property->objectClass = source->objectClass;
        property->metaClass = source->metaClass;
        property->structType = source->structType;
        property->arrayDim = source->arrayDim;
        property->propertyFlags = source->propertyFlags;
        return property;
    }

    bool isA(UField* field, const char* kind)
    {
        if (!field) {
            return false;
        }
        std::string want(kind);
        if (want == "Field") {
            return true;
        }
        if (want == "Property") {
            return cast<UProperty>(field) != nullptr;
        }
        if (want == "Function") {
            return cast<UFunction>(field) != nullptr;
        }
        if (want == "State") {
            return cast<UState>(field) != nullptr && !cast<UClass>(field);
        }
        if (want == "Struct") {
            return cast<UStruct>(field) != nullptr;
        }
        return false;
    }

    bool isAlnum(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) != 0;
    }
}
