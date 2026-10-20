#include "compiler/script_model.h"

#include "core/objects.h"
#include "engine/package.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>

namespace dx::compiler {
    namespace {
        int32_t alignTo(int32_t value, int32_t alignment)
        {
            return (value + alignment - 1) / alignment * alignment;
        }

        uint32_t crcTable(uint32_t index)
        {
            static uint32_t table[256];
            static bool ready = false;
            if (!ready) {
                for (uint32_t i = 0; i < 256; i++) {
                    uint32_t c = i << 24;
                    for (int bit = 0; bit < 8; bit++) {
                        c = (c & 0x80000000u) ? (c << 1) ^ 0x04c11db7u : (c << 1);
                    }
                    table[i] = c;
                }
                ready = true;
            }
            return table[index];
        }

        uint32_t stringCrc(const std::string& text)
        {
            uint32_t crc = 0xffffffffu;
            for (char c : text) {
                uint32_t low = static_cast<uint8_t>(c);
                crc = (crc << 8) ^ crcTable((crc >> 24) ^ low);
                crc = (crc << 8) ^ crcTable(crc >> 24);
            }
            return ~crc;
        }

        struct Ue1Layout {
            int32_t elementSize = 0;
            int32_t alignment = 4;
        };

        Ue1Layout layoutOf(UProperty* property)
        {
            switch (property->type) {
            case PropType::Byte:
                return { 1, 1 };
            case PropType::Int:
            case PropType::Float:
            case PropType::Name:
            case PropType::Object:
            case PropType::Class:
            case PropType::Bool:
            case PropType::Pointer:
                return { 4, 4 };
            case PropType::Str:
            case PropType::String:
            case PropType::Array:
                return { 12, 4 };
            case PropType::Map:
                return { 20, 4 };
            case PropType::Struct: {
                int32_t size = ue1PropertiesSize(property->structType);
                return { size, size == 2 ? 2 : (size >= 4 ? 4 : 1) };
            }
            case PropType::FixedArray:
                return { layoutOf(property->inner).elementSize * property->fixedCount, 4 };
            }
            return {};
        }
    }

    PropertyBase::PropertyBase(int propertyType)
        : type(propertyType)
    {
    }

    PropertyBase::PropertyBase(UEnum* enumType)
        : type(CptByte)
        , ref(enumType)
    {
    }

    PropertyBase::PropertyBase(UClass* cls, UClass* meta)
        : type(CptObject)
        , ref(cls)
        , metaClass(meta)
    {
    }

    PropertyBase::PropertyBase(UStruct* structType)
        : type(CptStruct)
        , ref(structType)
    {
    }

    PropertyBase::PropertyBase(UProperty* property)
    {
        bool dynamicArray = false;
        if (property->type == PropType::Array) {
            dynamicArray = true;
            property = property->inner;
        }
        switch (property->type) {
        case PropType::Byte:
            type = CptByte;
            ref = property->enumType;
            break;
        case PropType::Int:
            type = CptInt;
            break;
        case PropType::Bool:
            type = CptBool;
            break;
        case PropType::Float:
            type = CptFloat;
            break;
        case PropType::Class:
            type = CptObject;
            ref = property->objectClass;
            metaClass = property->metaClass;
            break;
        case PropType::Object:
            type = CptObject;
            ref = property->objectClass;
            break;
        case PropType::Name:
            type = CptName;
            break;
        case PropType::Str:
        case PropType::String:
            type = CptString;
            break;
        case PropType::Struct:
            type = CptStruct;
            ref = property->structType;
            break;
        case PropType::Array:
        case PropType::Map:
        case PropType::FixedArray:
        case PropType::Pointer:
            throw std::runtime_error("Unknown property type '" + property->pathName() + "'");
        }
        arrayDim = dynamicArray ? 0 : property->arrayDim;
        flags = property->propertyFlags;
    }

    UEnum* PropertyBase::enumType() const
    {
        return cast<UEnum>(ref);
    }

    UClass* PropertyBase::propertyClass() const
    {
        return cast<UClass>(ref);
    }

    UStruct* PropertyBase::structType() const
    {
        return cast<UStruct>(ref);
    }

    int32_t PropertyBase::size() const
    {
        static const int32_t elementSizes[CptMax] = { 0, 1, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0 };
        int32_t elementSize = type != CptStruct ? elementSizes[type] : ue1PropertiesSize(structType());
        return elementSize * arrayDim;
    }

    bool PropertyBase::isVector() const
    {
        return type == CptStruct && ref && iequals(ref->name.str(), "Vector");
    }

