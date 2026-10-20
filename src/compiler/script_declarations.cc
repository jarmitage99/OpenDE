#include "compiler/script_parser.h"

#include "core/objects.h"
#include "core/vm.h"
#include "engine/package.h"

#include <algorithm>
#include <cctype>

namespace dx::compiler::detail {
    UEnum* ScriptCompiler::compileEnum(UStruct* scope)
    {
        if (topNest->nestType != NestClass) {
            fail("Enums can only be declared in class or struct scope");
        }
        Token enumToken;
        if (!getIdentifier(enumToken)) {
            fail("Missing enumeration name");
        }
        UField* existing = findField(scope, enumToken.identifier);
        if (existing && existing->outer == scope) {
            fail("enum: '" + enumToken.identifier + "' already defined here");
        }
        requireSymbol("{", "'Enum'");
        UEnum* enumType = createField(new UEnum(), scope, Name(enumToken.identifier), "Enum", RF_Public);
        enumType->next = scope->children;
        scope->children = enumType;
        Token tagToken;
        while (getIdentifier(tagToken)) {
            Name tag(tagToken.identifier);
            if (std::find(enumType->names.begin(), enumType->names.end(), tag) != enumType->names.end()) {
                fail("Duplicate enumeration tag " + tagToken.identifier);
            }
            enumType->names.push_back(tag);
            if (enumType->names.size() > 255) {
                fail("Exceeded maximum of 255 enumerators");
            }
            if (!matchSymbol(",")) {
                break;
            }
        }
        if (enumType->names.empty()) {
            fail("Enumeration must contain at least one enumerator");
        }
        requireSymbol("}", "'Enum'");
        return enumType;
    }

    UStruct* ScriptCompiler::compileStruct(UStruct* scope)
    {
        if (topNest->nestType != NestClass) {
            fail("Enums can only be declared in class or struct scope");
        }
        Token structToken;
        if (!getIdentifier(structToken)) {
            fail("Missing struct name");
        }
        UField* existing = findField(scope, structToken.identifier);
        if (existing && existing->outer == scope) {
            fail("struct: '" + structToken.identifier + "' already defined here");
        }
        UStruct* baseStruct = nullptr;
        if (matchIdentifier(names().expands) || matchIdentifier(names().extends)) {
            Token parentName;
            if (!getIdentifier(parentName)) {
                fail("'struct': Missing parent struct after 'Expands'");
            }
            baseStruct = cast<UStruct>(findField(scope, parentName.identifier, "Struct", "'expands'"));
            if (!baseStruct) {
                fail("'expands': Can't find base struct '" + parentName.identifier + "'");
            }
        }
        UStruct* structType = createField(new UStruct(), scope, Name(structToken.identifier), "Struct", RF_Public);
        structType->superField = baseStruct;
        structType->friendlyName = structType->name;
        structType->next = scope->children;
        scope->children = structType;
        requireSymbol("{", "'struct'");
        Token token;
        do {
            getToken(token);
            if (token.matches(names().structName)) {
                compileStruct(structType);
                requireSymbol(";", "'struct'");
            } else if (token.matches(names().constName)) {
                compileConst(structType);
                requireSymbol(";", "'const'");
            } else if (token.matches(names().var)) {
                uint32_t editFlags = 0;
                if (matchSymbol("(")) {
                    editFlags |= CPF_Edit;
                    requireSymbol(")", "Editable 'struct' member variable");
                }
                uint32_t objectFlags = 0;
                PropertyBase originalProperty(CptNone);
                getVarType(structType, originalProperty, objectFlags, CPF_ParmFlags, "'struct' member variable");
                originalProperty.flags |= editFlags;
                Name category = (editFlags & CPF_Edit) ? Name::find(structToken.identifier) : Name();
                if (originalProperty.flags & CPF_ParmFlags) {
                    fail("Illegal type modifiers in variable");
                }
                do {
                    PropertyBase property = originalProperty;
                    getVarNameAndDim(structType, property, objectFlags, false, nullptr, "Variable declaration", category, false);
                } while (matchSymbol(","));
                requireSymbol(";", "'struct'");
            } else if (!token.matches("}")) {
                fail("'struct': Expecting 'Var', got '" + token.identifier + "'");
            }
        } while (!token.matches("}"));
        lookup.add(structType);
        return structType;
    }

    void ScriptCompiler::compileConst(UStruct* scope)
    {
        Token constToken;
        if (!getIdentifier(constToken)) {
            fail("Missing constant name");
        }
        Name constName(constToken.identifier);
        UField* existing = findField(scope, constToken.identifier);
        if (existing && existing->outer == scope) {
            fail("const: '" + constToken.identifier + "' already defined");
        }
        requireSymbol("=", "'const'");
        int32_t start = inputPos;
        Token valueToken;
        if (!getToken(valueToken)) {
            fail("const " + constName.str() + ": Missing value");
        }
        if (valueToken.kind != TokenConst) {
            fail("const " + constName.str() + ": Value is not constant");
        }
        std::string value(input + start, static_cast<size_t>(std::min(1024, inputPos - start + 1) - 1));
        UConst* constant = createField(new UConst(), scope, constName, "Const", 0);
        constant->value = value;
        constant->next = scope->children;
        scope->children = constant;
    }

