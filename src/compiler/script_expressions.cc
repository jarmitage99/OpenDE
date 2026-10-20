#include "compiler/script_parser.h"

#include "core/objects.h"
#include "core/vm.h"
#include "engine/package.h"

#include <algorithm>
#include <climits>

namespace dx::compiler::detail {
    void ScriptCompiler::requireSizeOfParm(Token& typeToken, const char* tag)
    {
        RetryPoint retry = initRetry();
        requireSymbol("(", tag);
        if (!compileExpr(PropertyBase(CptNone), tag, &typeToken)) {
            fail(std::string("Bad or missing expression in ") + tag);
        }
        requireSymbol(")", tag);
        performRetry(retry, true, false);
    }

    UClass* ScriptCompiler::getQualifiedClass(const char* thing)
    {
        std::string className;
        while (true) {
            Token token;
            if (!getIdentifier(token)) {
                break;
            }
            className += token.identifier;
            if (!matchSymbol(".")) {
                break;
            }
            className += ".";
        }
        if (!className.empty()) {
            UClass* result = lookup.findClass(className);
            if (!result) {
                fail("Class '" + className + "' not found");
            }
            return result;
        }
        if (thing) {
            fail(std::string(thing) + ": Missing class name");
        }
        return nullptr;
    }

    UField* ScriptCompiler::findField(UStruct* scope, const std::string& identifier, const char* fieldClass, const char* thing)
    {
        Name name = Name::find(identifier);
        if (name.none()) {
            return nullptr;
        }
        for (; scope; scope = cast<UStruct>(scope->outer)) {
            for (UField* field : fieldIterator(scope)) {
                if (field->name == name) {
                    if (!isA(field, fieldClass)) {
                        if (thing) {
                            fail(std::string(thing) + ": expecting " + fieldClass + ", got " + field->cls->name.str());
                        }
                        return nullptr;
                    }
                    return field;
                }
            }
        }
        return nullptr;
    }

