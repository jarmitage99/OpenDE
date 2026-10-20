#pragma once

#include "compiler/script_compiler.h"
#include "compiler/script_model.h"
#include "core/objects.h"
#include "engine/package.h"

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace dx::compiler {
    struct PackageCompiler::Impl {
        UObject* package = nullptr;
        LogSink log;
        Lookup lookup;
        std::vector<UClass*> imported;
        std::unordered_map<UClass*, std::string> defaultProperties;
        std::unordered_map<UClass*, std::filesystem::path> files;
        std::vector<Name> nativeClasses;
        int errorCount = 0;
    };

    namespace detail {
        enum TokenKind {
            TokenNone = 0,
            TokenIdentifier = 1,
            TokenSymbol = 2,
            TokenConst = 3,
        };

        enum NestType {
            NestNone = 0,
            NestClass = 1,
            NestState = 2,
            NestFunction = 3,
            NestIf = 5,
            NestLoop = 6,
            NestSwitch = 7,
            NestFor = 8,
            NestForEach = 10,
        };

        enum AllowFlags : uint32_t {
            AllowStateCmd = 0x00001,
            AllowCmd = 0x00002,
            AllowFunction = 0x00004,
            AllowState = 0x00008,
            AllowElseIf = 0x00010,
            AllowVarDecl = 0x00040,
            AllowClass = 0x00080,
            AllowCase = 0x00100,
            AllowDefault = 0x00200,
            AllowReturn = 0x00400,
            AllowBreak = 0x00800,
            AllowLabel = 0x02000,
            AllowIgnores = 0x04000,
            AllowInstance = 0x08000,
            AllowInWhile = 0x10000,
            AllowContinue = 0x20000,
            AllowIterator = 0x40000,
        };

        enum FixupType {
            FixupSwitchEnd = 0,
            FixupIfEnd = 1,
            FixupLoopStart = 2,
            FixupLoopEnd = 3,
            FixupLoopPostCond = 4,
            FixupForStart = 5,
            FixupForEnd = 6,
            FixupLabel = 7,
            FixupIteratorEnd = 8,
            FixupForInc = 9,
            FixupMax = 10,
        };

        constexpr int MaxNestLevels = 16;

        constexpr int MaxFuncParms = 16;

        constexpr int MaxArraySize = 255;

        constexpr int NameSize = 64;

        constexpr int MaxStringConstSize = 255;

        constexpr uint32_t AutoConvert = 0x100;

        constexpr uint32_t Truncate = 0x200;

        constexpr uint32_t ConvertMask = ~(AutoConvert | Truncate);

        struct CompileError : std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        struct Names {
            Name vect { "Vect" };
            Name rot { "Rot" };
            Name trueName { "True" };
            Name falseName { "False" };
            Name arrayCount { "ArrayCount" };
            Name enumCount { "EnumCount" };
            Name defaultName { "Default" };
            Name staticName { "Static" };
            Name global { "Global" };
            Name super { "Super" };
            Name spawn { "Spawn" };
            Name classOf { "Class" };
            Name outer { "Outer" };
            Name byteName { "Byte" };
            Name intName { "Int" };
            Name boolName { "Bool" };
            Name floatName { "Float" };
            Name nameName { "Name" };
            Name stringName { "String" };
            Name structName { "Struct" };
            Name vector { "Vector" };
            Name rotator { "Rotator" };
            Name self { "Self" };
            Name newName { "New" };
            Name expands { "Expands" };
            Name extends { "Extends" };
            Name constName { "Const" };
            Name var { "Var" };
            Name local { "Local" };
            Name config { "Config" };
            Name globalConfig { "GlobalConfig" };
            Name localized { "Localized" };
            Name travel { "Travel" };
            Name privateName { "Private" };
            Name editConst { "EditConst" };
            Name input { "Input" };
            Name transient { "Transient" };
            Name intrinsic { "Intrinsic" };
            Name native { "Native" };
            Name out { "Out" };
            Name exportName { "Export" };
            Name skip { "Skip" };
            Name coerce { "Coerce" };
            Name optional { "Optional" };
            Name enumName { "Enum" };
            Name array { "Array" };
            Name map { "Map" };
            Name within { "Within" };
            Name noExport { "NoExport" };
            Name nativeReplication { "NativeReplication" };
            Name perObjectConfig { "PerObjectConfig" };
            Name noUserCreate { "NoUserCreate" };
            Name abstract { "Abstract" };
            Name guid { "Guid" };
            Name safeReplace { "SafeReplace" };
            Name import { "Import" };
            Name package { "Package" };
            Name from { "From" };
            Name function { "Function" };
            Name operatorName { "Operator" };
            Name preOperator { "PreOperator" };
            Name postOperator { "PostOperator" };
            Name final { "Final" };
            Name latent { "Latent" };
            Name iterator { "Iterator" };
            Name singular { "Singular" };
            Name exec { "Exec" };
            Name event { "Event" };
            Name simulated { "Simulated" };
            Name state { "State" };
            Name autoName { "Auto" };
            Name ignores { "Ignores" };
            Name replication { "Replication" };
            Name reliable { "Reliable" };
            Name unreliable { "Unreliable" };
            Name ifName { "If" };
            Name elseName { "Else" };
            Name until { "Until" };
            Name whileName { "While" };
            Name doName { "Do" };
            Name switchName { "Switch" };
            Name caseName { "Case" };
            Name returnName { "Return" };
            Name breakName { "Break" };
            Name continueName { "Continue" };
            Name forName { "For" };
            Name forEach { "ForEach" };
            Name assert { "Assert" };
            Name gotoName { "Goto" };
            Name stop { "Stop" };
            Name none { "None" };
        };

        struct Token : PropertyBase {
            int kind = TokenNone;
            Name tokenName;
            int32_t startPos = 0;
            int32_t startLine = 0;
            std::string identifier;
            uint8_t byteValue = 0;
            int32_t intValue = 0;
            bool boolValue = false;
            float floatValue = 0.0f;
            UObject* object = nullptr;
            Name nameValue;
            std::string stringValue;
            float vectorValue[3] = {};
            int32_t rotatorValue[3] = {};

            Token() = default;

            explicit Token(const PropertyBase& base)
                : PropertyBase(base)
            {
            }

            bool matches(const char* text) const
            {
                return (kind == TokenIdentifier || kind == TokenSymbol) && iequals(identifier, text);
            }

            bool matches(Name name) const
            {
                return kind == TokenIdentifier && tokenName == name;
            }

            void setBase(const PropertyBase& base)
            {
                static_cast<PropertyBase&>(*this) = base;
            }

            void setConstByte(UEnum* enumType, uint8_t value)
            {
                setBase(PropertyBase(CptByte));
                ref = enumType;
                byteValue = value;
                kind = TokenConst;
            }

            void setConstInt(int32_t value)
            {
                setBase(PropertyBase(CptInt));
                intValue = value;
                kind = TokenConst;
            }

            void setConstBool(bool value)
            {
                setBase(PropertyBase(CptBool));
                boolValue = value;
                kind = TokenConst;
            }

            void setConstFloat(float value)
            {
                setBase(PropertyBase(CptFloat));
                floatValue = value;
                kind = TokenConst;
            }

            void setConstObject(UObject* value)
            {
                setBase(PropertyBase(CptObject));
                ref = value ? value->cls : nullptr;
                object = value;
                kind = TokenConst;
                if (ref && ref == objects::classClass()) {
                    metaClass = cast<UClass>(value);
                }
            }

            void setConstName(Name value)
            {
                setBase(PropertyBase(CptName));
                nameValue = value;
                kind = TokenConst;
            }

            void setConstString(const std::string& value)
            {
                setBase(PropertyBase(CptString));
                stringValue = value.substr(0, MaxStringConstSize);
                kind = TokenConst;
            }

            void setConstVector(Lookup& lookup, const float value[3])
            {
                setBase(PropertyBase(CptStruct));
                ref = lookup.findStruct("Vector");
                std::copy(value, value + 3, vectorValue);
                kind = TokenConst;
            }

            void setConstRotation(Lookup& lookup, const int32_t value[3])
            {
                setBase(PropertyBase(CptStruct));
                ref = lookup.findStruct("Rotator");
                std::copy(value, value + 3, rotatorValue);
                kind = TokenConst;
            }

            bool getConstByte(uint8_t& value) const
            {
                if (kind == TokenConst && type == CptByte) {
                    value = byteValue;
                    return true;
                }
                if (kind == TokenConst && type == CptInt && intValue >= 0 && intValue < 255) {
                    value = static_cast<uint8_t>(intValue);
                    return true;
                }
                if (kind == TokenConst && type == CptFloat && floatValue >= 0 && floatValue < 255 && floatValue == static_cast<float>(static_cast<int32_t>(floatValue))) {
                    value = static_cast<uint8_t>(floatValue);
                    return true;
                }
                return false;
            }

            bool getConstInt(int32_t& value) const
            {
                if (kind == TokenConst && type == CptInt) {
                    value = intValue;
                    return true;
                }
                if (kind == TokenConst && type == CptByte) {
                    value = byteValue;
                    return true;
                }
                if (kind == TokenConst && type == CptFloat && floatValue == static_cast<float>(static_cast<int32_t>(floatValue))) {
                    value = static_cast<int32_t>(floatValue);
                    return true;
                }
                return false;
            }

            bool getConstBool(bool& value) const
            {
                if (kind == TokenConst && type == CptBool) {
                    value = boolValue;
                    return true;
                }
                return false;
            }

            bool getConstFloat(float& value) const
            {
                if (kind == TokenConst && type == CptFloat) {
                    value = floatValue;
                    return true;
                }
                if (kind == TokenConst && type == CptInt) {
                    value = static_cast<float>(intValue);
                    return true;
                }
                if (kind == TokenConst && type == CptByte) {
                    value = byteValue;
                    return true;
                }
                return false;
            }

            bool getConstObject(UClass* desired, UObject*& value) const
            {
                if (kind == TokenConst && type == CptObject && (desired == nullptr || (propertyClass() && propertyClass()->isChildOf(desired)))) {
                    value = object;
                    return true;
                }
                return false;
            }

            bool getConstName(Name& value) const
            {
                if (kind == TokenConst && type == CptName) {
                    value = nameValue;
                    return true;
                }
                return false;
            }

            bool getConstVector(float value[3]) const
            {
                if (kind == TokenConst && isVector()) {
                    std::copy(vectorValue, vectorValue + 3, value);
                    return true;
                }
                return false;
            }

            bool getConstRotation(int32_t value[3]) const
            {
                if (kind == TokenConst && isRotator()) {
                    std::copy(rotatorValue, rotatorValue + 3, value);
                    return true;
                }
                return false;
            }

            void attemptToConvertConstant(Lookup& lookup, const PropertyBase& newType)
            {
                switch (newType.type) {
                case CptInt: {
                    int32_t value = 0;
                    if (getConstInt(value)) {
                        setConstInt(value);
                    }
                    break;
                }
                case CptBool: {
                    bool value = false;
                    if (getConstBool(value)) {
                        setConstBool(value);
                    }
                    break;
                }
                case CptFloat: {
                    float value = 0.0f;
                    if (getConstFloat(value)) {
                        setConstFloat(value);
                    }
                    break;
                }
                case CptName: {
                    Name value;
                    if (getConstName(value)) {
                        setConstName(value);
                    }
                    break;
                }
                case CptStruct:
                    if (newType.isVector()) {
                        float value[3] = {};
                        if (getConstVector(value)) {
                            setConstVector(lookup, value);
                        }
                    } else if (newType.isRotator()) {
                        int32_t value[3] = {};
                        if (getConstRotation(value)) {
                            setConstRotation(lookup, value);
                        }
                    }
                    break;
                case CptByte: {
                    uint8_t value = 0;
                    if (newType.enumType() == nullptr && getConstByte(value)) {
                        setConstByte(nullptr, value);
                    }
                    break;
                }
                case CptObject: {
                    UObject* value = nullptr;
                    if (getConstObject(newType.propertyClass(), value)) {
                        setConstObject(value);
                    }
                    break;
                }
                default:
                    break;
                }
            }
        };

        struct RetryPoint {
            const char* input = nullptr;
            int32_t inputLength = 0;
            int32_t inputPos = 0;
            int32_t inputLine = 0;
            int32_t codeTop = 0;
        };

        struct FixupRequest {
            FixupType type;
            int32_t code;
            Name name;
        };

        struct LabelRecord {
            Name name;
            int32_t code;
        };

        struct NestInfo {
            UStruct* node = nullptr;
            NestType nestType = NestNone;
            uint32_t allow = 0;
            int32_t fixups[FixupMax] = {};
            std::vector<FixupRequest> fixupList;
            std::vector<LabelRecord> labelList;
            int32_t codeChain = IndexNone;
            Token switchType;
            RetryPoint forRetry;

            void setFixup(FixupType fixupType, int32_t code)
            {
                fixups[fixupType] = code;
            }
        };

        [[noreturn]] void fail(const std::string& message);
        const Names& names();
        uint32_t getConversion(const PropertyBase& dest, const PropertyBase& source);
        extern const char* const cppTags[96];
        const char* cptTypeName(int type);
        bool isAnsi(const std::string& text);
        UClass* intrinsic(const char* name);
        UProperty* newProperty(PropType type, UObject* outer, Name name, uint32_t flags);
        UProperty* cloneParameter(UProperty* source, UObject* outer);
        bool isA(UField* field, const char* kind);
        void importDefaults(PackageCompiler::Impl& context, UClass* cls, const std::string& text);

        template <typename T>
        T* createField(T* field, UObject* outer, Name name, const char* className, uint32_t flags)
        {
            field->name = name;
            field->outer = outer;
            field->flags = flags;
            field->cls = className ? intrinsic(className) : objects::classClass();
            field->loaded = true;
            objects::registerObject(field);
            return field;
        }

        bool isAlnum(char c);

        class ScriptCompiler {
        public:
            ScriptCompiler(PackageCompiler::Impl& owner)
                : context(owner)
                , lookup(owner.lookup)
            {
            }

            bool compileScript(UClass* cls, int pass);

        private:
            PackageCompiler::Impl& context;
            Lookup& lookup;
            UClass* compiling = nullptr;
            std::string sourceText;
            const char* input = nullptr;
            int32_t inputLength = 0;
            int32_t inputPos = 0;
            int32_t inputLine = 0;
            int32_t prevPos = 0;
            int32_t prevLine = 0;
            bool gotAffector = false;
            int pass = 0;
            int nestLevel = 0;
            NestInfo* topNest = nullptr;
            UStruct* topNode = nullptr;
            NestInfo nest[MaxNestLevels];

            std::vector<uint8_t>& code();
            int32_t codeTop();
            void emit(uint8_t value);
            void emitWord(uint16_t value);
            void emitDword(uint32_t value);
            void emitFloat(float value);
            void emitObject(UObject* object);
            void emitName(Name name);
            void patchWord(int32_t at, uint16_t value);
            char getChar(bool literal = false);
            char peekAt(int32_t position) const;
            void ungetChar();
            char peekChar() const;
            char getLeadingChar();
            static bool isEol(char c);
            void emitConstant(Token& token);
            void emitStackNodeLinkFunction(UFunction* node, bool forceFinal, bool global);
            void emitAddressToFixupLater(NestInfo* target, FixupType type, Name name);
            void emitAddressToChainLater(NestInfo* target);
            void emitChainUpdate(NestInfo* target);
            void emitSize(int32_t size, const char* tag);
            void emitLet(const PropertyBase& type);
            bool getToken(Token& token, const PropertyBase* hint = nullptr, bool noConsts = false);
            bool getRawToken(Token& token);
            bool getIdentifier(Token& token, bool noConsts = false);
            bool getSymbol(Token& token);
            bool getConstInt(int32_t& result, const char* tag = nullptr);
            bool getConstFloat(float& result, const char* tag = nullptr);
            bool matchIdentifier(Name match);
            bool matchSymbol(const char* match);
            bool peekSymbol(const char* match);
            bool peekIdentifier(Name match);
            void ungetToken(const Token& token);
            void requireSymbol(const char* match, const std::string& tag);
            void requireIdentifier(Name match, const std::string& tag);
            void requireSizeOfParm(Token& typeToken, const char* tag);
            UClass* getQualifiedClass(const char* thing);
            UField* findField(UStruct* scope, const std::string& identifier, const char* fieldClass = "Field", const char* thing = nullptr);
            UEnum* compileEnum(UStruct* scope);
            UStruct* compileStruct(UStruct* scope);
            void compileConst(UStruct* scope);
            RetryPoint initRetry();
            void performRetry(const RetryPoint& retry, bool binary = true, bool text = true);
            void codeSwitcheroo(const RetryPoint& low, const RetryPoint& high);
            bool compileFieldExpr(UStruct* scope, PropertyBase& requiredType, Token token, Token& resultType, bool isSelf, bool isConcrete);
            int conversionCost(const PropertyBase& dest, const PropertyBase& source);
            bool compileDynamicCast(Token token, Token& resultType);
            int compileExpr(PropertyBase requiredType, const char* errorTag = nullptr, Token* resultToken = nullptr, int maxPrecedence = INT_MAX, PropertyBase* hintType = nullptr);
            static const char* nestTypeName(NestType type);
            void checkAllow(const std::string& thing, uint32_t allowFlags);
            void pushNest(NestType nestType, Name thisName, UStruct* inNode);
            void popNest(NestType nestType, const std::string& descr);
            int findNest(NestType nestType);
            void compileDirective();
            void execDirective(const std::string& command);
            bool getVarType(UStruct* scope, PropertyBase& varProperty, uint32_t& objectFlags, uint32_t disallow, const char* thing);
            UProperty* getVarNameAndDim(UStruct* scope, PropertyBase& varProperty, uint32_t objectFlags, bool noArrays, const char* hardcodedName, const char* thing, Name category, bool skip);
            static void linkFlags(UProperty* property);
            static bool needsConstructor(UStruct* structType);
            void compileAffector();
            bool compileDeclaration(Token& token, bool& needSemicolon);
            void compileFunctionDeclaration(Token& token, bool& needSemicolon);
            void compileIgnores();
            void compileCommand(Token& token, bool& needSemicolon);
            bool compileStatement();
            void compileStatements();
            void postParse(UStruct* node);
            void skipStatements(int subCount, const char* errorTag);
            void compileSecondPass(UStruct* node);
            void initReplication(UStruct* node);
            void compileReplication();
        };
    }
}