    void ScriptCompiler::compileDirective()
    {
        Token directive;
        if (!getIdentifier(directive)) {
            fail("Missing compiler directive after '#'");
        } else if (directive.matches("Error")) {
            fail("#Error directive encountered");
        } else if (directive.matches("Call") || directive.matches("AlwaysCall")) {
            Token identifier;
            if (!getRawToken(identifier)) {
                fail("'Call': missing command line");
            }
        } else if (directive.matches("Exec") || directive.matches("AlwaysExec") || directive.matches("ForceExec")) {
            Token identifier;
            if (!getRawToken(identifier)) {
                fail("'#Exec': missing command line");
            }
            if (pass == 0 || directive.matches("ForceExec")) {
                execDirective(identifier.stringValue);
            }
        } else {
            fail("Unrecognized compiler directive " + directive.identifier);
        }
        char c;
        while (!isEol(c = getChar())) {
        }
        if (c == 0) {
            ungetChar();
        }
    }

    void ScriptCompiler::execDirective(const std::string& command)
    {
        std::string text = command;
        auto next = [&]() {
            size_t start = text.find_first_not_of(" \t");
            if (start == std::string::npos) {
                text.clear();
                return std::string();
            }
            size_t end = text.find_first_of(" \t", start);
            std::string word = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
            text = end == std::string::npos ? std::string() : text.substr(end);
            return word;
        };
        std::string first = next();
        std::string second = next();
        if (iequals(first, "OBJ") && iequals(second, "LOAD")) {
            size_t at = 0;
            std::string upper = text;
            for (char& c : upper) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            at = upper.find("FILE=");
            if (at == std::string::npos) {
                fail("OBJ LOAD: missing FILE=");
            }
            std::string file = text.substr(at + 5);
            size_t end = file.find_first_of(" \t");
            if (end != std::string::npos) {
                file = file.substr(0, end);
            }
            std::replace(file.begin(), file.end(), '\\', '/');
            std::string packageName = std::filesystem::path(file).stem().string();
            Linker* linker = objects::loadPackage(Name(packageName));
            if (!linker) {
                context.log("Warning: OBJ LOAD could not load " + file);
                return;
            }
            for (size_t i = 0; i < linker->file().exports().size(); i++) {
                objects::ensureLoaded(linker->object(static_cast<int32_t>(i + 1)));
            }
            objects::flushLoads();
            return;
        }
        context.log("Warning: unsupported #exec " + command);
    }

