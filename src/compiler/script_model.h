#pragma once

#include "core/object.h"

#include <climits>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace dx::compiler {
    enum Cpt : int {
        CptNone = 0,
        CptByte = 1,
        CptInt = 2,
        CptBool = 3,
        CptFloat = 4,
        CptObject = 5,
        CptName = 6,
        CptStruct = 10,
        CptVector = 11,
        CptRotation = 12,
        CptString = 13,
        CptMax = 14,
    };

    enum Ex : uint8_t {
        ExLocalVariable = 0x00,
        ExInstanceVariable = 0x01,
        ExDefaultVariable = 0x02,
        ExReturn = 0x04,
        ExSwitch = 0x05,
        ExJump = 0x06,
        ExJumpIfNot = 0x07,
        ExStop = 0x08,
        ExAssert = 0x09,
        ExCase = 0x0a,
        ExNothing = 0x0b,
        ExLabelTable = 0x0c,
        ExGotoLabel = 0x0d,
        ExEatString = 0x0e,
        ExLet = 0x0f,
        ExDynArrayElement = 0x10,
        ExNew = 0x11,
        ExClassContext = 0x12,
        ExMetaCast = 0x13,
        ExLetBool = 0x14,
        ExEndFunctionParms = 0x16,
        ExSelf = 0x17,
        ExSkip = 0x18,
        ExContext = 0x19,
        ExArrayElement = 0x1a,
        ExVirtualFunction = 0x1b,
        ExFinalFunction = 0x1c,
        ExIntConst = 0x1d,
        ExFloatConst = 0x1e,
        ExStringConst = 0x1f,
        ExObjectConst = 0x20,
        ExNameConst = 0x21,
        ExRotationConst = 0x22,
        ExVectorConst = 0x23,
        ExByteConst = 0x24,
        ExIntZero = 0x25,
        ExIntOne = 0x26,
        ExTrue = 0x27,
        ExFalse = 0x28,
        ExNativeParm = 0x29,
        ExNoObject = 0x2a,
        ExIntConstByte = 0x2c,
        ExBoolVariable = 0x2d,
        ExDynamicCast = 0x2e,
        ExIterator = 0x2f,
        ExIteratorPop = 0x30,
        ExIteratorNext = 0x31,
        ExStructCmpEq = 0x32,
        ExStructCmpNe = 0x33,
        ExUnicodeStringConst = 0x34,
        ExStructMember = 0x36,
        ExGlobalFunction = 0x38,
        ExRotatorToVector = 0x39,
        ExByteToInt = 0x3a,
        ExByteToBool = 0x3b,
        ExByteToFloat = 0x3c,
        ExIntToByte = 0x3d,
        ExIntToBool = 0x3e,
        ExIntToFloat = 0x3f,
        ExBoolToByte = 0x40,
        ExBoolToInt = 0x41,
        ExBoolToFloat = 0x42,
        ExFloatToByte = 0x43,
        ExFloatToInt = 0x44,
        ExFloatToBool = 0x45,
        ExObjectToBool = 0x47,
        ExNameToBool = 0x48,
        ExStringToByte = 0x49,
        ExStringToInt = 0x4a,
        ExStringToBool = 0x4b,
        ExStringToFloat = 0x4c,
        ExStringToVector = 0x4d,
        ExStringToRotator = 0x4e,
        ExVectorToBool = 0x4f,
        ExVectorToRotator = 0x50,
        ExRotatorToBool = 0x51,
        ExByteToString = 0x52,
        ExIntToString = 0x53,
        ExBoolToString = 0x54,
        ExFloatToString = 0x55,
        ExObjectToString = 0x56,
        ExNameToString = 0x57,
        ExVectorToString = 0x58,
        ExRotatorToString = 0x59,
        ExExtendedNative = 0x60,
        ExFirstNative = 0x70,
    };

    constexpr uint32_t ExMax = 0x1000;

    constexpr uint32_t CPF_ConstRef = 0x00000040;
    constexpr uint32_t CPF_NeedCtorLink = 0x00400000;
    constexpr uint32_t CPF_ParmFlags = CPF_OptionalParm | CPF_Parm | CPF_OutParm | CPF_SkipParm | CPF_ReturnParm | CPF_CoerceParm;
    constexpr uint32_t CPF_PropagateFromStruct = CPF_Const | CPF_Native | CPF_Transient;
    constexpr uint32_t FUNC_FuncInherit = FUNC_Exec | FUNC_Event;
    constexpr uint32_t FUNC_FuncOverrideMatch = FUNC_Exec | FUNC_Final | FUNC_Latent | FUNC_PreOperator | FUNC_Iterator | FUNC_Static;
    constexpr uint32_t FUNC_NetFuncFlags = FUNC_Net | FUNC_NetReliable;
    constexpr uint32_t CLASS_Inherit = CLASS_Transient | CLASS_Config | CLASS_Localized | CLASS_SafeReplace | CLASS_RuntimeStatic | CLASS_PerObjectConfig;
    constexpr uint32_t CLASS_RecompilerClear = CLASS_Inherit | CLASS_Abstract | CLASS_NoExport | CLASS_NativeReplication;
    constexpr uint32_t RF_NotForClientServer = RF_NotForClient | RF_NotForServer;
    constexpr uint16_t MaxWord = 0xffff;
    constexpr int32_t IndexNone = -1;

    struct PropertyBase {
        int type = CptNone;
        int32_t arrayDim = 1;
        uint32_t flags = 0;
        UField* ref = nullptr;
        UClass* metaClass = nullptr;

        PropertyBase() = default;
        explicit PropertyBase(int propertyType);
        explicit PropertyBase(UEnum* enumType);
        explicit PropertyBase(UClass* cls, UClass* meta = nullptr);
        explicit PropertyBase(UStruct* structType);
        explicit PropertyBase(UProperty* property);

        UEnum* enumType() const;
        UClass* propertyClass() const;
        UStruct* structType() const;
        int32_t size() const;
        bool isVector() const;
        bool isRotator() const;
        bool matchesType(const PropertyBase& other, bool identity) const;
    };

    class Lookup {
    public:
        void refresh();
        UClass* findClass(std::string_view name);
        UStruct* findStruct(std::string_view name);
        UObject* findObject(UClass* cls, std::string_view path);
        UObject* findPackage(std::string_view name);
        void add(UObject* object);

    private:
        void index(UObject* object);
        static bool earlier(const UObject* a, const UObject* b);

        size_t indexed = 0;
        std::unordered_map<Name, UClass*> classes;
        std::unordered_map<Name, UStruct*> structs;
    };

    UClass* ownerClass(UObject* object);
    bool isPlainStruct(const UObject* object);
    int32_t ue1PropertiesSize(UStruct* structType);
    int32_t numParms(UFunction* function);
    UProperty* returnProperty(UFunction* function);
    std::vector<UField*> fieldIterator(UStruct* structType);
    UStruct* inheritanceSuper(UStruct* structType);
    uint32_t classCrc(UClass* cls);
    void addDependency(UClass* cls, UClass* dependency, bool deep);
}