    bool ScriptCompiler::compileFieldExpr(UStruct* scope, PropertyBase& requiredType, Token token, Token& resultType, bool isSelf, bool isConcrete)
    {
        RetryPoint retry = initRetry();
        bool forceFinal = false;
        bool global = false;
        bool isStatic = false;
        bool isDefault = false;
        const char* fieldClass = nullptr;
        const Names& n = names();
        if (token.matches(n.defaultName)) {
            isDefault = true;
            fieldClass = "Property";
            requireSymbol(".", "'default'");
            getToken(token);
        } else if (token.matches(n.staticName)) {
            isStatic = true;
            fieldClass = "Function";
            requireSymbol(".", "'static'");
            getToken(token);
        } else if (token.matches(n.global)) {
            if (!isSelf) {
                fail("Can only use 'global' with self");
            }
            if (!isConcrete) {
                fail("Can only use 'global' with concrete objects");
            }
            scope = ownerClass(scope);
            fieldClass = "Function";
            global = true;
            isSelf = false;
            requireSymbol(".", "'global'");
            getToken(token);
        } else if (token.matches(n.super) && !peekSymbol("(")) {
            if (!isSelf) {
                fail("Can only use 'super' with self");
            }
            while (scope && !inheritanceSuper(scope)) {
                scope = cast<UStruct>(scope->outer);
            }
            if (!scope) {
                fail("Can't use 'super': no superclass");
            }
            scope = scope->superStruct();
            fieldClass = "Function";
            forceFinal = true;
            isSelf = false;
            requireSymbol(".", "'super'");
            getToken(token);
        } else if (token.matches(n.super)) {
            if (!isSelf) {
                fail("Can only use 'super(classname)' with self");
            }
            requireSymbol("(", "'super'");
            Token classToken;
            if (!getIdentifier(classToken)) {
                fail("Missing class name");
            }
            scope = lookup.findClass(classToken.identifier);
            if (!scope) {
                fail("Bad class name '" + classToken.identifier + "'");
            }
            requireSymbol(")", "'super(classname)'");
            requireSymbol(".", "'super(classname)'");
            fieldClass = "Function";
            forceFinal = true;
            isSelf = false;
            getToken(token);
        }
        UField* field = findField(scope, token.identifier, fieldClass ? fieldClass : "Field");
        if (field && cast<UEnum>(field) && matchSymbol(".")) {
            UEnum* enumType = cast<UEnum>(field);
            int32_t enumIndex = IndexNone;
            if (!getToken(token)) {
                fail("Missing enum tag after '" + enumType->name.str() + "'");
            } else if (token.tokenName == n.enumCount) {
                enumIndex = static_cast<int32_t>(enumType->names.size());
            } else {
                auto it = std::find(enumType->names.begin(), enumType->names.end(), token.tokenName);
                if (token.tokenName.none() || it == enumType->names.end()) {
                    fail("Missing enum tag after '" + enumType->name.str() + "'");
                }
                enumIndex = static_cast<int32_t>(it - enumType->names.begin());
            }
            resultType.setConstByte(enumType, static_cast<uint8_t>(enumIndex));
            resultType.flags &= ~CPF_OutParm;
            emitConstant(resultType);
            return true;
        }
        if (field && cast<UProperty>(field)) {
            UProperty* property = cast<UProperty>(field);
            if (!(property->flags & RF_Public) && ownerClass(property) != compiling) {
                fail("Can't access private variable '" + property->name.str() + "' in '" + ownerClass(property)->name.str() + "'");
            }
            if (isDefault && !cast<UClass>(property->outer)) {
                fail("You can't access the default value of static and local variables");
            }
            if (!isConcrete && !isDefault && !cast<UFunction>(property->outer)) {
                fail("You can only access default values of variables here");
            }
            if (property->type == PropType::Bool) {
                emit(ExBoolVariable);
            }
            emit(isDefault ? ExDefaultVariable : cast<UFunction>(property->outer) ? ExLocalVariable
                                                                                  : ExInstanceVariable);
            emitObject(property);
            resultType = Token(PropertyBase(property));
            resultType.flags |= CPF_OutParm;
            if (property->name == n.classOf && property->outer == objects::objectClass()) {
                resultType.metaClass = ownerClass(scope);
            }
            if (property->name == n.outer && property->outer == objects::objectClass()) {
                resultType.ref = ownerClass(scope)->within;
            }
            return true;
        }
        if (field && cast<UFunction>(field) && matchSymbol("(")) {
            UFunction* function = cast<UFunction>(field);
            gotAffector = true;
            if (function->functionFlags & FUNC_Latent) {
                checkAllow(function->name.str(), AllowStateCmd);
            }
            if (!(function->functionFlags & FUNC_Static) && !isConcrete) {
                fail("Can't call instance functions from within static functions");
            }
            if (isStatic && !(function->functionFlags & FUNC_Static)) {
                fail("Function '" + function->name.str() + "' is not static");
            }
            if (function->functionFlags & FUNC_Iterator) {
                checkAllow(function->name.str(), AllowIterator);
                topNest->allow &= ~AllowIterator;
            }
            emitStackNodeLinkFunction(function, forceFinal, global);
            std::vector<UProperty*> parms;
            for (UField* child : fieldIterator(function)) {
                if (UProperty* parm = cast<UProperty>(child)) {
                    parms.push_back(parm);
                }
            }
            bool isIteratorCast = false;
            if ((function->functionFlags & FUNC_Iterator) && numParms(function) >= 2 && parms.size() >= 2) {
                UProperty* a = parms[0];
                UProperty* b = parms[1];
                if (a->type == PropType::Class || a->type == PropType::Object) {
                    if (b->type == PropType::Object || b->type == PropType::Class) {
                        if (a->objectClass == objects::classClass()) {
                            isIteratorCast = true;
                        }
                    }
                }
            }
            UClass* iteratorClass = nullptr;
            Token parmToken[MaxFuncParms];
            int count = 0;
            size_t it = 0;
            auto isParm = [&](size_t index) {
                return index < parms.size() && (parms[index]->propertyFlags & (CPF_Parm | CPF_ReturnParm)) == CPF_Parm;
            };
            for (; isParm(it); ++it, ++count) {
                PropertyBase parm(parms[it]);
                if (parm.flags & CPF_ReturnParm) {
                    break;
                }
                if (count == 1 && iteratorClass) {
                    parm.ref = iteratorClass;
                }
                if (count != 0 && !matchSymbol(",")) {
                    if (!(parm.flags & CPF_OptionalParm)) {
                        fail("Call to '" + function->name.str() + "': missing or bad parameter " + std::to_string(count + 1));
                    }
                    break;
                }
                if (peekSymbol("[")) {
                    Token temp;
                    Token nameToken;
                    getToken(temp);
                    if (!getToken(nameToken) || nameToken.kind != TokenIdentifier) {
                        fail("Call to '" + function->name.str() + "': Missing name in named parameter spec");
                    } else if (nameToken.tokenName.none()) {
                        fail("Call to '" + function->name.str() + "': Bad named parameter " + nameToken.identifier);
                    }
                    if (!matchSymbol("]")) {
                        fail("Call to '" + function->name.str() + "': Bad named parameter spec");
                    }
                    bool found = false;
                    for (; isParm(it); ++it, ++count) {
                        if (parms[it]->name == nameToken.tokenName) {
                            found = true;
                            break;
                        }
                        if (!(parms[it]->propertyFlags & CPF_OptionalParm)) {
                            fail("Call to '" + function->name.str() + "': Required parameter " + parms[it]->name.str() + " skipped");
                        }
                        emit(ExNothing);
                    }
                    if (!found) {
                        fail("Call to '" + function->name.str() + "': Named parameter " + nameToken.tokenName.str() + " not found");
                    }
                    parm = PropertyBase(parms[it]);
                    if (parm.flags & CPF_ReturnParm) {
                        break;
                    }
                }
                int result = compileExpr(parm, nullptr, &parmToken[count]);
                if (result == -1) {
                    fail("Call to '" + token.identifier + "': type mismatch in parameter " + std::to_string(count + 1));
                } else if (result == 0) {
                    if (!(parm.flags & CPF_OptionalParm)) {
                        fail("Call to '" + token.identifier + "': bad or missing parameter " + std::to_string(count + 1));
                    }
                    if (peekSymbol(")")) {
                        break;
                    }
                    emit(ExNothing);
                } else if (isIteratorCast && count == 0) {
                    UObject* value = nullptr;
                    if (parmToken[count].getConstObject(objects::classClass(), value)) {
                        iteratorClass = cast<UClass>(value);
                    }
                }
            }
            for (; isParm(it); ++it) {
                if (!(parms[it]->propertyFlags & CPF_OptionalParm)) {
                    fail("Call to '" + function->name.str() + "': Required parameter " + parms[it]->name.str() + " skipped");
                }
            }
            Token temp;
            if (!getToken(temp) || temp.kind != TokenSymbol) {
                fail("Call to '" + token.identifier + "': Bad expression or missing ')'");
            }
            if (!temp.matches(")")) {
                fail("Call to '" + token.identifier + "': Bad '" + temp.identifier + "' or missing ')'");
            }
            emit(ExEndFunctionParms);
            if (it < parms.size() && (parms[it]->propertyFlags & CPF_ReturnParm)) {
                resultType = Token(PropertyBase(parms[it]));
                resultType.flags &= ~CPF_OutParm;
                if (token.matches(n.spawn)) {
                    resultType.ref = parmToken[0].metaClass;
                }
            } else {
                resultType = Token(PropertyBase(CptNone));
            }
            resultType.flags &= ~CPF_OutParm;
            return true;
        }
        if (field && cast<UConst>(field)) {
            UConst* constant = cast<UConst>(field);
            RetryPoint constRetry = initRetry();
            input = constant->value.c_str();
            inputLength = static_cast<int32_t>(constant->value.size());
            inputPos = 0;
            if (!getToken(resultType, &requiredType) || resultType.kind != TokenConst) {
                fail("Error in constant");
            }
            performRetry(constRetry, false, true);
            resultType.attemptToConvertConstant(lookup, requiredType);
            resultType.flags &= ~CPF_OutParm;
            emitConstant(resultType);
            return true;
        }
        if (fieldClass) {
            fail(std::string("Unknown ") + fieldClass + " '" + token.identifier + "' in '" + scope->fullName() + "'");
        }
        performRetry(retry);
        return false;
    }

