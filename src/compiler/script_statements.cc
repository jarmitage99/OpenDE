#include "compiler/script_parser.h"

#include "core/objects.h"
#include "core/vm.h"
#include "engine/package.h"

#include <algorithm>

namespace dx::compiler::detail {
    const char* ScriptCompiler::nestTypeName(NestType type)
    {
        switch (type) {
        case NestNone:
            return "Global Scope";
        case NestClass:
            return "Class";
        case NestState:
            return "State";
        case NestFunction:
            return "Function";
        case NestIf:
            return "If";
        case NestLoop:
            return "Loop";
        case NestSwitch:
            return "Switch";
        case NestFor:
            return "For";
        case NestForEach:
            return "ForEach";
        }
        return "Unknown";
    }

    void ScriptCompiler::checkAllow(const std::string& thing, uint32_t allowFlags)
    {
        if ((topNest->allow & allowFlags) != allowFlags) {
            if (topNest->nestType == NestNone) {
                fail(thing + " is not allowed before the Class definition");
            }
            fail(thing + " is not allowed here");
        }
        if (allowFlags & AllowCmd) {
            topNest->allow &= ~(AllowVarDecl | AllowFunction | AllowIgnores);
        }
    }

    void ScriptCompiler::pushNest(NestType nestType, Name thisName, UStruct* inNode)
    {
        UStruct* prevTopNode = topNode;
        UStruct* prevNode = nullptr;
        uint32_t prevAllow = 0;
        if (pass == 0 && (nestType == NestState || nestType == NestFunction)) {
            for (UField* field : fieldIterator(topNode)) {
                if (field->outer != topNode) {
                    break;
                }
                if (field->name == thisName) {
                    fail("'" + thisName.str() + "' conflicts with '" + field->fullName() + "'");
                }
            }
        }
        topNest = &nest[nestLevel++];
        topNode = nullptr;
        topNest->node = inNode;
        topNest->nestType = nestType;
        topNest->codeChain = IndexNone;
        topNest->switchType = Token(PropertyBase(CptNone));
        topNest->fixupList.clear();
        topNest->labelList.clear();
        for (int32_t& fixup : topNest->fixups) {
            fixup = MaxWord;
        }
        if (nestLevel >= MaxNestLevels) {
            fail("Maximum nesting limit exceeded");
        }
        bool isNewNode = nestType == NestClass || nestType == NestState || nestType == NestFunction;
        if (nestLevel > 1) {
            if (pass == 1) {
                if (!isNewNode) {
                    topNest->node = topNest[-1].node;
                }
                topNode = topNest[0].node;
            } else {
                if (isNewNode) {
                    UObject* outer = prevTopNode ? static_cast<UObject*>(prevTopNode) : static_cast<UObject*>(compiling);
                    if (nestType == NestClass) {
                        topNest->node = topNode = compiling;
                        compiling->probeMask = 0;
                        compiling->ignoreMask = ~0ull;
                        compiling->labelTableOffset = MaxWord;
                    } else if (nestType == NestState) {
                        UState* state = createField(new UState(), outer, thisName, "State", RF_Public);
                        state->probeMask = 0;
                        state->ignoreMask = ~0ull;
                        state->labelTableOffset = MaxWord;
                        topNest->node = topNode = state;
                    } else {
                        UFunction* function = createField(new UFunction(), outer, thisName, "Function", RF_Public);
                        function->replicationOffset = MaxWord;
                        topNest->node = topNode = function;
                    }
                    topNode->friendlyName = thisName;
                    topNode->textPos = 0xffffffffu;
                    topNode->line = 0xffffffffu;
                } else {
                    topNest->node = topNest[-1].node;
                    topNode = topNest->node;
                }
            }
            prevNode = topNest[-1].node;
            prevAllow = topNest[-1].allow;
        }
        switch (nestType) {
        case NestNone:
            topNest->allow = AllowClass;
            break;
        case NestClass:
            topNest->allow = AllowVarDecl | AllowFunction | AllowState | AllowIgnores | AllowInstance;
            break;
        case NestState:
            topNest->allow = AllowFunction | AllowLabel | AllowStateCmd | AllowIgnores | AllowInstance;
            if (pass == 0) {
                topNode->next = prevNode->children;
                prevNode->children = topNode;
            }
            break;
        case NestFunction:
            topNest->allow = AllowVarDecl | AllowReturn | AllowCmd | AllowLabel;
            if (!(cast<UFunction>(topNode)->functionFlags & FUNC_Static)) {
                topNest->allow |= AllowInstance;
            }
            if (pass == 0) {
                topNode->next = prevNode->children;
                prevNode->children = topNode;
            }
            break;
        case NestIf:
            topNest->allow = AllowElseIf | (prevAllow & (AllowCmd | AllowLabel | AllowBreak | AllowContinue | AllowStateCmd | AllowReturn | AllowInstance));
            break;
        case NestLoop:
            topNest->allow = AllowBreak | AllowContinue | (prevAllow & (AllowCmd | AllowLabel | AllowStateCmd | AllowReturn | AllowInstance));
            break;
        case NestSwitch:
            topNest->allow = AllowCase | AllowDefault | (prevAllow & (AllowStateCmd | AllowReturn | AllowInstance));
            break;
        case NestFor:
            topNest->allow = AllowBreak | AllowContinue | (prevAllow & (AllowCmd | AllowLabel | AllowStateCmd | AllowReturn | AllowInstance));
            break;
        case NestForEach:
            topNest->allow = AllowIterator | AllowBreak | AllowContinue | (prevAllow & (AllowCmd | AllowLabel | AllowReturn | AllowInstance));
            break;
        }
    }