    bool ScriptCompiler::getVarType(UStruct* scope, PropertyBase& varProperty, uint32_t& objectFlags, uint32_t disallow, const char* thing)
    {
        const Names& n = names();
        objectFlags = RF_Public;
        uint32_t flags = 0;
        while (true) {
            Token specifier;
            getToken(specifier);
            if (specifier.matches(n.constName)) {
                flags |= CPF_Const;
            } else if (specifier.matches(n.config)) {
                flags |= CPF_Config;
            } else if (specifier.matches(n.globalConfig)) {
                flags |= CPF_GlobalConfig | CPF_Config;
            } else if (specifier.matches(n.localized)) {
                flags |= CPF_Localized;
            } else if (specifier.matches(n.travel)) {
                flags |= CPF_Travel;
            } else if (specifier.matches(n.privateName)) {
                objectFlags &= ~RF_Public;
            } else if (specifier.matches(n.editConst)) {
                flags |= CPF_EditConst;
            } else if (specifier.matches(n.input)) {
                flags |= CPF_Input;
            } else if (specifier.matches(n.transient)) {
                flags |= CPF_Transient;
            } else if (specifier.matches(n.intrinsic) || specifier.matches(n.native)) {
                flags |= CPF_Native;
            } else if (specifier.matches(n.out)) {
                flags |= CPF_OutParm;
            } else if (specifier.matches(n.exportName)) {
                flags |= CPF_ExportObject;
            } else if (specifier.matches(n.skip)) {
                flags |= CPF_SkipParm;
            } else if (specifier.matches(n.coerce)) {
                flags |= CPF_CoerceParm;
            } else if (specifier.matches(n.optional)) {
                flags |= CPF_OptionalParm;
            } else {
                ungetToken(specifier);
                break;
            }
        }
        Token varType;
        if (!getIdentifier(varType, true)) {
            if (!thing) {
                return false;
            }
            fail(std::string(thing) + ": Missing variable type");
        } else if (varType.matches(n.enumName)) {
            varProperty = PropertyBase(compileEnum(scope));
        } else if (varType.matches(n.structName)) {
            varProperty = PropertyBase(compileStruct(scope));
        } else if (varType.matches(n.byteName)) {
            varProperty = PropertyBase(CptByte);
        } else if (varType.matches(n.intName)) {
            varProperty = PropertyBase(CptInt);
        } else if (varType.matches(n.boolName)) {
            varProperty = PropertyBase(CptBool);
        } else if (varType.matches(n.floatName)) {
            varProperty = PropertyBase(CptFloat);
        } else if (varType.matches(n.nameName)) {
            varProperty = PropertyBase(CptName);
        } else if (varType.matches(n.array)) {
            requireSymbol("<", "'array'");
            getVarType(scope, varProperty, objectFlags, disallow, "'array'");
            if (varProperty.arrayDim == 0) {
                fail("Arrays within arrays not supported");
            }
            varProperty.arrayDim = 0;
            requireSymbol(">", "'array'");
        } else if (varType.matches(n.map)) {
            fail("Map are not supported in UnrealScript yet");
        } else if (varType.matches(n.stringName)) {
            if (matchSymbol("[")) {
                int32_t stringSize = 0;
                if (!getConstInt(stringSize)) {
                    fail(std::string(thing ? thing : "Declaration") + ": Missing string size");
                }
                if (!matchSymbol("]")) {
                    fail(std::string(thing ? thing : "Declaration") + ": Missing ']'");
                }
            }
            varProperty = PropertyBase(CptString);
        } else if (UClass* tempClass = lookup.findClass(varType.identifier)) {
            varProperty = PropertyBase(tempClass);
            if (tempClass == objects::classClass()) {
                if (matchSymbol("<")) {
                    Token limitor;
                    if (!getIdentifier(limitor)) {
                        fail("'class': Missing class limitor");
                    }
                    varProperty.metaClass = lookup.findClass(limitor.identifier);
                    if (!varProperty.metaClass) {
                        fail("'class': Limitor '" + limitor.identifier + "' is not a class name");
                    }
                    requireSymbol(">", "'class limitor'");
                } else {
                    varProperty.metaClass = objects::objectClass();
                }
            }
        } else if (UField* field = findField(scope, varType.identifier)) {
            if (cast<UEnum>(field)) {
                varProperty = PropertyBase(cast<UEnum>(field));
            } else if (isPlainStruct(field)) {
                varProperty = PropertyBase(cast<UStruct>(field));
            } else {
                if (!thing) {
                    ungetToken(varType);
                    return false;
                }
                fail("Unrecognized type '" + varType.identifier + "'");
            }
        } else if (!thing) {
            ungetToken(varType);
            return false;
        } else {
            fail("Unrecognized type '" + varType.identifier + "'");
        }
        varProperty.flags |= flags;
        if (varProperty.flags & disallow) {
            fail("Specified type modifiers not allowed here");
        }
        return true;
    }

    UProperty* ScriptCompiler::getVarNameAndDim(UStruct* scope, PropertyBase& varProperty, uint32_t objectFlags, bool noArrays, const char* hardcodedName, const char* thing, Name category, bool skip)
    {
        Token varToken;
        if (hardcodedName) {
            varToken.kind = TokenIdentifier;
            varToken.identifier = hardcodedName;
        } else if (!getIdentifier(varToken)) {
            fail("Missing variable name");
        }
        if (!skip) {
            UField* existing = findField(scope, varToken.identifier);
            if (existing && existing->outer == scope) {
                fail(std::string(thing) + ": '" + varToken.identifier + "' already defined");
            }
        }
        Name varName(varToken.identifier);
        if (matchSymbol("[")) {
            if (noArrays) {
                fail("Arrays aren't allowed in this context");
            }
            if (varProperty.type == CptBool) {
                fail("Bool arrays are not allowed");
            }
            if (!peekSymbol("]")) {
                if (!getConstInt(varProperty.arrayDim)) {
                    fail(std::string(thing) + " " + varToken.identifier + ": Bad or missing array size");
                }
                if (varProperty.arrayDim <= 1 && varProperty.arrayDim > MaxArraySize) {
                    fail(std::string(thing) + " " + varToken.identifier + ": Illegal array size");
                }
            }
            if (!matchSymbol("]")) {
                fail(std::string(thing) + " " + varToken.identifier + ": Missing ']'");
            }
        } else if (matchSymbol("(")) {
            fail("Use [] for arrays, not ()");
        }
        if (skip) {
            return nullptr;
        }
        UProperty* prev = nullptr;
        for (UField* child = scope->children; child; child = child->next) {
            if (child->outer != scope) {
                break;
            }
            if (UProperty* property = cast<UProperty>(child)) {
                prev = property;
            }
        }
        UObject* newScope = scope;
        UProperty* array = nullptr;
        if (varProperty.arrayDim == 0) {
            array = newProperty(PropType::Array, scope, varName, objectFlags);
            newScope = array;
            varProperty.arrayDim = 1;
            objectFlags = RF_Public;
        }
        UProperty* property = nullptr;
        switch (varProperty.type) {
        case CptByte:
            property = newProperty(PropType::Byte, newScope, varName, objectFlags);
            property->enumType = varProperty.enumType();
            break;
        case CptInt:
            property = newProperty(PropType::Int, newScope, varName, objectFlags);
            break;
        case CptBool:
            property = newProperty(PropType::Bool, newScope, varName, objectFlags);
            break;
        case CptFloat:
            property = newProperty(PropType::Float, newScope, varName, objectFlags);
            break;
        case CptObject:
            if (varProperty.ref == objects::classClass()) {
                property = newProperty(PropType::Class, newScope, varName, objectFlags);
                property->metaClass = varProperty.metaClass;
            } else {
                property = newProperty(PropType::Object, newScope, varName, objectFlags);
            }
            property->objectClass = varProperty.propertyClass();
            break;
        case CptName:
            property = newProperty(PropType::Name, newScope, varName, objectFlags);
            break;
        case CptString:
            property = newProperty(PropType::Str, newScope, varName, objectFlags);
            break;
        case CptStruct:
            property = newProperty(PropType::Struct, newScope, varName, objectFlags);
            property->structType = varProperty.structType();
            break;
        default:
            fail("Unknown property type " + std::to_string(varProperty.type));
        }
        if (array) {
            array->inner = property;
            property = array;
        }
        property->arrayDim = varProperty.arrayDim;
        property->propertyFlags = varProperty.flags;
        property->category = category;
        linkFlags(property);
        if (array) {
            linkFlags(array->inner);
        }
        if (prev) {
            property->next = prev->next;
            prev->next = property;
        } else {
            property->next = scope->children;
            scope->children = property;
        }
        return property;
    }