    int ScriptCompiler::conversionCost(const PropertyBase& dest, const PropertyBase& source)
    {
        uint32_t conversion = getConversion(dest, source);
        if (dest.matchesType(source, true)) {
            return 0;
        }
        if (dest.flags & CPF_OutParm) {
            return INT_MAX;
        }
        if (dest.matchesType(source, false)) {
            int result = 1;
            if (source.type == CptObject && source.ref != nullptr) {
                for (UClass* test = source.propertyClass(); test && test != dest.propertyClass(); test = test->superClass()) {
                    result++;
                }
            }
            return result;
        }
        if (dest.arrayDim != 1 || source.arrayDim != 1) {
            return INT_MAX;
        }
        if (dest.type == CptByte && dest.ref != nullptr) {
            return INT_MAX;
        }
        if (dest.type == CptObject && dest.ref != nullptr) {
            return INT_MAX;
        }
        if ((dest.flags & CPF_CoerceParm) ? (conversion == ExMax) : !(conversion & AutoConvert)) {
            return INT_MAX;
        }
        if (getConversion(dest, source) & Truncate) {
            return 104;
        }
        if ((source.type == CptInt || source.type == CptByte) && dest.type == CptFloat) {
            return 103;
        }
        return 101;
    }

    bool ScriptCompiler::compileDynamicCast(Token token, Token& resultType)
    {
        RetryPoint lowRetry = initRetry();
        if (!matchSymbol("(") && !peekSymbol("<")) {
            return false;
        }
        UClass* destClass = lookup.findClass(token.identifier);
        if (!destClass) {
            performRetry(lowRetry);
            return false;
        }
        UClass* metaClass = objects::objectClass();
        if (destClass == objects::classClass()) {
            if (matchSymbol("<")) {
                Token metaToken;
                if (!getIdentifier(metaToken)) {
                    return false;
                }
                metaClass = lookup.findClass(metaToken.identifier);
                if (!metaClass || !matchSymbol(">") || !matchSymbol("(")) {
                    performRetry(lowRetry);
                    return false;
                }
            }
        }
        Token tempType;
        PropertyBase requiredType(objects::objectClass());
        if (compileExpr(requiredType, nullptr, &tempType) != 1 || !matchSymbol(")")) {
            performRetry(lowRetry);
            return false;
        }
        UClass* tempClass = tempType.propertyClass();
        if ((!tempClass || tempClass->isChildOf(destClass)) && (tempClass != objects::classClass() || destClass != objects::classClass() || (tempType.metaClass && tempType.metaClass->isChildOf(metaClass)))) {
            fail(std::string("Cast from '") + (tempClass ? tempClass->name.str() : "None") + "' to '" + destClass->name.str() + "' is unnecessary");
        } else if (destClass->isChildOf(tempClass) && (destClass != objects::classClass() || tempType.metaClass == nullptr || metaClass->isChildOf(tempType.metaClass))) {
            RetryPoint highRetry = initRetry();
            if (destClass == objects::classClass()) {
                emit(ExMetaCast);
                emitObject(metaClass);
            } else {
                emit(ExDynamicCast);
                emitObject(destClass);
            }
            codeSwitcheroo(lowRetry, highRetry);
        } else {
            fail("Cast from '" + tempClass->name.str() + "' to '" + destClass->name.str() + "' will always fail");
        }
        resultType = Token(PropertyBase(destClass));
        if (resultType.ref == objects::classClass()) {
            resultType.metaClass = metaClass;
        }
        return true;
    }