    void ScriptCompiler::popNest(NestType nestType, const std::string& descr)
    {
        if (topNode && topNode->script.size() > 65534) {
            fail("Code space for " + topNode->name.str() + " overflowed by " + std::to_string(topNode->script.size() - 65534) + " bytes");
        }
        if (nestLevel <= 0) {
            fail("Unexpected '" + descr + "' at global scope");
        } else if (nestType == NestNone) {
            nestType = topNest->nestType;
        } else if (topNest->nestType != nestType) {
            fail("Unexpected end of " + descr + " in '" + nestTypeName(topNest->nestType) + "' block");
        }
        if (pass == 0) {
            if (nestType == NestState || nestType == NestFunction) {
                topNode->textPos = static_cast<uint32_t>(inputPos);
                topNode->line = static_cast<uint32_t>(inputLine);
            } else if (nestType != NestClass) {
                fail("Bad first pass NestType");
            }
        } else {
            if (nestType == NestState) {
                emit(ExStop);
                if (!topNest->labelList.empty()) {
                    while ((topNode->script.size() & 3) != 3) {
                        emit(ExNothing);
                    }
                    emit(ExLabelTable);
                    cast<UState>(topNode)->labelTableOffset = static_cast<uint16_t>(codeTop());
                    for (const LabelRecord& label : topNest->labelList) {
                        emitName(label.name);
                        emitDword(static_cast<uint32_t>(label.code));
                    }
                    emitName(Name());
                    emitDword(MaxWord);
                }
            } else if (nestType == NestFunction) {
                UFunction* topFunction = cast<UFunction>(topNode);
                if (!(topFunction->functionFlags & FUNC_Native)) {
                    emit(ExReturn);
                    emit(ExNothing);
                }
            } else if (nestType == NestSwitch) {
                if (topNest->allow & AllowCase) {
                    emitChainUpdate(topNest);
                    emit(ExCase);
                    emitWord(MaxWord);
                }
                topNest->setFixup(FixupSwitchEnd, codeTop());
            } else if (nestType == NestIf) {
                if (matchIdentifier(names().elseName)) {
                    emit(ExJump);
                    emitAddressToFixupLater(topNest, FixupIfEnd, Name());
                    emitChainUpdate(topNest);
                    if (matchIdentifier(names().ifName)) {
                        checkAllow("'Else If'", AllowElseIf);
                        emit(ExJumpIfNot);
                        emitAddressToChainLater(topNest);
                        requireSymbol("(", "'Else If'");
                        compileExpr(PropertyBase(CptBool), "'Else If'");
                        requireSymbol(")", "'Else If'");
                        if (!matchSymbol("{")) {
                            compileStatement();
                            popNest(NestIf, "'ElseIf'");
                        }
                    } else {
                        checkAllow("'Else'", AllowElseIf);
                        topNest->allow &= ~AllowElseIf;
                        if (!matchSymbol("{")) {
                            compileStatement();
                            popNest(NestIf, "'Else'");
                        }
                    }
                    return;
                }
                emitChainUpdate(topNest);
                topNest->setFixup(FixupIfEnd, codeTop());
            } else if (nestType == NestFor) {
                topNest->setFixup(FixupForInc, codeTop());
                RetryPoint retry = initRetry();
                performRetry(topNest->forRetry, false, true);
                compileAffector();
                performRetry(retry, false, true);
                emit(ExJump);
                emitAddressToFixupLater(topNest, FixupForStart, Name());
                topNest->setFixup(FixupForEnd, codeTop());
            } else if (nestType == NestForEach) {
                emit(ExIteratorNext);
                topNest->setFixup(FixupIteratorEnd, codeTop());
                emit(ExIteratorPop);
            } else if (nestType == NestLoop) {
                topNest->setFixup(FixupLoopPostCond, codeTop());
                if (matchIdentifier(names().until)) {
                    emit(ExJumpIfNot);
                    emitAddressToFixupLater(topNest, FixupLoopStart, Name());
                    requireSymbol("(", "'Until'");
                    compileExpr(PropertyBase(CptBool), "'Until'");
                    requireSymbol(")", "'Until'");
                    topNest->setFixup(FixupLoopEnd, codeTop());
                } else if (peekIdentifier(names().whileName) && !(topNest->allow & AllowInWhile)) {
                    fail("The loop syntax is do...until, not do...while");
                } else {
                    emit(ExJump);
                    emitAddressToFixupLater(topNest, FixupLoopStart, Name());
                    topNest->setFixup(FixupLoopEnd, codeTop());
                }
            }
            for (const FixupRequest& fixup : topNest->fixupList) {
                if (fixup.type == FixupLabel) {
                    bool found = false;
                    for (const LabelRecord& label : topNest->labelList) {
                        if (label.name == fixup.name) {
                            patchWord(fixup.code, static_cast<uint16_t>(label.code));
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        fail("Label '" + fixup.name.str() + "' not found in this block of code");
                    }
                } else {
                    if (topNest->fixups[fixup.type] == MaxWord) {
                        fail("Internal fixup error " + std::to_string(fixup.type));
                    }
                    patchWord(fixup.code, static_cast<uint16_t>(topNest->fixups[fixup.type]));
                }
            }
        }
        nestType = topNest->nestType;
        nestLevel--;
        topNest--;
        topNode = topNest->node;
        if (nestType == NestFunction || nestType == NestState) {
            topNest->allow &= ~AllowVarDecl;
        }
    }

    int ScriptCompiler::findNest(NestType nestType)
    {
        for (int i = nestLevel - 1; i > 0; i--) {
            if (nest[i].nestType == nestType) {
                return i;
            }
        }
        return -1;
    }

    void ScriptCompiler::compileAffector()
    {
        RetryPoint lowRetry = initRetry();
        gotAffector = false;
        PropertyBase requiredType(CptNone);
        Token resultType;
        if (compileExpr(requiredType, nullptr, &resultType) < 0) {
            Token token;
            getToken(token);
            fail("'" + token.identifier + "': Bad command or expression");
        }
        if (matchSymbol("=")) {
            if (!(resultType.flags & CPF_OutParm)) {
                fail("'=': Left value is not a variable");
            }
            requiredType = resultType;
            requiredType.flags &= ~CPF_OutParm;
            compileExpr(requiredType, "'='");
            RetryPoint highRetry = initRetry();
            emitLet(resultType);
            codeSwitcheroo(lowRetry, highRetry);
        } else if (gotAffector) {
            if (resultType.type == CptString) {
                RetryPoint highRetry = initRetry();
                emit(ExEatString);
                codeSwitcheroo(lowRetry, highRetry);
            }
        } else if (resultType.type != CptNone) {
            Token token;
            getToken(token);
            fail("'" + token.identifier + "': Expression has no effect");
        } else {
            Token token;
            getToken(token);
            fail("'" + token.identifier + "': Bad command or expression");
        }
    }

    bool ScriptCompiler::compileStatement()
    {
        bool needSemicolon = true;
        Token token;
        if (!getToken(token, nullptr, true)) {
            return false;
        }
        if (!compileDeclaration(token, needSemicolon)) {
            if (pass == 0) {
                if (nestLevel < 3) {
                    fail("Unexpected '" + token.identifier + "'");
                }
                ungetToken(token);
                popNest(topNest->nestType, nestTypeName(topNest->nestType));
                skipStatements(1, nestTypeName(topNest->nestType));
                needSemicolon = false;
            } else {
                compileCommand(token, needSemicolon);
            }
        }
        if (needSemicolon) {
            if (!matchSymbol(";")) {
                if (getToken(token)) {
                    fail("Missing ';' before '" + token.identifier + "'");
                }
                fail("Missing ';'");
            }
        }
        return true;
    }

    void ScriptCompiler::compileStatements()
    {
        int originalNestLevel = nestLevel;
        do {
            compileStatement();
        } while (nestLevel > originalNestLevel);
    }

    void ScriptCompiler::postParse(UStruct* node)
    {
        UState* state = cast<UState>(node);
        if (state) {
            for (UField* field : fieldIterator(state)) {
                UFunction* function = cast<UFunction>(field);
                if (!function || !(function->functionFlags & FUNC_Defined)) {
                    continue;
                }
                int32_t probe = vm::probeIndex(function->name);
                if (probe >= 0) {
                    state->probeMask |= 1ull << probe;
                }
            }
        }
        for (UField* child = node->children; child; child = child->next) {
            if (child->outer != node) {
                break;
            }
            if (UStruct* structType = cast<UStruct>(child)) {
                postParse(structType);
            }
        }
    }

    void ScriptCompiler::skipStatements(int subCount, const char* errorTag)
    {
        Token token;
        while (subCount > 0 && getToken(token, nullptr, true)) {
            if (token.matches("{")) {
                subCount++;
            } else if (token.matches("}")) {
                subCount--;
            }
        }
        if (subCount > 0) {
            fail(std::string("Unexpected end of file at end of ") + errorTag);
        }
    }

    void ScriptCompiler::compileCommand(Token& token, bool& needSemicolon)
    {
        const Names& n = names();
        if (token.matches(n.switchName)) {
            checkAllow("'Switch'", AllowCmd);
            pushNest(NestSwitch, Name(), nullptr);
            emit(ExSwitch);
            RetryPoint lowRetry = initRetry();
            compileExpr(PropertyBase(CptNone), "'Switch'", &topNest->switchType);
            if (topNest->switchType.arrayDim != 1) {
                fail("Can't switch on arrays");
            }
            RetryPoint highRetry = initRetry();
            emitSize(topNest->switchType.size(), "'Switch'");
            codeSwitcheroo(lowRetry, highRetry);
            topNest->switchType.flags &= ~CPF_OutParm;
            requireSymbol("{", "'Switch'");
            needSemicolon = false;
        } else if (token.matches(n.caseName)) {
            checkAllow("'Class'", AllowCase);
            emitChainUpdate(topNest);
            emit(ExCase);
            emitAddressToChainLater(topNest);
            PropertyBase switchType = topNest->switchType;
            compileExpr(switchType, "'Case'");
            requireSymbol(":", "'Case'");
            needSemicolon = false;
            topNest->allow |= AllowCmd | AllowLabel | AllowBreak;
        } else if (token.matches(n.defaultName) && (topNest->allow & AllowCase)) {
            checkAllow("'Default'", AllowCase);
            emitChainUpdate(topNest);
            emit(ExCase);
            emitWord(MaxWord);
            requireSymbol(":", "'Default'");
            needSemicolon = false;
            topNest->allow &= ~AllowCase;
            topNest->allow |= AllowCmd | AllowLabel | AllowBreak;
        } else if (token.matches(n.returnName)) {
            checkAllow("'Return'", AllowReturn);
            int i = nestLevel - 1;
            for (; i > 0; i--) {
                if (nest[i].nestType == NestFunction) {
                    break;
                }
                if (nest[i].nestType == NestForEach) {
                    emit(ExIteratorPop);
                }
            }
            if (i <= 0) {
                fail("Internal consistency error on 'Return'");
            }
            UFunction* function = cast<UFunction>(nest[i].node);
            UProperty* returnValue = returnProperty(function);
            emit(ExReturn);
            if (returnValue) {
                PropertyBase property(returnValue);
                property.flags &= ~CPF_OutParm;
                compileExpr(property, "'Return'");
            } else {
                emit(ExNothing);
            }
        } else if (token.matches(n.ifName)) {
            checkAllow("'If'", AllowCmd);
            pushNest(NestIf, Name(), nullptr);
            emit(ExJumpIfNot);
            emitAddressToChainLater(topNest);
            requireSymbol("(", "'If'");
            compileExpr(PropertyBase(CptBool), "'If'");
            requireSymbol(")", "'If'");
            needSemicolon = false;
            if (!matchSymbol("{")) {
                compileStatements();
                popNest(NestIf, "'If'");
            }
        } else if (token.matches(n.whileName)) {
            checkAllow("'While'", AllowCmd);
            pushNest(NestLoop, Name(), nullptr);
            topNest->allow |= AllowInWhile;
            topNest->setFixup(FixupLoopStart, codeTop());
            emit(ExJumpIfNot);
            emitAddressToFixupLater(topNest, FixupLoopEnd, Name());
            requireSymbol("(", "'While'");
            compileExpr(PropertyBase(CptBool), "'While'");
            requireSymbol(")", "'While'");
            needSemicolon = false;
            if (!matchSymbol("{")) {
                compileStatements();
                popNest(NestLoop, "'While'");
            }
        } else if (token.matches(n.doName)) {
            checkAllow("'Do'", AllowCmd);
            pushNest(NestLoop, Name(), nullptr);
            topNest->setFixup(FixupLoopStart, codeTop());
            needSemicolon = false;
            if (!matchSymbol("{")) {
                compileStatements();
                popNest(NestLoop, "'Do'");
            }
        } else if (token.matches(n.breakName)) {
            checkAllow("'Break'", AllowBreak);
            int iNest = findNest(NestLoop);
            iNest = std::max(iNest, findNest(NestFor));
            iNest = std::max(iNest, findNest(NestForEach));
            iNest = std::max(iNest, findNest(NestSwitch));
            emit(ExJump);
            if (nest[iNest].nestType == NestLoop) {
                emitAddressToFixupLater(&nest[iNest], FixupLoopEnd, Name());
            } else if (nest[iNest].nestType == NestFor) {
                emitAddressToFixupLater(&nest[iNest], FixupForEnd, Name());
            } else if (nest[iNest].nestType == NestForEach) {
                emitAddressToFixupLater(&nest[iNest], FixupIteratorEnd, Name());
            } else {
                emitAddressToFixupLater(topNest, FixupSwitchEnd, Name());
            }
        } else if (token.matches(n.continueName)) {
            checkAllow("'Continue'", AllowContinue);
            int iNest = findNest(NestLoop);
            iNest = std::max(iNest, findNest(NestFor));
            iNest = std::max(iNest, findNest(NestForEach));
            if (nest[iNest].nestType == NestLoop) {
                emit(ExJump);
                emitAddressToFixupLater(&nest[iNest], FixupLoopPostCond, Name());
            } else if (nest[iNest].nestType == NestFor) {
                emit(ExJump);
                emitAddressToFixupLater(&nest[iNest], FixupForInc, Name());
            } else if (nest[iNest].nestType == NestForEach) {
                emit(ExIteratorNext);
                emit(ExJump);
                emitAddressToFixupLater(&nest[iNest], FixupIteratorEnd, Name());
            }
        } else if (token.matches(n.forName)) {
            checkAllow("'For'", AllowCmd);
            pushNest(NestFor, Name(), nullptr);
            requireSymbol("(", "'For'");
            compileAffector();
            requireSymbol(";", "'For'");
            topNest->setFixup(FixupForStart, codeTop());
            emit(ExJumpIfNot);
            emitAddressToFixupLater(topNest, FixupForEnd, Name());
            compileExpr(PropertyBase(CptBool), "'For'");
            requireSymbol(";", "'For'");
            topNest->forRetry = initRetry();
            compileAffector();
            performRetry(topNest->forRetry, true, false);
            requireSymbol(")", "'For'");
            needSemicolon = false;
            if (!matchSymbol("{")) {
                compileStatements();
                popNest(NestFor, "'If'");
            }
        } else if (token.matches(n.forEach)) {
            checkAllow("'ForEach'", AllowCmd);
            pushNest(NestForEach, Name(), nullptr);
            emit(ExIterator);
            compileExpr(PropertyBase(CptNone), "'ForEach'");
            if (topNest->allow & AllowIterator) {
                fail("'ForEach': An iterator expression is required");
            }
            emitAddressToFixupLater(topNest, FixupIteratorEnd, Name());
            needSemicolon = false;
            if (!matchSymbol("{")) {
                compileStatements();
                popNest(NestForEach, "'ForEach'");
            }
        } else if (token.matches(n.assert)) {
            checkAllow("'Assert'", AllowCmd);
            uint16_t line = static_cast<uint16_t>(inputLine);
            emit(ExAssert);
            emitWord(line);
            compileExpr(PropertyBase(CptBool), "'Assert'");
        } else if (token.matches(n.gotoName)) {
            checkAllow("'Goto'", AllowLabel);
            if (topNest->allow & AllowStateCmd) {
                emit(ExGotoLabel);
                compileExpr(PropertyBase(CptName), "'Goto'");
            } else {
                int iNest = nestLevel - 1;
                for (; iNest >= 2; iNest--) {
                    if (nest[iNest].nestType == NestState || nest[iNest].nestType == NestFunction || nest[iNest].nestType == NestForEach) {
                        break;
                    }
                }
                if (iNest < 2) {
                    fail("Goto is not allowed here");
                }
                NestInfo* labelNest = &nest[iNest];
                Token label;
                if (!getToken(label)) {
                    fail("Goto: Missing label");
                }
                if (label.tokenName.none()) {
                    label.tokenName = Name(label.identifier);
                }
                if (label.tokenName.none()) {
                    fail("Invalid label '" + label.identifier + "'");
                }
                emit(ExJump);
                emitAddressToFixupLater(labelNest, FixupLabel, label.tokenName);
            }
        } else if (token.matches(n.stop)) {
            checkAllow("'Stop'", AllowStateCmd);
            emit(ExStop);
        } else if (token.matches("}")) {
            if (topNest->nestType == NestClass) {
                fail("Unexpected '}' at class scope");
            } else if (topNest->nestType == NestNone) {
                fail("Unexpected '}' at global scope");
            }
            popNest(NestNone, "'}'");
            needSemicolon = false;
        } else if (token.matches(";")) {
            needSemicolon = false;
        } else if (matchSymbol(":")) {
            checkAllow("Label", AllowLabel);
            if (token.tokenName.none()) {
                token.tokenName = Name(token.identifier);
            }
            if (token.tokenName.none()) {
                fail("Invalid label name '" + token.identifier + "'");
            }
            if (!(topNest->allow & AllowCmd)) {
                topNest->allow |= AllowCmd;
                topNest->allow &= ~(AllowFunction | AllowVarDecl);
            }
            int iNest = nestLevel - 1;
            for (; iNest >= 2; iNest--) {
                if (nest[iNest].nestType == NestState || nest[iNest].nestType == NestFunction || nest[iNest].nestType == NestForEach) {
                    break;
                }
            }
            if (iNest < 2) {
                fail("Labels are not allowed here");
            }
            NestInfo* labelNest = &nest[iNest];
            for (const LabelRecord& label : labelNest->labelList) {
                if (label.name == token.tokenName) {
                    fail("Duplicate label '" + token.tokenName.str() + "'");
                }
            }
            labelNest->labelList.insert(labelNest->labelList.begin(), { token.tokenName, codeTop() });
            needSemicolon = false;
        } else {
            checkAllow("Expression", AllowCmd);
            ungetToken(token);
            compileAffector();
        }
    }

    void ScriptCompiler::compileReplication()
    {
        const Names& n = names();
        inputPos = prevPos = static_cast<int32_t>(compiling->textPos);
        inputLine = prevLine = static_cast<int32_t>(compiling->line);
        while (true) {
            uint32_t propertyFlags = CPF_Net;
            uint32_t functionFlags = FUNC_Net;
            Token token;
            getToken(token);
            if (token.matches("}")) {
                break;
            } else if (token.matches(n.reliable)) {
                functionFlags |= FUNC_NetReliable;
            } else if (!token.matches(n.unreliable)) {
                fail("Missing 'Reliable' or 'Unreliable'");
            }
            requireIdentifier(n.ifName, "Replication statement");
            requireSymbol("(", "Replication condition");
            uint16_t repOffset = static_cast<uint16_t>(codeTop());
            compileExpr(PropertyBase(CptBool), "Replication condition");
            requireSymbol(")", "Replication condition");
            do {
                Token varToken;
                if (!getIdentifier(varToken)) {
                    fail("Missing variable name in replication definition");
                }
                Name varName = Name::find(varToken.identifier);
                if (varName.none()) {
                    fail("Unrecognized variable '" + varToken.identifier + "' name in replication definition");
                }
                bool found = false;
                for (UField* child = compiling->children; child && !found; child = child->next) {
                    UProperty* property = cast<UProperty>(child);
                    if (!property || property->name != varName) {
                        continue;
                    }
                    if (property->propertyFlags & CPF_Net) {
                        fail("Variable '" + varName.str() + "' already has a replication definition");
                    }
                    property->propertyFlags |= propertyFlags;
                    property->replicationOffset = repOffset;
                    found = true;
                }
                for (UField* child = compiling->children; child && !found; child = child->next) {
                    UFunction* function = cast<UFunction>(child);
                    if (!function || function->name != varName) {
                        continue;
                    }
                    if (cast<UFunction>(function->superField)) {
                        fail("Function '" + varName.str() + "' is defined in base class '" + ownerClass(function->superField)->name.str() + "'");
                    }
                    if (function->functionFlags & FUNC_Net) {
                        fail("Function '" + varName.str() + "' already has a replication definition");
                    }
                    if ((function->functionFlags & FUNC_Native) && (function->functionFlags & FUNC_Final)) {
                        fail("Native final functions may not be replicated");
                    }
                    function->functionFlags |= functionFlags;
                    function->replicationOffset = repOffset;
                    found = true;
                }
                if (!found) {
                    fail("Bad variable or function '" + varName.str() + "' in replication definition");
                }
            } while (matchSymbol(","));
            requireSymbol(";", "Replication definition");
        }
    }

    void ScriptCompiler::compileSecondPass(UStruct* node)
    {
        int startNestLevel = nestLevel;
        NestType newNest = NestNone;
        if (cast<UFunction>(node)) {
            newNest = NestFunction;
        } else if (node == compiling) {
            newNest = NestClass;
        } else if (cast<UState>(node)) {
            newNest = NestState;
        }
        pushNest(newNest, node->name, node);
        topNode->script.clear();
        UFunction* topFunction = cast<UFunction>(topNode);
        UFunction* superFunction = topFunction ? cast<UFunction>(topFunction->superField) : nullptr;
        if (topFunction && superFunction) {
            topFunction->functionFlags &= ~FUNC_NetFuncFlags;
            topFunction->functionFlags |= (superFunction->functionFlags & FUNC_NetFuncFlags);
        }
        if (node == compiling && node->line != 0xffffffffu) {
            compileReplication();
        }
        std::vector<UStruct*> children;
        for (UField* child = node->children; child; child = child->next) {
            if (child->outer != node) {
                break;
            }
            if (UStruct* structType = cast<UStruct>(child)) {
                if (!isPlainStruct(structType)) {
                    children.push_back(structType);
                }
            }
        }
        for (UStruct* child : children) {
            compileSecondPass(child);
        }
        bool doCompile = false;
        if (node->line != 0xffffffffu && node != compiling) {
            inputPos = prevPos = static_cast<int32_t>(node->textPos);
            prevLine = static_cast<int32_t>(node->line);
            inputLine = static_cast<int32_t>(node->line);
            UState* topState = cast<UState>(topNode);
            if (topFunction && !(topFunction->functionFlags & FUNC_Native)) {
                doCompile = (topFunction->functionFlags & FUNC_Defined) != 0;
            } else if (topFunction) {
                for (UField* field : fieldIterator(topFunction)) {
                    UProperty* property = cast<UProperty>(field);
                    if (!property || (property->propertyFlags & (CPF_Parm | CPF_ReturnParm)) != CPF_Parm) {
                        break;
                    }
                    emit(ExNativeParm);
                    emitObject(property);
                }
            } else if (topState) {
                doCompile = true;
            }
        }
        if (doCompile) {
            while (nestLevel > startNestLevel) {
                if (!compileStatement()) {
                    fail("Unexpected end of code in " + node->cls->name.str());
                }
            }
        } else if (node != compiling) {
            popNest(newNest, node->cls->name.str());
        }
    }

    void ScriptCompiler::initReplication(UStruct* node)
    {
        if (UFunction* function = cast<UFunction>(node)) {
            function->functionFlags &= ~FUNC_NetFuncFlags;
            function->replicationOffset = MaxWord;
        }
        for (UField* child = node->children; child; child = child->next) {
            if (child->outer != node) {
                break;
            }
            if (UStruct* structType = cast<UStruct>(child)) {
                initReplication(structType);
            }
        }
    }

    bool ScriptCompiler::compileScript(UClass* cls, int compilePass)
    {
        compiling = cls;
        pass = compilePass;
        context.log(std::string(pass ? "Compiling " : "Parsing ") + cls->name.str());
        if (pass == 0) {
            cls->script.clear();
            cls->dependencies.clear();
            cls->classFlags &= ~CLASS_RecompilerClear;
            if (UClass* parent = cls->superClass()) {
                cls->classFlags |= parent->classFlags & CLASS_Inherit;
                cls->configName = parent->configName;
                if (!cls->within) {
                    cls->within = parent->within;
                }
            }
        } else {
            cls->script.clear();
            for (UField* child = cls->children; child; child = child->next) {
                if (child->outer != cls) {
                    break;
                }
                if (UProperty* property = cast<UProperty>(child)) {
                    property->propertyFlags &= ~CPF_Net;
                    property->replicationOffset = MaxWord;
                }
            }
            initReplication(cls);
        }
        sourceText = cls->scriptText->text;
        input = sourceText.c_str();
        inputLength = static_cast<int32_t>(sourceText.size());
        inputPos = 0;
        prevPos = 0;
        prevLine = 1;
        inputLine = 1;
        nestLevel = 0;
        topNode = nullptr;
        topNest = nullptr;
        try {
            pushNest(NestNone, Name(), nullptr);
            if (pass == 0) {
                while (compileStatement()) {
                }
                cls->script.clear();
                cls->classFlags |= CLASS_Parsed;
            } else {
                compileSecondPass(cls);
                cls->classFlags |= CLASS_Compiled;
            }
            if (nestLevel == 0) {
                fail("Internal nest inconsistency");
            } else if (nestLevel == 1) {
                fail("Missing 'Class' definition");
            } else if (nestLevel == 2) {
                popNest(NestClass, "'Class'");
            } else {
                fail(std::string("Unexpected end of script in '") + nestTypeName(topNest->nestType) + "' block");
            }
            if (pass == 0) {
                postParse(cls);
                for (UField* child = cls->children; child; child = child->next) {
                    UProperty* property = cast<UProperty>(child);
                    if (!property || property->outer != cls) {
                        continue;
                    }
                    if (property->propertyFlags & CPF_Localized) {
                        cls->classFlags |= CLASS_Localized;
                    }
                    if (property->propertyFlags & CPF_Config) {
                        cls->classFlags |= CLASS_Config;
                    }
                }
            }
            return true;
        } catch (const std::exception& error) {
            std::string file = context.files.count(cls) ? context.files[cls].string() : cls->name.str() + ".uc";
            context.log(file + "(" + std::to_string(inputLine) + ") : Error, " + error.what());
            context.errorCount++;
            cls->classFlags &= ~(CLASS_Parsed | CLASS_Compiled);
            cls->script.clear();
            cls->dependencies.clear();
            return false;
        }
    }
}