    void ScriptCompiler::linkFlags(UProperty* property)
    {
        if (property->propertyFlags & CPF_Native) {
            return;
        }
        if (property->type == PropType::Str || property->type == PropType::String || property->type == PropType::Array) {
            property->propertyFlags |= CPF_NeedCtorLink;
        } else if (property->type == PropType::Struct && needsConstructor(property->structType)) {
            property->propertyFlags |= CPF_NeedCtorLink;
        }
    }

    bool ScriptCompiler::needsConstructor(UStruct* structType)
    {
        for (UField* field : fieldIterator(structType)) {
            UProperty* member = cast<UProperty>(field);
            if (member && (member->propertyFlags & CPF_NeedCtorLink)) {
                return true;
            }
        }
        return false;
    }

    bool ScriptCompiler::compileDeclaration(Token& token, bool& needSemicolon)
    {
        const Names& n = names();
        if (token.matches(n.classOf) && (topNest->allow & AllowClass)) {
            checkAllow("'class'", AllowClass);
            if (!getToken(token)) {
                fail("Missing class name");
            }
            if (!token.matches(compiling->name.c_str())) {
                fail("Class must be named " + compiling->name.str() + ", not " + token.identifier);
            }
            if (matchIdentifier(n.expands) || matchIdentifier(n.extends)) {
                UClass* tempClass = getQualifiedClass("'expands'");
                if (compiling->superClass() == nullptr) {
                    compiling->superField = tempClass;
                } else if (compiling->superClass() != tempClass) {
                    fail(compiling->pathName() + "'s superclass must be " + compiling->superClass()->pathName() + ", not " + tempClass->pathName());
                }
            } else if (compiling->superClass()) {
                fail("class: missing 'Expands " + compiling->superClass()->name.str() + "'");
            }
            if (matchIdentifier(n.within)) {
                UClass* tempClass = getQualifiedClass("'within'");
                if (compiling->within == nullptr) {
                    compiling->within = tempClass;
                } else if (compiling->within != tempClass) {
                    fail(compiling->pathName() + " must be within " + compiling->within->pathName() + ", not " + tempClass->pathName());
                }
            }
            UClass* expectedWithin = compiling->superClass() ? compiling->superClass()->within : objects::objectClass();
            if (!compiling->within->isChildOf(expectedWithin)) {
                fail("'within': within " + compiling->within->name.str() + " is not a generalization of superclass within '" + expectedWithin->name.str() + "'");
            }
            while (true) {
                Token attribute;
                getToken(attribute);
                if (attribute.matches(n.intrinsic) || attribute.matches(n.native)) {
                    if (compiling->superClass() && !(compiling->superClass()->flags & RF_Native)) {
                        fail("Native classes cannot expand non-native classes");
                    }
                    compiling->flags |= RF_Native;
                } else if (attribute.matches(n.noExport)) {
                    compiling->classFlags |= CLASS_NoExport;
                } else if (attribute.matches(n.nativeReplication)) {
                    compiling->classFlags |= CLASS_NativeReplication;
                } else if (attribute.matches(n.perObjectConfig)) {
                    compiling->classFlags |= CLASS_PerObjectConfig;
                } else if (attribute.matches(n.noUserCreate)) {
                    compiling->classFlags |= CLASS_NoUserCreate;
                } else if (attribute.matches(n.abstract)) {
                    compiling->classFlags |= CLASS_Abstract;
                } else if (attribute.matches(n.guid)) {
                    requireSymbol("(", "'Guid'");
                    for (int i = 0; i < 4; i++) {
                        int32_t part = 0;
                        getConstInt(part);
                        compiling->guid[i] = static_cast<uint32_t>(part);
                        requireSymbol(i < 3 ? "," : ")", "'Guid'");
                    }
                } else if (attribute.matches(n.transient)) {
                    compiling->classFlags |= CLASS_Transient;
                } else if (attribute.matches(n.localized)) {
                    fail("Class 'localized' keyword is no longer required");
                } else if (attribute.matches(n.config)) {
                    if (matchSymbol("(")) {
                        Token configToken;
                        if (!getIdentifier(configToken, false)) {
                            fail("config: Missing configuration name");
                        }
                        compiling->configName = Name(configToken.identifier);
                        requireSymbol(")", "config");
                    }
                } else if (attribute.matches(n.safeReplace)) {
                    compiling->classFlags |= CLASS_SafeReplace;
                } else {
                    ungetToken(attribute);
                    break;
                }
            }
            requireSymbol(";", "'Class'");
            needSemicolon = false;
            compiling->script.clear();
            compiling->children = nullptr;
            compiling->next = nullptr;
            compiling->probeMask = 0;
            compiling->ignoreMask = 0;
            compiling->stateFlags = 0;
            compiling->labelTableOffset = 0;
            compiling->flags &= ~RF_Transient;
            compiling->packageImports.clear();
            for (UClass* current = compiling; current; current = current->superClass()) {
                auto addUnique = [&](Name item) {
                    if (std::find(compiling->packageImports.begin(), compiling->packageImports.end(), item) == compiling->packageImports.end()) {
                        compiling->packageImports.push_back(item);
                    }
                };
                addUnique(current->outer->name);
                for (Name imported : current->packageImports) {
                    addUnique(imported);
                }
            }
            compiling->dependencies.clear();
            addDependency(compiling, compiling, true);
            if (compiling->superClass()) {
                addDependency(compiling, compiling->superClass(), true);
            }
            pushNest(NestClass, compiling->name, nullptr);
        } else if (token.matches(n.import)) {
            checkAllow("'Uses'", AllowVarDecl);
            if (topNest->nestType != NestClass) {
                fail("'Uses' is are only allowed at class scope");
            }
            Token importThing;
            getToken(importThing);
            if (!importThing.matches(n.enumName) && !importThing.matches(n.package)) {
                fail("'import': Missing 'enum', 'struct', or 'package'");
            }
            Token importName;
            if (!getIdentifier(importName)) {
                fail("'import': Missing package, enum, or struct name to import");
            }
            if (importThing.matches(n.package)) {
                Name imported(importName.identifier);
                if (std::find(compiling->packageImports.begin(), compiling->packageImports.end(), imported) == compiling->packageImports.end()) {
                    compiling->packageImports.push_back(imported);
                }
            } else {
                fail("'Uses': Unrecognized '" + importThing.identifier + "'");
            }
        } else if (token.matches(n.function) || token.matches(n.operatorName) || token.matches(n.preOperator) || token.matches(n.postOperator) || token.matches(n.intrinsic) || token.matches(n.native) || token.matches(n.final) || token.matches(n.privateName) || token.matches(n.latent) || token.matches(n.iterator) || token.matches(n.singular) || token.matches(n.staticName) || token.matches(n.exec) || (token.matches(n.event) && (topNest->allow & AllowFunction)) || (token.matches(n.simulated) && !peekIdentifier(n.state))) {
            compileFunctionDeclaration(token, needSemicolon);
        } else if (token.matches(n.constName)) {
            compileConst(compiling);
        } else if (token.matches(n.var) || token.matches(n.local)) {
            uint32_t disallow = 0;
            if (token.matches(n.var)) {
                checkAllow("'Var'", AllowVarDecl);
                if (topNest->nestType != NestClass) {
                    fail("Instance variables are only allowed at class scope (use 'local'?)");
                }
                disallow = CPF_ParmFlags;
            } else {
                checkAllow("'Local'", AllowVarDecl);
                if (topNest->nestType == NestClass) {
                    fail("Local variables are only allowed in functions");
                }
                disallow = ~0u;
            }
            Name editCategory;
            uint32_t editFlags = 0;
            if (matchSymbol("(")) {
                editFlags |= CPF_Edit;
                Token category;
                if (getIdentifier(category, true)) {
                    editCategory = Name(category.identifier);
                } else {
                    editCategory = compiling->name;
                }
                if (!matchSymbol(")")) {
                    fail("Missing ')' after editable category");
                }
            }
            PropertyBase originalProperty(CptNone);
            uint32_t objectFlags = 0;
            getVarType(topNode, originalProperty, objectFlags, disallow, "Variable declaration");
            originalProperty.flags |= editFlags;
            if ((originalProperty.flags & CPF_Edit) && editCategory.none()) {
                editCategory = compiling->name;
            }
            if ((originalProperty.flags & (CPF_Transient | CPF_Native)) && topNest->nestType != NestClass) {
                fail("Static and local variables may not be transient or native");
            }
            if (originalProperty.flags & CPF_ParmFlags) {
                fail("Illegal type modifiers in variable");
            }
            do {
                PropertyBase property = originalProperty;
                getVarNameAndDim(topNode, property, objectFlags, false, nullptr, "Variable declaration", editCategory, false);
            } while (matchSymbol(","));
        } else if (token.matches(n.enumName)) {
            checkAllow("'Enum'", AllowVarDecl);
            compileEnum(compiling);
        } else if (token.matches(n.structName)) {
            checkAllow("'struct'", AllowVarDecl);
            compileStruct(compiling);
        } else if (token.matches(n.state) || token.matches(n.autoName) || token.matches(n.simulated)) {
            checkAllow("'State'", AllowState);
            uint32_t stateFlags = 0;
            bool gotState = false;
            while (true) {
                if (token.matches(n.state)) {
                    gotState = true;
                    if (matchSymbol("(")) {
                        requireSymbol(")", "'State'");
                        stateFlags |= STATE_Editable;
                    }
                } else if (token.matches(n.simulated)) {
                    stateFlags |= STATE_Simulated;
                } else if (token.matches(n.autoName)) {
                    stateFlags |= STATE_Auto;
                } else {
                    ungetToken(token);
                    break;
                }
                getToken(token);
            }
            if (!gotState) {
                fail("Missing 'State'");
            }
            Token nameToken;
            if (!getIdentifier(nameToken)) {
                fail("Missing state name");
            }
            UState* parentState = cast<UState>(findField(topNode, nameToken.identifier, "State", "'state'"));
            if (parentState && ownerClass(parentState) == compiling) {
                fail("Duplicate state '" + nameToken.identifier + "'");
            }
            if (matchIdentifier(n.expands) || matchIdentifier(n.extends)) {
                Token parentToken;
                if (parentState) {
                    fail("'Expands' not allowed here: state '" + nameToken.identifier + "' overrides version in parent class");
                }
                if (!getIdentifier(parentToken)) {
                    fail("Missing parent state name");
                }
                parentState = cast<UState>(findField(topNode, parentToken.identifier, "State", "'state'"));
                if (!parentState) {
                    fail("'expands': Parent state '" + parentToken.identifier + "' not found");
                }
            }
            pushNest(NestState, Name(nameToken.identifier), nullptr);
            UState* state = cast<UState>(topNode);
            state->stateFlags |= stateFlags;
            state->superField = parentState;
            requireSymbol("{", "'State'");
            needSemicolon = false;
        } else if (token.matches(n.ignores)) {
            compileIgnores();
        } else if (token.matches(n.replication)) {
            if (topNest->nestType != NestClass) {
                fail("'Replication' is not allowed here");
            }
            requireSymbol("{", "'Replication'");
            topNode->textPos = static_cast<uint32_t>(inputPos);
            topNode->line = static_cast<uint32_t>(inputLine);
            skipStatements(1, "'Replication'");
            needSemicolon = false;
        } else if (token.matches("#")) {
            compileDirective();
            needSemicolon = false;
        } else {
            return false;
        }
        return true;
    }