    int ScriptCompiler::compileExpr(PropertyBase requiredType, const char* errorTag, Token* resultToken, int maxPrecedence, PropertyBase* hintType)
    {
        RetryPoint lowRetry = initRetry();
        Token token { PropertyBase(CptNone) };
        const Names& n = names();
        if (!getToken(token, hintType ? hintType : &requiredType)) {
            token.setBase(PropertyBase(CptNone));
        } else if (token.kind == TokenConst) {
            token.attemptToConvertConstant(lookup, requiredType);
            token.flags &= ~CPF_OutParm;
            emitConstant(token);
        } else if (token.matches("(")) {
            if (!compileExpr(requiredType, nullptr, &token)) {
                fail("Bad or missing expression in parenthesis");
            }
            requireSymbol(")", "expression");
            if (token.type == CptNone) {
                fail("Bad or missing expression in parenthesis");
            }
        } else if ((token.tokenName == n.byteName || token.tokenName == n.intName || token.tokenName == n.boolName || token.tokenName == n.floatName || token.tokenName == n.nameName || token.tokenName == n.stringName || token.tokenName == n.structName || token.tokenName == n.vector || token.tokenName == n.rotator) && matchSymbol("(")) {
            PropertyBase toType(CptNone);
            if (token.tokenName == n.vector) {
                toType = PropertyBase(lookup.findStruct("Vector"));
            } else if (token.tokenName == n.rotator) {
                toType = PropertyBase(lookup.findStruct("Rotator"));
            } else {
                int type = token.tokenName == n.byteName ? CptByte : token.tokenName == n.intName ? CptInt
                    : token.tokenName == n.boolName                                               ? CptBool
                    : token.tokenName == n.floatName                                              ? CptFloat
                    : token.tokenName == n.nameName                                               ? CptName
                    : token.tokenName == n.stringName                                             ? CptString
                                                                                                  : CptStruct;
                toType = PropertyBase(type);
            }
            Token fromType;
            std::string tag = token.tokenName.str();
            compileExpr(PropertyBase(CptNone), tag.c_str(), &fromType);
            if (fromType.type == CptNone) {
                fail("'" + tag + "' conversion: Bad or missing expression");
            }
            uint32_t conversion = getConversion(toType, fromType);
            if (conversion != ExMax) {
                RetryPoint highRetry = initRetry();
                emit(static_cast<uint8_t>(conversion & ConvertMask));
                codeSwitcheroo(lowRetry, highRetry);
                token = Token(toType);
            } else if (toType.type == fromType.type) {
                fail(std::string("No need to cast '") + cptTypeName(toType.type) + "' to itself");
            } else {
                fail(std::string("Can't convert '") + cptTypeName(fromType.type) + "' to '" + cptTypeName(toType.type) + "'");
            }
            token.flags &= ~CPF_OutParm;
            if (!matchSymbol(")")) {
                fail("Missing ')' in type conversion");
            }
        } else if (token.tokenName == n.self) {
            checkAllow("'self'", AllowInstance);
            emit(ExSelf);
            token = Token(PropertyBase(compiling));
        } else if (token.tokenName == n.newName) {
            emit(ExNew);
            bool paren = matchSymbol("(") && !matchSymbol(")");
            bool going = paren;
            UClass* parentClass = compiling;
            if (going) {
                Token newParent;
                compileExpr(PropertyBase(objects::objectClass()), "'new' parent object", &newParent);
                if (!newParent.ref) {
                    newParent.ref = objects::objectClass();
                }
                parentClass = newParent.propertyClass();
                going = matchSymbol(",");
            } else {
                emit(ExNothing);
            }
            if (going) {
                compileExpr(PropertyBase(CptName), "'new' name");
                going = matchSymbol(",");
            } else {
                emit(ExNothing);
            }
            if (going) {
                compileExpr(PropertyBase(CptInt), "'new' flags");
            } else {
                emit(ExNothing);
            }
            if (paren) {
                requireSymbol(")", "'new'");
            }
            Token newClass;
            compileExpr(PropertyBase(objects::classClass(), objects::objectClass()), "'new'", &newClass);
            if (!newClass.metaClass) {
                fail("'new': Invalid class");
            }
            token = Token(PropertyBase(newClass.metaClass));
            if (!parentClass || !parentClass->isChildOf(newClass.metaClass->within)) {
                fail("'new': " + newClass.metaClass->name.str() + " objects must reside in " + newClass.metaClass->within->name.str() + " objects, not " + (parentClass ? parentClass->name.str() : std::string("None")) + " objects");
            }
            if (matchSymbol("(")) {
                requireSymbol(")", "'new' constructor parameters");
            }
        } else if (compileDynamicCast(token, token)) {
        } else if (compileFieldExpr(topNode, requiredType, token, token, true, (topNest->allow & AllowInstance) != 0)) {
        } else {
            ungetToken(token);
            token.setBase(PropertyBase(CptNone));
        }
        while (true) {
            if (token.arrayDim != 1) {
                if (!matchSymbol("[")) {
                    break;
                }
                RetryPoint highRetry = initRetry();
                emit(token.arrayDim > 1 ? ExArrayElement : ExDynArrayElement);
                token.arrayDim = 1;
                compileExpr(PropertyBase(CptInt), "array index");
                if (!matchSymbol("]")) {
                    fail(token.identifier + " is an array; expecting ']'");
                }
                codeSwitcheroo(lowRetry, highRetry);
            } else if (token.type == CptStruct && matchSymbol(".")) {
                Token tag;
                getToken(tag);
                UProperty* member = nullptr;
                for (UField* child : fieldIterator(token.structType())) {
                    UProperty* property = cast<UProperty>(child);
                    if (property && property->name == tag.tokenName) {
                        member = property;
                        break;
                    }
                }
                if (!member) {
                    fail("Unknown member '" + tag.identifier + "' in struct '" + token.structType()->name.str() + "'");
                }
                uint32_t originalFlags = token.flags;
                token.setBase(PropertyBase(member));
                token.flags = originalFlags | (token.flags & CPF_PropagateFromStruct);
                RetryPoint highRetry = initRetry();
                if (member->type == PropType::Bool) {
                    emit(ExBoolVariable);
                }
                emit(ExStructMember);
                emitObject(member);
                codeSwitcheroo(lowRetry, highRetry);
            } else if (token.type == CptObject && matchSymbol(".")) {
                Token originalToken = token;
                addDependency(compiling, originalToken.propertyClass(), false);
                if (token.ref == objects::classClass()) {
                    addDependency(compiling, token.metaClass, false);
                    if (peekIdentifier(n.defaultName) || peekIdentifier(n.staticName)) {
                        RetryPoint highRetry = initRetry();
                        emit(ExClassContext);
                        codeSwitcheroo(lowRetry, highRetry);
                        getToken(token);
                        RetryPoint contextStart = initRetry();
                        PropertyBase required = requiredType;
                        if (!compileFieldExpr(token.metaClass, required, token, token, false, false)) {
                            fail("'" + token.metaClass->name.str() + "': Bad context expression");
                        }
                        RetryPoint contextEnd = initRetry();
                        emitWord(static_cast<uint16_t>(codeTop() - contextStart.codeTop));
                        emitSize(token.size(), "Context expression");
                        codeSwitcheroo(contextStart, contextEnd);
                        continue;
                    }
                }
                RetryPoint highRetry = initRetry();
                emit(ExContext);
                codeSwitcheroo(lowRetry, highRetry);
                Token tempToken;
                getToken(tempToken);
                RetryPoint contextStart = initRetry();
                PropertyBase required = requiredType;
                if (!compileFieldExpr(originalToken.propertyClass(), required, tempToken, token, false, true)) {
                    fail("Unrecognized member '" + tempToken.identifier + "' in class '" + originalToken.propertyClass()->name.str() + "'");
                }
                RetryPoint contextEnd = initRetry();
                emitWord(static_cast<uint16_t>(codeTop() - contextStart.codeTop));
                emitSize(token.size(), "Context expression");
                codeSwitcheroo(contextStart, contextEnd);
            } else {
                break;
            }
        }
        while (true) {
            Token operToken;
            int precedence = 0;
            int bestMatch = 0;
            int matches = 0;
            int32_t numParmsLimit = 3;
            std::vector<UFunction*> operLinks;
            bool isPreOperator = token.type == CptNone;
            uint32_t requiredFunctionFlags = (isPreOperator ? FUNC_PreOperator : 0) | FUNC_Operator;
            bool handled = false;
            if (getToken(operToken, nullptr, true)) {
                if (!operToken.tokenName.none()) {
                    for (int i = nestLevel - 1; i >= 1; i--) {
                        for (UField* child : fieldIterator(nest[i].node)) {
                            UFunction* function = cast<UFunction>(child);
                            if (function && function->friendlyName == operToken.tokenName && requiredFunctionFlags == (function->functionFlags & (FUNC_PreOperator | FUNC_Operator))) {
                                operLinks.push_back(function);
                                precedence = function->precedence;
                                numParmsLimit = std::min(numParmsLimit, numParms(function));
                            }
                        }
                    }
                    if (!operLinks.empty() && precedence < maxPrecedence) {
                        RetryPoint midRetry = initRetry();
                        PropertyBase newRequiredType(CptNone);
                        Token newResultType;
                        if (numParmsLimit == 3 || isPreOperator) {
                            std::string newErrorTag = "Following '" + operToken.tokenName.str() + "'";
                            compileExpr(newRequiredType, newErrorTag.c_str(), &newResultType, precedence, &token);
                            if (newResultType.type == CptNone) {
                                fail("Bad or missing expression after '" + operToken.tokenName.str() + "'");
                            }
                        }
                        UFunction* bestOperLink = nullptr;
                        bool anyLeftValid = false;
                        bool anyRightValid = false;
                        for (UFunction* node : operLinks) {
                            int thisMatch = 0;
                            std::vector<UProperty*> nodeParms;
                            for (UField* child : fieldIterator(node)) {
                                if (UProperty* parm = cast<UProperty>(child)) {
                                    nodeParms.push_back(parm);
                                }
                            }
                            size_t at = 0;
                            int32_t nodeParmCount = numParms(node);
                            if (nodeParmCount == 3 || !isPreOperator) {
                                int cost = conversionCost(PropertyBase(nodeParms[at++]), token);
                                thisMatch = cost;
                                anyLeftValid = anyLeftValid || cost != INT_MAX;
                            }
                            if (nodeParmCount == 3 || isPreOperator) {
                                int cost = conversionCost(PropertyBase(nodeParms[at++]), newResultType);
                                thisMatch = std::max(thisMatch, cost);
                                anyRightValid = anyRightValid || cost != INT_MAX;
                            }
                            if ((!bestOperLink || thisMatch < bestMatch) && nodeParmCount == numParmsLimit) {
                                bestOperLink = node;
                                bestMatch = thisMatch;
                                matches = 1;
                            } else if (thisMatch == bestMatch) {
                                matches++;
                            }
                        }
                        if (bestMatch == INT_MAX) {
                            std::string symbol = operToken.tokenName.str();
                            if ((symbol == "==" || symbol == "!=") && token.type == CptStruct && newResultType.type == CptStruct && token.ref == newResultType.ref) {
                                RetryPoint highRetry = initRetry();
                                emit(symbol == "==" ? ExStructCmpEq : ExStructCmpNe);
                                emitObject(token.structType());
                                codeSwitcheroo(lowRetry, highRetry);
                                token = Token(PropertyBase(CptBool));
                                handled = true;
                            } else if (anyLeftValid && !anyRightValid) {
                                fail("Right type is incompatible with '" + symbol + "'");
                            } else if (anyRightValid && !anyLeftValid) {
                                fail("Left type is incompatible with '" + symbol + "'");
                            } else {
                                fail("Types are incompatible with '" + symbol + "'");
                            }
                        } else if (matches > 1) {
                            fail("Operator '" + operToken.tokenName.str() + "': Can't resolve overload (" + std::to_string(matches) + " matches of quality " + std::to_string(bestMatch) + ")");
                        } else {
                            std::vector<UProperty*> bestParms;
                            for (UField* child : fieldIterator(bestOperLink)) {
                                if (UProperty* parm = cast<UProperty>(child)) {
                                    bestParms.push_back(parm);
                                }
                            }
                            UProperty* operParm1 = bestParms[0];
                            UProperty* operParm2 = isPreOperator ? bestParms[0] : bestParms[1];
                            UProperty* operReturn = returnProperty(bestOperLink);
                            if (numParms(bestOperLink) == 3 || isPreOperator) {
                                if (operParm2->propertyFlags & CPF_OutParm) {
                                    gotAffector = true;
                                }
                                if (newResultType.type != PropertyBase(operParm2).type) {
                                    RetryPoint highRetry = initRetry();
                                    emit(static_cast<uint8_t>(getConversion(PropertyBase(operParm2), newResultType) & ConvertMask));
                                    codeSwitcheroo(midRetry, highRetry);
                                }
                                if (operParm2->propertyFlags & CPF_SkipParm) {
                                    RetryPoint highRetry = initRetry();
                                    emit(ExSkip);
                                    emitWord(static_cast<uint16_t>(1 + highRetry.codeTop - midRetry.codeTop));
                                    codeSwitcheroo(midRetry, highRetry);
                                }
                            }
                            if (!isPreOperator) {
                                if (operParm1->propertyFlags & CPF_OutParm) {
                                    gotAffector = true;
                                }
                                if (token.type != PropertyBase(operParm1).type) {
                                    RetryPoint highRetry = initRetry();
                                    emit(static_cast<uint8_t>(getConversion(PropertyBase(operParm1), token) & ConvertMask));
                                    codeSwitcheroo(lowRetry, highRetry);
                                }
                            }
                            RetryPoint highRetry = initRetry();
                            emitStackNodeLinkFunction(bestOperLink, true, false);
                            codeSwitcheroo(lowRetry, highRetry);
                            emit(ExEndFunctionParms);
                            token = Token(PropertyBase(operReturn));
                            token.flags &= ~CPF_OutParm;
                            handled = true;
                        }
                    }
                }
            }
            if (!handled) {
                ungetToken(operToken);
                break;
            }
        }
        if (token.type == CptNone && requiredType.type != CptNone) {
            if (errorTag) {
                fail(std::string("Bad or missing expression in ") + errorTag);
            }
            if (resultToken) {
                *resultToken = token;
            }
            return 0;
        }
        if (!requiredType.matchesType(token, false)) {
            uint32_t conversion = getConversion(requiredType, token);
            if (requiredType.flags & CPF_OutParm) {
                if (errorTag) {
                    if (token.kind == TokenConst) {
                        fail("Expecting a variable, not a constant");
                    } else if (token.flags & CPF_Const) {
                        fail(std::string("Const mismatch in Out variable ") + errorTag);
                    } else {
                        fail(std::string("Type mismatch in Out variable ") + errorTag);
                    }
                }
                if (resultToken) {
                    *resultToken = token;
                }
                return -1;
            } else if (requiredType.arrayDim != 1 || token.arrayDim != 1) {
                if (errorTag) {
                    fail(std::string("Array mismatch in ") + errorTag);
                }
                if (resultToken) {
                    *resultToken = token;
                }
                return -1;
            } else if (((requiredType.flags & CPF_CoerceParm) ? (conversion != ExMax) : (conversion & AutoConvert)) && (requiredType.type != CptByte || requiredType.ref == nullptr)) {
                RetryPoint highRetry = initRetry();
                emit(static_cast<uint8_t>(getConversion(requiredType, token) & ConvertMask));
                codeSwitcheroo(lowRetry, highRetry);
                token.flags &= ~CPF_OutParm;
                token = Token(PropertyBase(requiredType.type));
            } else {
                if (errorTag) {
                    fail(std::string("Type mismatch in ") + errorTag);
                }
                if (resultToken) {
                    *resultToken = token;
                }
                return -1;
            }
        }
        if (resultToken) {
            *resultToken = token;
        }
        return token.type != CptNone ? 1 : 0;
    }
}
