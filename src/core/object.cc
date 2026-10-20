#include "core/object.h"

#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "engine/reader.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace dx {
    namespace {
        enum Token : uint8_t {
            TokLocalVariable = 0x00,
            TokInstanceVariable = 0x01,
            TokDefaultVariable = 0x02,
            TokReturn = 0x04,
            TokSwitch = 0x05,
            TokJump = 0x06,
            TokJumpIfNot = 0x07,
            TokStop = 0x08,
            TokAssert = 0x09,
            TokCase = 0x0A,
            TokNothing = 0x0B,
            TokLabelTable = 0x0C,
            TokGotoLabel = 0x0D,
            TokEatString = 0x0E,
            TokLet = 0x0F,
            TokDynArrayElement = 0x10,
            TokNew = 0x11,
            TokClassContext = 0x12,
            TokMetaCast = 0x13,
            TokLetBool = 0x14,
            TokUnknown15 = 0x15,
            TokEndFunctionParms = 0x16,
            TokSelf = 0x17,
            TokSkip = 0x18,
            TokContext = 0x19,
            TokArrayElement = 0x1A,
            TokVirtualFunction = 0x1B,
            TokFinalFunction = 0x1C,
            TokIntConst = 0x1D,
            TokFloatConst = 0x1E,
            TokStringConst = 0x1F,
            TokObjectConst = 0x20,
            TokNameConst = 0x21,
            TokRotationConst = 0x22,
            TokVectorConst = 0x23,
            TokByteConst = 0x24,
            TokIntZero = 0x25,
            TokIntOne = 0x26,
            TokTrue = 0x27,
            TokFalse = 0x28,
            TokNativeParm = 0x29,
            TokNoObject = 0x2A,
            TokUnknown2B = 0x2B,
            TokIntConstByte = 0x2C,
            TokBoolVariable = 0x2D,
            TokDynamicCast = 0x2E,
            TokIterator = 0x2F,
            TokIteratorPop = 0x30,
            TokIteratorNext = 0x31,
            TokStructCmpEq = 0x32,
            TokStructCmpNe = 0x33,
            TokUnicodeStringConst = 0x34,
            TokStructMember = 0x36,
            TokGlobalFunction = 0x38,
            TokMinConversion = 0x39,
            TokMaxConversion = 0x5A,
            TokExtendedNative = 0x60,
            TokFirstNative = 0x70,
        };

        void pushBytes(std::vector<uint8_t>& out, const void* bytes, size_t count)
        {
            const uint8_t* begin = static_cast<const uint8_t*>(bytes);
            out.insert(out.end(), begin, begin + count);
        }

        void pushU8(std::vector<uint8_t>& out, uint8_t value)
        {
            out.push_back(value);
        }

        void pushU16(std::vector<uint8_t>& out, uint16_t value)
        {
            pushBytes(out, &value, sizeof(value));
        }

        void pushU32(std::vector<uint8_t>& out, uint32_t value)
        {
            pushBytes(out, &value, sizeof(value));
        }

        void pushObject(std::vector<uint8_t>& out, Reader& reader, Linker& source)
        {
            UObject* object = source.readObject(reader);
            pushU32(out, object ? object->handle : 0);
        }

        void pushName(std::vector<uint8_t>& out, Reader& reader, Linker& source)
        {
            pushU32(out, source.readName(reader).index());
        }

        uint8_t readToken(std::vector<uint8_t>& out, Reader& reader, Linker& source, int depth)
        {
            if (depth > 64) {
                throw std::runtime_error("bytecode nesting too deep");
            }
            depth++;
            uint8_t token = reader.u8();
            pushU8(out, token);

            if (token >= TokMinConversion && token <= TokMaxConversion) {
                readToken(out, reader, source, depth);
                return token;
            }
            if (token >= TokFirstNative) {
                while (readToken(out, reader, source, depth) != TokEndFunctionParms) {
                }
                return token;
            }
            if (token >= TokExtendedNative) {
                pushU8(out, reader.u8());
                while (readToken(out, reader, source, depth) != TokEndFunctionParms) {
                }
                return token;
            }

            switch (token) {
            case TokLocalVariable:
            case TokInstanceVariable:
            case TokDefaultVariable:
            case TokObjectConst:
            case TokNativeParm:
                pushObject(out, reader, source);
                break;
            case TokReturn:
                readToken(out, reader, source, depth);
                break;
            case TokSwitch:
                pushU8(out, reader.u8());
                readToken(out, reader, source, depth);
                break;
            case TokJump:
                pushU16(out, static_cast<uint16_t>(reader.i16()));
                break;
            case TokJumpIfNot:
            case TokAssert:
            case TokSkip:
                pushU16(out, static_cast<uint16_t>(reader.i16()));
                readToken(out, reader, source, depth);
                break;
            case TokCase: {
                uint16_t next = static_cast<uint16_t>(reader.i16());
                pushU16(out, next);
                if (next != 0xffff) {
                    readToken(out, reader, source, depth);
                }
                break;
            }
            case TokLabelTable:
                while (true) {
                    Name label = source.readName(reader);
                    pushU32(out, label.index());
                    pushU32(out, reader.u32());
                    if (label.none()) {
                        break;
                    }
                }
                break;
            case TokGotoLabel:
            case TokEatString:
            case TokBoolVariable:
                readToken(out, reader, source, depth);
                break;
            case TokLet:
            case TokLetBool:
            case TokDynArrayElement:
            case TokArrayElement:
                readToken(out, reader, source, depth);
                readToken(out, reader, source, depth);
                break;
            case TokNew:
                for (int i = 0; i < 4; i++) {
                    readToken(out, reader, source, depth);
                }
                break;
            case TokClassContext:
            case TokContext:
                readToken(out, reader, source, depth);
                pushU16(out, static_cast<uint16_t>(reader.i16()));
                pushU8(out, reader.u8());
                readToken(out, reader, source, depth);
                break;
            case TokMetaCast:
            case TokDynamicCast:
            case TokStructMember:
                pushObject(out, reader, source);
                readToken(out, reader, source, depth);
                break;
            case TokStructCmpEq:
            case TokStructCmpNe:
                pushObject(out, reader, source);
                readToken(out, reader, source, depth);
                readToken(out, reader, source, depth);
                break;
            case TokVirtualFunction:
            case TokGlobalFunction:
                pushName(out, reader, source);
                while (readToken(out, reader, source, depth) != TokEndFunctionParms) {
                }
                break;
            case TokFinalFunction:
                pushObject(out, reader, source);
                while (readToken(out, reader, source, depth) != TokEndFunctionParms) {
                }
                break;
            case TokIntConst:
            case TokFloatConst:
                pushU32(out, reader.u32());
                break;
            case TokStringConst:
                while (true) {
                    uint8_t c = reader.u8();
                    pushU8(out, c);
                    if (c == 0) {
                        break;
                    }
                }
                break;
            case TokUnicodeStringConst:
                while (true) {
                    uint16_t c = static_cast<uint16_t>(reader.i16());
                    pushU16(out, c);
                    if (c == 0) {
                        break;
                    }
                }
                break;
            case TokNameConst:
                pushName(out, reader, source);
                break;
            case TokRotationConst:
            case TokVectorConst:
                pushU32(out, reader.u32());
                pushU32(out, reader.u32());
                pushU32(out, reader.u32());
                break;
            case TokByteConst:
            case TokIntConstByte:
                pushU8(out, reader.u8());
                break;
            case TokUnknown2B:
                pushU8(out, reader.u8());
                readToken(out, reader, source, depth);
                break;
            case TokIterator:
                readToken(out, reader, source, depth);
                pushU16(out, static_cast<uint16_t>(reader.i16()));
                break;
            case TokStop:
            case TokNothing:
            case TokUnknown15:
            case TokEndFunctionParms:
            case TokSelf:
            case TokIntZero:
            case TokIntOne:
            case TokTrue:
            case TokFalse:
            case TokNoObject:
            case TokIteratorPop:
            case TokIteratorNext:
                break;
            default:
                throw std::runtime_error("unknown bytecode token " + std::to_string(token));
            }
            return token;
        }

        size_t alignUp(size_t value, size_t alignment)
        {
            return (value + alignment - 1) / alignment * alignment;
        }
    }

    UObject::UObject() = default;

    UObject::~UObject()
    {
        if (data && cls) {
            cls->destroyValue(data);
        }
        delete[] data;
    }

    void UObject::serialize(Reader&, Linker&)
    {
    }

    void UObject::postLoad()
    {
    }

    bool UObject::isA(const UClass* other) const
    {
        for (const UClass* current = cls; current; current = current->superClass()) {
            if (current == other) {
                return true;
            }
        }
        return false;
    }

    bool UObject::isA(Name className) const
    {
        for (const UClass* current = cls; current; current = current->superClass()) {
            if (current->name == className) {
                return true;
            }
        }
        return false;
    }

    std::string UObject::pathName() const
    {
        if (!outer) {
            return name.str();
        }
        return outer->pathName() + "." + name.str();
    }

    std::string UObject::fullName() const
    {
        return (cls ? cls->name.str() : std::string("None")) + " " + pathName();
    }

    UObject* UObject::outermost()
    {
        UObject* current = this;
        while (current->outer) {
            current = current->outer;
        }
        return current;
    }

    uint8_t* UObject::propertyAddress(Name propertyName, int32_t index) const
    {
        if (!data || !cls) {
            return nullptr;
        }
        UProperty* property = cls->findProperty(propertyName);
        if (!property || index < 0 || index >= property->arrayDim) {
            return nullptr;
        }
        return data + property->offset + static_cast<size_t>(index) * property->elementSize();
    }

    void UField::serialize(Reader& reader, Linker& source)
    {
        superField = cast<UField>(source.readObject(reader));
        next = cast<UField>(source.readObject(reader));
    }

    void UTextBuffer::serialize(Reader& reader, Linker&)
    {
        reader.u32();
        reader.u32();
        text = reader.string();
    }

    void UEnum::serialize(Reader& reader, Linker& source)
    {
        UField::serialize(reader, source);
        int32_t count = reader.index();
        for (int32_t i = 0; i < count; i++) {
            names.push_back(source.readName(reader));
        }
    }

    void UConst::serialize(Reader& reader, Linker& source)
    {
        UField::serialize(reader, source);
        value = reader.string();
    }

    void UStruct::serialize(Reader& reader, Linker& source)
    {
        UField::serialize(reader, source);
        scriptText = cast<UTextBuffer>(source.readObject(reader));
        children = cast<UField>(source.readObject(reader));
        friendlyName = source.readName(reader);
        line = reader.u32();
        textPos = reader.u32();
        uint32_t scriptSize = reader.u32();
        script.reserve(scriptSize);
        while (script.size() < scriptSize) {
            readToken(script, reader, source, 0);
        }
        if (script.size() != scriptSize) {
            throw std::runtime_error("bytecode size mismatch in " + pathName());
        }
    }

    UStruct* UStruct::superStruct() const
    {
        return cast<UStruct>(superField);
    }

    UProperty* UStruct::findProperty(Name propertyName) const
    {
        if (propertyIndexSize != properties.size() || propertyIndexData != properties.data()) {
            propertyIndex.clear();
            for (UProperty* property : properties) {
                propertyIndex[property->name] = property;
            }
            propertyIndexSize = properties.size();
            propertyIndexData = properties.data();
        }
        auto it = propertyIndex.find(propertyName);
        return it == propertyIndex.end() ? nullptr : it->second;
    }

    std::vector<UProperty*> UStruct::fieldOrder() const
    {
        std::vector<UProperty*> order;
        for (const UStruct* current = this; current; current = current->superStruct()) {
            for (UField* child = current->children; child; child = child->next) {
                if (UProperty* property = cast<UProperty>(child)) {
                    order.push_back(property);
                }
            }
        }
        return order;
    }

    bool UStruct::isChildOf(const UStruct* other) const
    {
        for (const UStruct* current = this; current; current = current->superStruct()) {
            if (current == other) {
                return true;
            }
        }
        return false;
    }

    void UStruct::initValue(uint8_t* memory) const
    {
        std::memset(memory, 0, size);
        for (UProperty* property : constructed) {
            property->constructAll(memory + property->offset);
        }
    }

    void UStruct::destroyValue(uint8_t* memory) const
    {
        for (UProperty* property : constructed) {
            property->destroyAll(memory + property->offset);
        }
    }

    void UStruct::copyValue(uint8_t* destination, const uint8_t* source) const
    {
        if (constructed.empty()) {
            std::memcpy(destination, source, size);
            return;
        }
        for (UProperty* property : properties) {
            property->copyAll(destination + property->offset, source + property->offset);
        }
    }

    bool UStruct::equalsValue(const uint8_t* a, const uint8_t* b) const
    {
        for (UProperty* property : properties) {
            for (int32_t i = 0; i < property->arrayDim; i++) {
                size_t at = property->offset + static_cast<size_t>(i) * property->elementSize();
                if (!property->equals(a + at, b + at)) {
                    return false;
                }
            }
        }
        return true;
    }

    void UStruct::link()
    {
        if (linked) {
            return;
        }
        linked = true;

        bool isFunction = cast<UFunction>(this) != nullptr;
        bool isState = cast<UState>(this) != nullptr && cast<UClass>(this) == nullptr;
        UStruct* parent = superStruct();
        if (parent && !isFunction && !isState) {
            objects::ensureLoaded(parent);
            parent->link();
            properties = parent->properties;
            size = parent->size;
            alignment = parent->alignment;
        }

        for (UField* child = children; child; child = child->next) {
            objects::ensureLoaded(child);
            UProperty* property = cast<UProperty>(child);
            if (!property) {
                continue;
            }
            if (property->type == PropType::Struct && property->structType) {
                objects::ensureLoaded(property->structType);
                property->structType->link();
            }
            size_t align = property->elementAlignment();
            property->offset = alignUp(size, align);
            size = property->offset + property->totalSize();
            alignment = std::max(alignment, align);
            properties.push_back(property);
        }
        size = alignUp(size, alignment);

        constructed.clear();
        for (UProperty* property : properties) {
            if (property->needsConstruct()) {
                constructed.push_back(property);
            }
        }

        if (UFunction* function = cast<UFunction>(this)) {
            function->linkFunction();
        }
    }

    void UFunction::serialize(Reader& reader, Linker& source)
    {
        UStruct::serialize(reader, source);
        nativeIndex = static_cast<uint16_t>(reader.i16());
        precedence = reader.u8();
        functionFlags = reader.u32();
        if (functionFlags & FUNC_Net) {
            replicationOffset = static_cast<uint16_t>(reader.i16());
        }
    }

    void UFunction::linkFunction()
    {
        params.clear();
        returnParm = nullptr;
        for (UProperty* property : properties) {
            if (property->propertyFlags & CPF_ReturnParm) {
                returnParm = property;
            } else if (property->propertyFlags & CPF_Parm) {
                params.push_back(property);
            }
        }
        stub.clear();
        for (UProperty* property : params) {
            stub.push_back(0x00);
            uint32_t propertyHandle = property->handle;
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&propertyHandle);
            stub.insert(stub.end(), bytes, bytes + 4);
        }
        stub.push_back(0x16);
        if (functionFlags & FUNC_Native) {
            native = vm::bindNative(this);
        }
    }

    void UState::serialize(Reader& reader, Linker& source)
    {
        UStruct::serialize(reader, source);
        probeMask = reader.u64();
        ignoreMask = reader.u64();
        labelTableOffset = static_cast<uint16_t>(reader.i16());
        stateFlags = reader.u32();
    }

    void UState::linkState()
    {
        functions.clear();
        UState* parent = cast<UState>(superField);
        if (parent && !cast<UClass>(this)) {
            objects::ensureLoaded(parent);
            UClass* parentClass = cast<UClass>(parent->outer);
            if (parentClass) {
                objects::ensureLoaded(parentClass);
            }
            parent->linkState();
            functions = parent->functions;
        }
        for (UField* child = children; child; child = child->next) {
            objects::ensureLoaded(child);
            if (UFunction* function = cast<UFunction>(child)) {
                function->link();
                functions[function->name] = function;
            }
        }
    }

    UFunction* UState::findStateFunction(Name functionName) const
    {
        auto it = functions.find(functionName);
        if (it == functions.end()) {
            return nullptr;
        }
        return it->second;
    }

    UClass::~UClass()
    {
        if (defaultObject) {
            delete defaultObject;
        }
    }

    void UClass::serialize(Reader& reader, Linker& source)
    {
        UState::serialize(reader, source);
        classFlags = reader.u32();
        for (uint32_t& part : guid) {
            part = reader.u32();
        }
        int32_t dependencyCount = reader.index();
        dependencies.clear();
        for (int32_t i = 0; i < dependencyCount; i++) {
            ClassDependency dependency;
            if (objects::editorMode()) {
                dependency.cls = cast<UClass>(source.readObject(reader));
            } else {
                reader.index();
            }
            dependency.deep = reader.u32();
            dependency.scriptTextCrc = reader.u32();
            dependencies.push_back(dependency);
        }
        int32_t importCount = reader.index();
        packageImports.clear();
        for (int32_t i = 0; i < importCount; i++) {
            packageImports.push_back(source.readName(reader));
        }
        within = cast<UClass>(source.readObject(reader));
        configName = source.readName(reader);

        linkClass();
        objects::readTaggedProperties(this, defaultObject->data, reader, source);
        config::applyClassDefaults(this);
    }

    void UClass::linkClass()
    {
        if (defaultObject) {
            return;
        }
        UClass* parent = superClass();
        if (parent) {
            objects::ensureLoaded(parent);
            parent->linkClass();
        }
        link();

        vtable.clear();
        states.clear();
        if (parent) {
            vtable = parent->vtable;
            states = parent->states;
        }
        for (UField* child = children; child; child = child->next) {
            objects::ensureLoaded(child);
            if (UFunction* function = cast<UFunction>(child)) {
                function->link();
                vtable[function->name] = function;
            }
        }
        for (UField* child = children; child; child = child->next) {
            if (UState* state = cast<UState>(child)) {
                if (cast<UFunction>(child)) {
                    continue;
                }
                state->link();
                state->linkState();
                states[state->name] = state;
            }
        }
        for (UField* child = children; child; child = child->next) {
            if (UStruct* structType = cast<UStruct>(child)) {
                if (!cast<UFunction>(child) && !cast<UState>(child)) {
                    structType->link();
                }
            }
        }

        defaultObject = objects::createNative(this);
        defaultObject->name = Name("Default__" + name.str());
        defaultObject->cls = this;
        defaultObject->outer = outer;
        defaultObject->flags = RF_Public;
        defaultObject->data = new uint8_t[std::max<size_t>(size, 1)];
        initValue(defaultObject->data);
        if (parent && parent->defaultObject) {
            parent->copyValue(defaultObject->data, parent->defaultObject->data);
        }
        objects::registerObject(defaultObject);
        objects::syncObjectProperties(defaultObject);
    }

    UFunction* UClass::findFunction(Name functionName) const
    {
        auto it = vtable.find(functionName);
        if (it == vtable.end()) {
            return nullptr;
        }
        return it->second;
    }

    UState* UClass::findState(Name stateName) const
    {
        auto it = states.find(stateName);
        if (it == states.end()) {
            return nullptr;
        }
        return it->second;
    }

    UClass* UClass::superClass() const
    {
        return static_cast<UClass*>(superField);
    }

    UProperty::UProperty(PropType propertyType)
        : type(propertyType)
    {
    }

    void UProperty::serialize(Reader& reader, Linker& source)
    {
        UField::serialize(reader, source);
        arrayDim = reader.i32();
        if (arrayDim < 1) {
            arrayDim = 1;
        }
        propertyFlags = reader.u32();
        category = source.readName(reader);
        if (propertyFlags & CPF_Net) {
            replicationOffset = static_cast<uint16_t>(reader.i16());
        }
        switch (type) {
        case PropType::Byte:
            enumType = cast<UEnum>(source.readObject(reader));
            break;
        case PropType::Object:
            objectClass = cast<UClass>(source.readObject(reader));
            break;
        case PropType::Class:
            objectClass = cast<UClass>(source.readObject(reader));
            metaClass = cast<UClass>(source.readObject(reader));
            break;
        case PropType::Struct:
            structType = cast<UStruct>(source.readObject(reader));
            break;
        case PropType::Array:
            inner = cast<UProperty>(source.readObject(reader));
            break;
        case PropType::FixedArray:
            inner = cast<UProperty>(source.readObject(reader));
            fixedCount = reader.i32();
            break;
        case PropType::Map:
            inner = cast<UProperty>(source.readObject(reader));
            mapValue = cast<UProperty>(source.readObject(reader));
            break;
        case PropType::Int:
        case PropType::Bool:
        case PropType::Float:
        case PropType::Name:
        case PropType::Str:
        case PropType::String:
        case PropType::Pointer:
            break;
        }
    }

    size_t UProperty::elementSize() const
    {
        switch (type) {
        case PropType::Byte:
            return 1;
        case PropType::Int:
        case PropType::Bool:
        case PropType::Float:
        case PropType::Name:
        case PropType::Pointer:
            return 4;
        case PropType::Object:
        case PropType::Class:
            return sizeof(UObject*);
        case PropType::Str:
        case PropType::String:
            return sizeof(std::string);
        case PropType::Struct:
            return structType ? structType->size : 0;
        case PropType::Array:
            return sizeof(std::vector<uint8_t>);
        case PropType::Map:
            return sizeof(void*);
        case PropType::FixedArray:
            return inner ? inner->elementSize() * static_cast<size_t>(fixedCount) : 0;
        }
        return 0;
    }

    size_t UProperty::elementAlignment() const
    {
        switch (type) {
        case PropType::Byte:
            return 1;
        case PropType::Int:
        case PropType::Bool:
        case PropType::Float:
        case PropType::Name:
        case PropType::Pointer:
            return 4;
        case PropType::Object:
        case PropType::Class:
        case PropType::Map:
            return alignof(void*);
        case PropType::Str:
        case PropType::String:
            return alignof(std::string);
        case PropType::Array:
            return alignof(std::vector<uint8_t>);
        case PropType::Struct:
            return structType ? structType->alignment : 1;
        case PropType::FixedArray:
            return inner ? inner->elementAlignment() : 1;
        }
        return 1;
    }

    size_t UProperty::totalSize() const
    {
        return elementSize() * static_cast<size_t>(arrayDim);
    }

    bool UProperty::needsConstruct() const
    {
        switch (type) {
        case PropType::Str:
        case PropType::String:
        case PropType::Array:
            return true;
        case PropType::Struct:
            return structType && !structType->constructed.empty();
        case PropType::FixedArray:
            return inner && inner->needsConstruct();
        case PropType::Byte:
        case PropType::Int:
        case PropType::Bool:
        case PropType::Float:
        case PropType::Object:
        case PropType::Class:
        case PropType::Name:
        case PropType::Map:
        case PropType::Pointer:
            return false;
        }
        return false;
    }

    void UProperty::construct(uint8_t* memory) const
    {
        switch (type) {
        case PropType::Str:
        case PropType::String:
            new (memory) std::string();
            break;
        case PropType::Array:
            new (memory) std::vector<uint8_t>();
            break;
        case PropType::Struct:
            structType->initValue(memory);
            break;
        case PropType::FixedArray:
            for (int32_t i = 0; i < fixedCount; i++) {
                inner->construct(memory + static_cast<size_t>(i) * inner->elementSize());
            }
            break;
        case PropType::Byte:
        case PropType::Int:
        case PropType::Bool:
        case PropType::Float:
        case PropType::Object:
        case PropType::Class:
        case PropType::Name:
        case PropType::Map:
        case PropType::Pointer:
            std::memset(memory, 0, elementSize());
            break;
        }
    }

    void UProperty::destroy(uint8_t* memory) const
    {
        switch (type) {
        case PropType::Str:
        case PropType::String:
            reinterpret_cast<std::string*>(memory)->~basic_string();
            break;
        case PropType::Array:
            reinterpret_cast<std::vector<uint8_t>*>(memory)->~vector();
            break;
        case PropType::Struct:
            structType->destroyValue(memory);
            break;
        case PropType::FixedArray:
            for (int32_t i = 0; i < fixedCount; i++) {
                inner->destroy(memory + static_cast<size_t>(i) * inner->elementSize());
            }
            break;
        case PropType::Byte:
        case PropType::Int:
        case PropType::Bool:
        case PropType::Float:
        case PropType::Object:
        case PropType::Class:
        case PropType::Name:
        case PropType::Map:
        case PropType::Pointer:
            break;
        }
    }

    void UProperty::copy(uint8_t* destination, const uint8_t* source) const
    {
        if (destination == source) {
            return;
        }
        switch (type) {
        case PropType::Str:
        case PropType::String:
            *reinterpret_cast<std::string*>(destination) = *reinterpret_cast<const std::string*>(source);
            break;
        case PropType::Array:
            *reinterpret_cast<std::vector<uint8_t>*>(destination) = *reinterpret_cast<const std::vector<uint8_t>*>(source);
            break;
        case PropType::Struct:
            structType->copyValue(destination, source);
            break;
        case PropType::FixedArray:
            for (int32_t i = 0; i < fixedCount; i++) {
                size_t at = static_cast<size_t>(i) * inner->elementSize();
                inner->copy(destination + at, source + at);
            }
            break;
        case PropType::Byte:
        case PropType::Int:
        case PropType::Bool:
        case PropType::Float:
        case PropType::Object:
        case PropType::Class:
        case PropType::Name:
        case PropType::Map:
        case PropType::Pointer:
            std::memcpy(destination, source, elementSize());
            break;
        }
    }

    bool UProperty::equals(const uint8_t* a, const uint8_t* b) const
    {
        switch (type) {
        case PropType::Str:
        case PropType::String:
            return *reinterpret_cast<const std::string*>(a) == *reinterpret_cast<const std::string*>(b);
        case PropType::Array:
            return *reinterpret_cast<const std::vector<uint8_t>*>(a) == *reinterpret_cast<const std::vector<uint8_t>*>(b);
        case PropType::Struct:
            return structType->equalsValue(a, b);
        case PropType::Bool:
            return (*reinterpret_cast<const uint32_t*>(a) != 0) == (*reinterpret_cast<const uint32_t*>(b) != 0);
        case PropType::FixedArray:
            for (int32_t i = 0; i < fixedCount; i++) {
                size_t at = static_cast<size_t>(i) * inner->elementSize();
                if (!inner->equals(a + at, b + at)) {
                    return false;
                }
            }
            return true;
        case PropType::Byte:
        case PropType::Int:
        case PropType::Float:
        case PropType::Object:
        case PropType::Class:
        case PropType::Name:
        case PropType::Map:
        case PropType::Pointer:
            return std::memcmp(a, b, elementSize()) == 0;
        }
        return false;
    }

    void UProperty::constructAll(uint8_t* memory) const
    {
        for (int32_t i = 0; i < arrayDim; i++) {
            construct(memory + static_cast<size_t>(i) * elementSize());
        }
    }

    void UProperty::destroyAll(uint8_t* memory) const
    {
        for (int32_t i = 0; i < arrayDim; i++) {
            destroy(memory + static_cast<size_t>(i) * elementSize());
        }
    }

    void UProperty::copyAll(uint8_t* destination, const uint8_t* source) const
    {
        for (int32_t i = 0; i < arrayDim; i++) {
            size_t at = static_cast<size_t>(i) * elementSize();
            copy(destination + at, source + at);
        }
    }

    void UProperty::clear(uint8_t* memory) const
    {
        destroy(memory);
        construct(memory);
    }

    std::string UProperty::toText(const uint8_t* memory) const
    {
        char buffer[64];
        switch (type) {
        case PropType::Byte:
            if (enumType && *memory < enumType->names.size()) {
                return enumType->names[*memory].str();
            }
            return std::to_string(*memory);
        case PropType::Int:
            return std::to_string(*reinterpret_cast<const int32_t*>(memory));
        case PropType::Bool:
            return *reinterpret_cast<const uint32_t*>(memory) ? "True" : "False";
        case PropType::Float:
            std::snprintf(buffer, sizeof(buffer), "%.6f", static_cast<double>(*reinterpret_cast<const float*>(memory)));
            return buffer;
        case PropType::Object:
        case PropType::Class: {
            const UObject* object = *reinterpret_cast<UObject* const*>(memory);
            if (!object) {
                return "None";
            }
            return object->cls->name.str() + "'" + object->pathName() + "'";
        }
        case PropType::Name:
            return Name::fromIndex(*reinterpret_cast<const uint32_t*>(memory)).str();
        case PropType::Str:
        case PropType::String:
            return *reinterpret_cast<const std::string*>(memory);
        case PropType::Struct: {
            std::string text = "(";
            bool first = true;
            for (UProperty* property : structType->properties) {
                for (int32_t i = 0; i < property->arrayDim; i++) {
                    if (!first) {
                        text += ",";
                    }
                    first = false;
                    text += property->name.str();
                    if (property->arrayDim > 1) {
                        text += "[" + std::to_string(i) + "]";
                    }
                    text += "=";
                    std::string value = property->toText(memory + property->offset + static_cast<size_t>(i) * property->elementSize());
                    if (property->type == PropType::Str || property->type == PropType::String) {
                        text += "\"" + value + "\"";
                    } else {
                        text += value;
                    }
                }
            }
            return text + ")";
        }
        case PropType::Array:
        case PropType::Map:
        case PropType::FixedArray:
        case PropType::Pointer:
            return "";
        }
        return "";
    }

    bool UProperty::fromText(uint8_t* memory, std::string_view text) const
    {
        std::string value(text);
        switch (type) {
        case PropType::Byte:
            if (enumType) {
                for (size_t i = 0; i < enumType->names.size(); i++) {
                    if (iequals(enumType->names[i].str(), value)) {
                        *memory = static_cast<uint8_t>(i);
                        return true;
                    }
                }
            }
            *memory = static_cast<uint8_t>(std::atoi(value.c_str()));
            return true;
        case PropType::Int:
            *reinterpret_cast<int32_t*>(memory) = std::atoi(value.c_str());
            return true;
        case PropType::Bool:
            *reinterpret_cast<uint32_t*>(memory) = (iequals(value, "true") || value == "1" || iequals(value, "yes")) ? 1 : 0;
            return true;
        case PropType::Float:
            *reinterpret_cast<float*>(memory) = static_cast<float>(std::atof(value.c_str()));
            return true;
        case PropType::Object:
        case PropType::Class: {
            if (iequals(value, "None") || value.empty()) {
                *reinterpret_cast<UObject**>(memory) = nullptr;
                return true;
            }
            std::string path = value;
            size_t quote = value.find('\'');
            UClass* expected = type == PropType::Class ? objects::classClass() : objectClass;
            if (quote != std::string::npos) {
                std::string className = value.substr(0, quote);
                path = value.substr(quote + 1);
                if (!path.empty() && path.back() == '\'') {
                    path.pop_back();
                }
                UClass* named = objects::findClass(className);
                if (named) {
                    expected = named;
                }
            }
            UObject* object = type == PropType::Class ? objects::findClass(path) : objects::loadObject(path, expected);
            *reinterpret_cast<UObject**>(memory) = object;
            return object != nullptr;
        }
        case PropType::Name:
            *reinterpret_cast<uint32_t*>(memory) = Name(value).index();
            return true;
        case PropType::Str:
        case PropType::String:
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            *reinterpret_cast<std::string*>(memory) = value;
            return true;
        case PropType::Struct: {
            size_t pos = 0;
            while (pos < value.size() && (value[pos] == '(' || value[pos] == ' ')) {
                pos++;
            }
            while (pos < value.size()) {
                size_t equals = value.find('=', pos);
                if (equals == std::string::npos) {
                    break;
                }
                std::string key = value.substr(pos, equals - pos);
                while (!key.empty() && (key.front() == ' ' || key.front() == ',')) {
                    key.erase(key.begin());
                }
                int32_t index = 0;
                size_t bracket = key.find('[');
                if (bracket != std::string::npos) {
                    index = std::atoi(key.c_str() + bracket + 1);
                    key = key.substr(0, bracket);
                }
                size_t end = equals + 1;
                int depth = 0;
                bool quoted = false;
                while (end < value.size()) {
                    char c = value[end];
                    if (c == '"') {
                        quoted = !quoted;
                    } else if (!quoted && c == '(') {
                        depth++;
                    } else if (!quoted && c == ')') {
                        if (depth == 0) {
                            break;
                        }
                        depth--;
                    } else if (!quoted && c == ',' && depth == 0) {
                        break;
                    }
                    end++;
                }
                std::string item = value.substr(equals + 1, end - equals - 1);
                UProperty* member = structType->findProperty(Name(key));
                if (member && index >= 0 && index < member->arrayDim) {
                    member->fromText(memory + member->offset + static_cast<size_t>(index) * member->elementSize(), item);
                }
                pos = end + 1;
            }
            return true;
        }
        case PropType::Array:
        case PropType::Map:
        case PropType::FixedArray:
        case PropType::Pointer:
            return false;
        }
        return false;
    }
}