    void ScriptCompiler::compileFunctionDeclaration(Token& token, bool& needSemicolon)
    {
        const Names& n = names();
        const char* nestName = nullptr;
        RetryPoint funcNameRetry;
        Token functionToken;
        int32_t precedence = 0;
        uint32_t functionFlags = 0;
        int32_t nativeIndex = 0;
        int32_t expectParms = 0;
        while (true) {
            funcNameRetry = initRetry();
            if (token.matches(n.function)) {
                checkAllow("'Function'", AllowFunction);
                nestName = "function";
            } else if (token.matches(n.operatorName)) {
                checkAllow("'Operator'", AllowFunction);
                nestName = "operator";
                functionFlags |= FUNC_Operator;
                expectParms = 3;
                if (!matchSymbol("(")) {
                    fail("Missing '(' and precedence after 'Operator'");
                } else if (!getConstInt(precedence)) {
                    fail("Missing precedence value");
                } else if (precedence < 0 || precedence > 255) {
                    fail("Bad precedence value");
                } else if (!matchSymbol(")")) {
                    fail("Missing ')' after operator precedence");
                }
            } else if (token.matches(n.preOperator)) {
                checkAllow("'PreOperator'", AllowFunction);
                nestName = "preoperator";
                expectParms = 2;
                functionFlags |= FUNC_Operator | FUNC_PreOperator;
            } else if (token.matches(n.postOperator)) {
                checkAllow("'PostOperator'", AllowFunction);
                nestName = "postoperator";
                functionFlags |= FUNC_Operator;
                expectParms = 2;
            } else if (token.matches(n.intrinsic) || token.matches(n.native)) {
                functionFlags |= FUNC_Native;
                if (matchSymbol("(")) {
                    if (!getConstInt(nativeIndex)) {
                        fail("Missing native id");
                    } else if (!matchSymbol(")")) {
                        fail("Missing ')' after internal id");
                    }
                }
            } else if (token.matches(n.event)) {
                checkAllow("'Function'", AllowFunction);
                nestName = "event";
                functionFlags |= FUNC_Event;
            } else if (token.matches(n.staticName)) {
                functionFlags |= FUNC_Static;
                if (cast<UState>(topNode) && !cast<UClass>(topNode)) {
                    fail("Static functions cannot exist in a state");
                }
            } else if (token.matches(n.simulated)) {
                functionFlags |= FUNC_Simulated;
            } else if (token.matches(n.iterator)) {
                functionFlags |= FUNC_Iterator;
            } else if (token.matches(n.singular)) {
                functionFlags |= FUNC_Singular;
            } else if (token.matches(n.latent)) {
                functionFlags |= FUNC_Latent;
            } else if (token.matches(n.exec)) {
                functionFlags |= FUNC_Exec;
            } else if (token.matches(n.final)) {
                functionFlags |= FUNC_Final;
            } else {
                break;
            }
            getToken(token);
        }
        ungetToken(token);
        if (!nestName) {
            fail("Missing 'function'");
        }
        RetryPoint start = initRetry();
        uint32_t objectFlags = 0;
        PropertyBase returnType(CptNone);
        Token testToken;
        bool hasReturnValue = false;
        if (getIdentifier(testToken, true)) {
            if (!peekSymbol("(")) {
                performRetry(start);
                hasReturnValue = getVarType(topNode, returnType, objectFlags, ~0u, nullptr);
            } else {
                performRetry(start);
            }
        }
        if (!getIdentifier(functionToken) && (!(functionFlags & FUNC_Operator) || !getSymbol(functionToken))) {
            fail(std::string("Missing ") + nestName + " name");
        }
        if (!matchSymbol("(")) {
            fail(std::string("Bad ") + nestName + " definition");
        }
        if (functionFlags & FUNC_Native) {
            if (nativeIndex && !(functionFlags & FUNC_Final)) {
                fail("Numbered native functions must be final");
            }
        } else {
            if (functionFlags & FUNC_Latent) {
                fail("Only native functions may use 'Latent'");
            }
            if (functionFlags & FUNC_Iterator) {
                fail("Only native functions may use 'Iterator'");
            }
        }
        std::string signature;
        if (functionFlags & FUNC_Operator) {
            for (char c : functionToken.identifier) {
                signature += cppTags[c - 32];
            }
            RetryPoint retry = initRetry();
            if (!matchSymbol(")")) {
                signature += "_";
                if (functionFlags & FUNC_PreOperator) {
                    signature += "Pre";
                }
                do {
                    PropertyBase property(CptNone);
                    uint32_t parmFlags = 0;
                    getVarType(topNode, property, parmFlags, ~CPF_ParmFlags, "Function parameter");
                    getVarNameAndDim(topNode, property, parmFlags, false, nullptr, "Function parameter", Name(), true);
                    if (property.type == CptObject || property.type == CptStruct) {
                        signature += property.ref->name.str();
                    } else {
                        signature += cptTypeName(property.type);
                    }
                } while (matchSymbol(","));
                requireSymbol(")", "parameter list");
            }
            performRetry(retry, true, true);
        } else {
            signature = functionToken.identifier;
        }
        pushNest(NestFunction, Name(signature), nullptr);
        UFunction* topFunction = cast<UFunction>(topNode);
        topFunction->functionFlags |= functionFlags;
        topFunction->precedence = static_cast<uint8_t>(precedence);
        topFunction->nativeIndex = static_cast<uint16_t>(nativeIndex);
        topFunction->friendlyName = Name(functionToken.identifier);
        int32_t parmCount = 0;
        if (!matchSymbol(")")) {
            bool optional = false;
            do {
                PropertyBase property(CptNone);
                uint32_t parmFlags = 0;
                getVarType(topNode, property, parmFlags, ~CPF_ParmFlags, "Function parameter");
                property.flags |= CPF_Parm;
                UProperty* parm = getVarNameAndDim(topNode, property, parmFlags, false, nullptr, "Function parameter", Name(), false);
                parmCount++;
                if ((functionFlags & FUNC_Operator) && (property.flags & ~CPF_ParmFlags)) {
                    fail("Operator parameters may not have modifiers");
                } else if (property.type == CptBool && (property.flags & CPF_OutParm)) {
                    fail("Booleans may not be out parameters");
                } else if ((property.flags & CPF_SkipParm) && (!(topFunction->functionFlags & FUNC_Native) || !(topFunction->functionFlags & FUNC_Operator) || parmCount != 2)) {
                    fail("Only parameter 2 of native operators may be 'Skip'");
                }
                if (matchSymbol("=")) {
                    parm->propertyFlags |= CPF_OptionalParm;
                }
                if (parm->propertyFlags & CPF_OptionalParm) {
                    optional = true;
                } else if (optional) {
                    fail("After an optional parameters, all other parmeters must be optional");
                }
            } while (matchSymbol(","));
            requireSymbol(")", "parameter list");
        }
        if (hasReturnValue) {
            returnType.flags |= CPF_Parm | CPF_OutParm | CPF_ReturnParm;
            getVarNameAndDim(topNode, returnType, objectFlags, true, "ReturnValue", "Function return type", Name(), false);
            parmCount++;
        }
        if (parmCount > MaxFuncParms) {
            fail("'" + topNode->name.str() + "': too many parameters");
        }
        if (functionFlags & FUNC_Operator) {
            if (parmCount != expectParms) {
                fail(std::string(nestName) + " must have " + std::to_string(expectParms - 1) + " parameters");
            }
            if (!returnProperty(topFunction)) {
                fail("Operator must have a return value");
            }
            if (!(functionFlags & FUNC_Final)) {
                fail("Operators must be declared as 'Final'");
            }
        }
        if (!peekSymbol(";")) {
            topFunction->functionFlags |= FUNC_Defined;
            if (topFunction->functionFlags & FUNC_Native) {
                fail("Native functions may only be declared, not defined");
            }
            requireSymbol("{", nestName);
            needSemicolon = false;
        }
        bool found = false;
        for (int i = nestLevel - 2; i >= 1 && !found; i--) {
            for (UField* field : fieldIterator(nest[i].node)) {
                UFunction* function = cast<UFunction>(field);
                if (!function || function->name != topNode->name || function == topNode || ((function->functionFlags ^ topFunction->functionFlags) & (FUNC_Operator | FUNC_PreOperator)) != 0) {
                    continue;
                }
                int32_t otherParms = numParms(function);
                if (function->precedence != topFunction->precedence && otherParms == parmCount) {
                    fail("Overloaded operator differs in precedence");
                }
                if (parmCount != otherParms || (!returnProperty(topFunction)) != (!returnProperty(function))) {
                    fail(std::string("Redefinition of '") + nestName + " " + functionToken.identifier + "' differs from original");
                }
                std::vector<UField*> mine = fieldIterator(topFunction);
                std::vector<UField*> theirs = fieldIterator(function);
                int count = 0;
                for (; count < otherParms; count++) {
                    UProperty* a = cast<UProperty>(mine[static_cast<size_t>(count)]);
                    UProperty* b = cast<UProperty>(theirs[static_cast<size_t>(count)]);
                    if (!PropertyBase(a).matchesType(PropertyBase(b), true)) {
                        if (a->propertyFlags & CPF_ReturnParm) {
                            fail(std::string("Redefinition of ") + nestName + " " + functionToken.identifier + " differs only by return type");
                        } else if (!(functionFlags & FUNC_Operator)) {
                            fail(std::string("Redefinition of '") + nestName + " " + functionToken.identifier + "' differs from original");
                        }
                        break;
                    }
                }
                if (count < parmCount) {
                    continue;
                }
                functionFlags |= (function->functionFlags & FUNC_FuncInherit);
                if ((function->functionFlags & FUNC_FuncOverrideMatch) != (functionFlags & FUNC_FuncOverrideMatch)) {
                    fail("Function '" + function->name.str() + "' specifiers differ from original");
                }
                if (topNode == function->outer) {
                    performRetry(funcNameRetry);
                    fail("Duplicate function '" + function->name.str() + "'");
                }
                if (function->functionFlags & FUNC_Final) {
                    performRetry(funcNameRetry);
                    fail(function->name.str() + ": Can't override a 'final' function");
                }
                topNode->superField = function;
                found = true;
                break;
            }
        }
        if (!(topFunction->functionFlags & FUNC_Defined)) {
            popNest(NestFunction, nestName);
        }
    }