    bool PropertyBase::isRotator() const
    {
        return type == CptStruct && ref && iequals(ref->name.str(), "Rotator");
    }

    bool PropertyBase::matchesType(const PropertyBase& other, bool identity) const
    {
        if (flags & CPF_OutParm) {
            if ((other.flags & CPF_Const) || !(other.flags & CPF_OutParm)) {
                return false;
            }
            identity = true;
        }
        if (type == CptNone && (other.type == CptNone || !identity)) {
            return true;
        }
        if (type != other.type) {
            return false;
        }
        if (arrayDim != other.arrayDim) {
            return false;
        }
        if (type == CptByte) {
            return ref == other.ref || (ref == nullptr && !identity);
        }
        if (type == CptObject) {
            if (identity) {
                return ref == other.ref && metaClass == other.metaClass;
            }
            if (other.ref == nullptr) {
                return true;
            }
            if (other.propertyClass()->isChildOf(propertyClass())) {
                if (propertyClass() != objects::classClass() || (other.metaClass && metaClass && other.metaClass->isChildOf(metaClass))) {
                    return true;
                }
            }
            return false;
        }
        if (type == CptStruct) {
            if (identity) {
                return ref == other.ref;
            }
            return other.structType()->isChildOf(structType());
        }
        return true;
    }

    bool Lookup::earlier(const UObject* a, const UObject* b)
    {
        uint64_t orderA = a->linker ? a->linker->loadOrder : UINT64_MAX;
        uint64_t orderB = b->linker ? b->linker->loadOrder : UINT64_MAX;
        if (orderA != orderB) {
            return orderA < orderB;
        }
        if (a->linker) {
            return a->exportRef < b->exportRef;
        }
        return a->handle < b->handle;
    }

    void Lookup::index(UObject* object)
    {
        if (!object || object->destroyed) {
            return;
        }
        if (UClass* cls = cast<UClass>(object)) {
            auto [it, inserted] = classes.emplace(cls->name, cls);
            if (!inserted && (it->second->destroyed || earlier(cls, it->second))) {
                it->second = cls;
            }
        } else if (isPlainStruct(object)) {
            auto [it, inserted] = structs.emplace(object->name, static_cast<UStruct*>(object));
            if (!inserted && (it->second->destroyed || earlier(object, it->second))) {
                it->second = static_cast<UStruct*>(object);
            }
        }
    }

    void Lookup::refresh()
    {
        const std::vector<UObject*>& table = objects::all();
        for (; indexed < table.size(); indexed++) {
            index(table[indexed]);
        }
    }

    void Lookup::add(UObject* object)
    {
        refresh();
        index(object);
    }

    UClass* Lookup::findClass(std::string_view name)
    {
        refresh();
        size_t dot = name.rfind('.');
        if (dot != std::string_view::npos) {
            return cast<UClass>(findObject(objects::classClass(), name));
        }
        auto it = classes.find(Name::find(name));
        if (it == classes.end() || Name::find(name).none()) {
            return nullptr;
        }
        objects::ensureLoaded(it->second);
        return it->second;
    }

    UStruct* Lookup::findStruct(std::string_view name)
    {
        refresh();
        auto it = structs.find(Name::find(name));
        if (it == structs.end() || Name::find(name).none()) {
            return nullptr;
        }
        objects::ensureLoaded(it->second);
        return it->second;
    }

    UObject* Lookup::findPackage(std::string_view name)
    {
        Name packageName = Name::find(name);
        if (packageName.none()) {
            return nullptr;
        }
        for (UObject* object : objects::all()) {
            if (object && !object->destroyed && !object->outer && object->name == packageName) {
                return object;
            }
        }
        return nullptr;
    }

