#pragma once

#include "core/name.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dx {
    class UClass;
    class UStruct;
    class UFunction;
    class UState;
    class UProperty;
    class UEnum;
    class Linker;
    class Reader;
    struct Frame;
    struct StateFrame;

    constexpr uint32_t RF_Transactional = 0x00000001;
    constexpr uint32_t RF_Public = 0x00000004;
    constexpr uint32_t RF_Transient = 0x00004000;
    constexpr uint32_t RF_Standalone = 0x00080000;
    constexpr uint32_t RF_NotForClient = 0x00100000;
    constexpr uint32_t RF_NotForServer = 0x00200000;
    constexpr uint32_t RF_NotForEdit = 0x00400000;
    constexpr uint32_t RF_HasStackFlag = 0x02000000;
    constexpr uint32_t RF_Native = 0x04000000;

    constexpr uint32_t FUNC_Final = 0x00000001;
    constexpr uint32_t FUNC_Defined = 0x00000002;
    constexpr uint32_t FUNC_Iterator = 0x00000004;
    constexpr uint32_t FUNC_Latent = 0x00000008;
    constexpr uint32_t FUNC_PreOperator = 0x00000010;
    constexpr uint32_t FUNC_Singular = 0x00000020;
    constexpr uint32_t FUNC_Net = 0x00000040;
    constexpr uint32_t FUNC_NetReliable = 0x00000080;
    constexpr uint32_t FUNC_Simulated = 0x00000100;
    constexpr uint32_t FUNC_Exec = 0x00000200;
    constexpr uint32_t FUNC_Native = 0x00000400;
    constexpr uint32_t FUNC_Event = 0x00000800;
    constexpr uint32_t FUNC_Operator = 0x00001000;
    constexpr uint32_t FUNC_Static = 0x00002000;

    constexpr uint32_t CPF_Edit = 0x00000001;
    constexpr uint32_t CPF_Const = 0x00000002;
    constexpr uint32_t CPF_Input = 0x00000004;
    constexpr uint32_t CPF_ExportObject = 0x00000008;
    constexpr uint32_t CPF_OptionalParm = 0x00000010;
    constexpr uint32_t CPF_Net = 0x00000020;
    constexpr uint32_t CPF_Parm = 0x00000080;
    constexpr uint32_t CPF_OutParm = 0x00000100;
    constexpr uint32_t CPF_SkipParm = 0x00000200;
    constexpr uint32_t CPF_ReturnParm = 0x00000400;
    constexpr uint32_t CPF_CoerceParm = 0x00000800;
    constexpr uint32_t CPF_Native = 0x00001000;
    constexpr uint32_t CPF_Transient = 0x00002000;
    constexpr uint32_t CPF_Config = 0x00004000;
    constexpr uint32_t CPF_Localized = 0x00008000;
    constexpr uint32_t CPF_Travel = 0x00010000;
    constexpr uint32_t CPF_EditConst = 0x00020000;
    constexpr uint32_t CPF_GlobalConfig = 0x00040000;

    constexpr uint32_t STATE_Editable = 0x00000001;
    constexpr uint32_t STATE_Auto = 0x00000002;
    constexpr uint32_t STATE_Simulated = 0x00000004;

    constexpr uint32_t CLASS_Abstract = 0x00000001;
    constexpr uint32_t CLASS_Compiled = 0x00000002;
    constexpr uint32_t CLASS_Config = 0x00000004;
    constexpr uint32_t CLASS_Transient = 0x00000008;
    constexpr uint32_t CLASS_Parsed = 0x00000010;
    constexpr uint32_t CLASS_Localized = 0x00000020;
    constexpr uint32_t CLASS_SafeReplace = 0x00000040;
    constexpr uint32_t CLASS_RuntimeStatic = 0x00000080;
    constexpr uint32_t CLASS_NoExport = 0x00000100;
    constexpr uint32_t CLASS_NoUserCreate = 0x00000200;
    constexpr uint32_t CLASS_PerObjectConfig = 0x00000400;
    constexpr uint32_t CLASS_NativeReplication = 0x00000800;

    class UObject {
    public:
        UObject();
        virtual ~UObject();
        UObject(const UObject&) = delete;
        UObject& operator=(const UObject&) = delete;

        virtual void serialize(Reader& reader, Linker& linker);
        virtual void postLoad();

        bool isA(const UClass* other) const;
        bool isA(Name className) const;
        std::string fullName() const;
        std::string pathName() const;
        UObject* outermost();

        uint8_t* propertyAddress(Name propertyName, int32_t index = 0) const;

        Name name;
        UClass* cls = nullptr;
        UObject* outer = nullptr;
        uint32_t flags = 0;
        uint32_t handle = 0;
        uint8_t* data = nullptr;
        Linker* linker = nullptr;
        int32_t exportRef = 0;
        bool loaded = true;
        bool destroyed = false;
        bool singular = false;
        std::unique_ptr<StateFrame> stateFrame;
    };

    class UField : public UObject {
    public:
        void serialize(Reader& reader, Linker& linker) override;

        UField* superField = nullptr;
        UField* next = nullptr;
    };

    class UTextBuffer : public UObject {
    public:
        void serialize(Reader& reader, Linker& linker) override;

        std::string text;
    };

    class UEnum : public UField {
    public:
        void serialize(Reader& reader, Linker& linker) override;

        std::vector<Name> names;
    };

    class UConst : public UField {
    public:
        void serialize(Reader& reader, Linker& linker) override;

        std::string value;
    };

    using NativeFunction = void (*)(UObject* self, Frame& stack, void* result);

    class UStruct : public UField {
    public:
        void serialize(Reader& reader, Linker& linker) override;

        UStruct* superStruct() const;
        UProperty* findProperty(Name propertyName) const;
        std::vector<UProperty*> fieldOrder() const;
        bool isChildOf(const UStruct* other) const;
        void initValue(uint8_t* memory) const;
        void destroyValue(uint8_t* memory) const;
        void copyValue(uint8_t* destination, const uint8_t* source) const;
        bool equalsValue(const uint8_t* a, const uint8_t* b) const;
        void link();

        UTextBuffer* scriptText = nullptr;
        UField* children = nullptr;
        Name friendlyName;
        uint32_t line = 0;
        uint32_t textPos = 0;
        std::vector<uint8_t> script;
        std::vector<UProperty*> properties;
        std::vector<UProperty*> constructed;
        size_t size = 0;
        size_t alignment = 1;
        bool linked = false;

    private:
        mutable std::unordered_map<Name, UProperty*> propertyIndex;
        mutable size_t propertyIndexSize = 0;
        mutable UProperty* const* propertyIndexData = nullptr;
    };

    class UFunction : public UStruct {
    public:
        void serialize(Reader& reader, Linker& linker) override;
        void linkFunction();

        uint16_t nativeIndex = 0;
        uint8_t precedence = 0;
        uint32_t functionFlags = 0;
        uint16_t replicationOffset = 0;
        std::vector<UProperty*> params;
        UProperty* returnParm = nullptr;
        NativeFunction native = nullptr;
        std::vector<uint8_t> stub;
    };

    class UState : public UStruct {
    public:
        void serialize(Reader& reader, Linker& linker) override;
        void linkState();
        UFunction* findStateFunction(Name functionName) const;

        uint64_t probeMask = 0;
        uint64_t ignoreMask = 0;
        uint16_t labelTableOffset = 0;
        uint32_t stateFlags = 0;
        std::unordered_map<Name, UFunction*> functions;
    };

    struct ClassDependency {
        UClass* cls = nullptr;
        uint32_t deep = 0;
        uint32_t scriptTextCrc = 0;
    };

    class UClass : public UState {
    public:
        ~UClass() override;
        void serialize(Reader& reader, Linker& linker) override;
        void linkClass();
        UFunction* findFunction(Name functionName) const;
        UState* findState(Name stateName) const;
        UClass* superClass() const;

        uint32_t classFlags = 0;
        uint32_t guid[4] = {};
        std::vector<ClassDependency> dependencies;
        std::vector<Name> packageImports;
        UClass* within = nullptr;
        Name configName;
        UObject* defaultObject = nullptr;
        std::unordered_map<Name, UFunction*> vtable;
        std::unordered_map<Name, UState*> states;
        std::vector<UClass*> nativeChain;
        size_t defaultsOffset = 0;
    };

    enum class PropType : uint8_t {
        Byte,
        Int,
        Bool,
        Float,
        Object,
        Class,
        Name,
        Str,
        String,
        Struct,
        Array,
        Map,
        FixedArray,
        Pointer,
    };

    class UProperty : public UField {
    public:
        explicit UProperty(PropType propertyType);
        void serialize(Reader& reader, Linker& linker) override;

        size_t elementSize() const;
        size_t elementAlignment() const;
        size_t totalSize() const;
        bool needsConstruct() const;
        void construct(uint8_t* memory) const;
        void destroy(uint8_t* memory) const;
        void copy(uint8_t* destination, const uint8_t* source) const;
        bool equals(const uint8_t* a, const uint8_t* b) const;
        void constructAll(uint8_t* memory) const;
        void destroyAll(uint8_t* memory) const;
        void copyAll(uint8_t* destination, const uint8_t* source) const;
        std::string toText(const uint8_t* memory) const;
        bool fromText(uint8_t* memory, std::string_view text) const;
        void clear(uint8_t* memory) const;

        PropType type;
        int32_t arrayDim = 1;
        uint32_t propertyFlags = 0;
        Name category;
        uint16_t replicationOffset = 0;
        size_t offset = 0;
        UEnum* enumType = nullptr;
        UClass* objectClass = nullptr;
        UClass* metaClass = nullptr;
        UStruct* structType = nullptr;
        UProperty* inner = nullptr;
        UProperty* mapValue = nullptr;
        int32_t fixedCount = 0;
    };

    template <typename T>
    T* cast(UObject* object)
    {
        return dynamic_cast<T*>(object);
    }

    template <typename T>
    const T* cast(const UObject* object)
    {
        return dynamic_cast<const T*>(object);
    }
}