    void ScriptCompiler::compileIgnores()
    {
        checkAllow("'Ignores'", AllowIgnores);
        while (true) {
            Token ignoreFunction;
            if (!getToken(ignoreFunction)) {
                fail("'Ignores': Missing probe function name");
            }
            int32_t probe = ignoreFunction.tokenName.none() ? -1 : vm::probeIndex(ignoreFunction.tokenName);
            if (probe < 0) {
                bool found = false;
                for (int i = nestLevel - 2; i >= 1 && !found; i--) {
                    for (UField* field : fieldIterator(nest[i].node)) {
                        UFunction* function = cast<UFunction>(field);
                        if (!function || function->name != ignoreFunction.tokenName) {
                            continue;
                        }
                        if (function->functionFlags & FUNC_Final) {
                            fail("'" + function->name.str() + "': Cannot ignore final functions");
                        }
                        pushNest(NestFunction, function->name, nullptr);
                        UFunction* topFunction = cast<UFunction>(topNode);
                        topFunction->functionFlags |= (function->functionFlags & FUNC_FuncOverrideMatch);
                        topFunction->superField = function;
                        UField** link = &topFunction->children;
                        for (UField* child : fieldIterator(function)) {
                            UProperty* parm = cast<UProperty>(child);
                            if (!parm || !(parm->propertyFlags & CPF_Parm)) {
                                break;
                            }
                            UProperty* property = cloneParameter(parm, topFunction);
                            *link = property;
                            link = &property->next;
                        }
                        popNest(NestFunction, "Ignores");
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    fail("'Ignores': '" + ignoreFunction.identifier + "' is not a function");
                }
            } else {
                UState* state = cast<UState>(topNode);
                state->ignoreMask &= ~(1ull << probe);
            }
            if (!matchSymbol(",")) {
                break;
            }
        }
    }
}