    UObject* Lookup::findObject(UClass* cls, std::string_view path)
    {
        std::vector<Name> parts;
        size_t start = 0;
        while (true) {
            size_t dot = path.find('.', start);
            Name part = Name::find(path.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start));
            if (part.none()) {
                return nullptr;
            }
            parts.push_back(part);
            if (dot == std::string_view::npos) {
                break;
            }
            start = dot + 1;
        }
        UObject* found = nullptr;
        if (parts.size() == 1) {
            for (UObject* object : objects::all()) {
                if (object && !object->destroyed && object->name == parts[0] && object->outer && object->isA(cls) && (!found || earlier(object, found))) {
                    found = object;
                }
            }
        } else {
            std::vector<UObject*> candidates;
            for (UObject* object : objects::all()) {
                if (object && !object->destroyed && !object->outer && object->name == parts[0]) {
                    candidates.push_back(object);
                }
            }
            for (size_t i = 1; i < parts.size(); i++) {
                std::vector<UObject*> next;
                for (UObject* object : objects::all()) {
                    if (object && !object->destroyed && object->name == parts[i] && std::find(candidates.begin(), candidates.end(), object->outer) != candidates.end()) {
                        next.push_back(object);
                    }
                }
                candidates = std::move(next);
            }
            for (UObject* candidate : candidates) {
                if (candidate->isA(cls) && (!found || earlier(candidate, found))) {
                    found = candidate;
                }
            }
        }
        if (found) {
            objects::ensureLoaded(found);
        }
        return found;
    }

    UClass* ownerClass(UObject* object)
    {
        for (UObject* current = object; current; current = current->outer) {
            if (UClass* cls = cast<UClass>(current)) {
                return cls;
            }
        }
        return nullptr;
    }

    bool isPlainStruct(const UObject* object)
    {
        return cast<UStruct>(object) && !cast<UFunction>(object) && !cast<UState>(object);
    }

    UStruct* inheritanceSuper(UStruct* structType)
    {
        if (cast<UFunction>(structType)) {
            return nullptr;
        }
        if (UClass* cls = cast<UClass>(structType)) {
            return cls->superClass();
        }
        if (cast<UState>(structType)) {
            return cast<UState>(structType->superField);
        }
        return structType->superStruct();
    }

    std::vector<UField*> fieldIterator(UStruct* structType)
    {
        std::vector<UField*> fields;
        for (UStruct* current = structType; current; current = inheritanceSuper(current)) {
            objects::ensureLoaded(current);
            for (UField* child = current->children; child; child = child->next) {
                objects::ensureLoaded(child);
                fields.push_back(child);
            }
        }
        return fields;
    }

    int32_t ue1PropertiesSize(UStruct* structType)
    {
        objects::ensureLoaded(structType);
        int32_t size = 0;
        int32_t alignment = 4;
        if (UStruct* parent = inheritanceSuper(structType)) {
            size = ue1PropertiesSize(parent);
        }
        bool mergeBools = !cast<UFunction>(structType);
        UProperty* previous = nullptr;
        int32_t previousOffset = 0;
        uint32_t previousMask = 0;
        for (UField* child = structType->children; child; child = child->next) {
            objects::ensureLoaded(child);
            if (child->outer != structType) {
                break;
            }
            UProperty* property = cast<UProperty>(child);
            if (!property) {
                continue;
            }
            Ue1Layout layout = layoutOf(property);
            int32_t offset = 0;
            if (property->type == PropType::Bool) {
                if (mergeBools && previous && previous->type == PropType::Bool && (previousMask << 1) != 0) {
                    offset = previousOffset;
                    previousMask <<= 1;
                } else {
                    offset = alignTo(size, 4);
                    previousMask = 1;
                }
            } else {
                offset = alignTo(size, layout.alignment);
            }
            size = offset + layout.elementSize * property->arrayDim;
            previous = property;
            previousOffset = offset;
        }
        return alignTo(size, alignment);
    }

    int32_t numParms(UFunction* function)
    {
        objects::ensureLoaded(function);
        int32_t count = 0;
        for (UField* child = function->children; child; child = child->next) {
            objects::ensureLoaded(child);
            UProperty* property = cast<UProperty>(child);
            if (property && (property->propertyFlags & CPF_Parm)) {
                count++;
            }
        }
        return count;
    }

    UProperty* returnProperty(UFunction* function)
    {
        objects::ensureLoaded(function);
        for (UField* child = function->children; child; child = child->next) {
            objects::ensureLoaded(child);
            UProperty* property = cast<UProperty>(child);
            if (property && (property->propertyFlags & CPF_ReturnParm)) {
                return property;
            }
        }
        return nullptr;
    }

    uint32_t classCrc(UClass* cls)
    {
        if (!cls->scriptText) {
            return 0;
        }
        static std::unordered_map<const UTextBuffer*, uint32_t> cache;
        auto it = cache.find(cls->scriptText);
        if (it != cache.end()) {
            return it->second;
        }
        objects::ensureLoaded(cls->scriptText);
        uint32_t crc = stringCrc(cls->scriptText->text);
        cache.emplace(cls->scriptText, crc);
        return crc;
    }

    void addDependency(UClass* cls, UClass* dependency, bool deep)
    {
        ClassDependency entry;
        entry.cls = dependency;
        entry.deep = deep ? 1u : 0u;
        entry.scriptTextCrc = dependency ? classCrc(dependency) : 0;
        cls->dependencies.push_back(entry);
    }
}
