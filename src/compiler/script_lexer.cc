#include "compiler/script_parser.h"

#include "core/objects.h"
#include "core/vm.h"
#include "engine/package.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace dx::compiler::detail {
    std::vector<uint8_t>& ScriptCompiler::code()
    {
        return topNode->script;
    }

    int32_t ScriptCompiler::codeTop()
    {
        return static_cast<int32_t>(topNode->script.size());
    }

    void ScriptCompiler::emit(uint8_t value)
    {
        code().push_back(value);
    }

    void ScriptCompiler::emitWord(uint16_t value)
    {
        emit(static_cast<uint8_t>(value & 0xff));
        emit(static_cast<uint8_t>(value >> 8));
    }

    void ScriptCompiler::emitDword(uint32_t value)
    {
        for (int i = 0; i < 4; i++) {
            emit(static_cast<uint8_t>((value >> (8 * i)) & 0xff));
        }
    }

    void ScriptCompiler::emitFloat(float value)
    {
        uint32_t bits = 0;
        std::memcpy(&bits, &value, 4);
        emitDword(bits);
    }

    void ScriptCompiler::emitObject(UObject* object)
    {
        emitDword(object ? object->handle : 0);
    }

    void ScriptCompiler::emitName(Name name)
    {
        emitDword(name.index());
    }

    void ScriptCompiler::patchWord(int32_t at, uint16_t value)
    {
        code()[static_cast<size_t>(at)] = static_cast<uint8_t>(value & 0xff);
        code()[static_cast<size_t>(at) + 1] = static_cast<uint8_t>(value >> 8);
    }

    char ScriptCompiler::getChar(bool literal)
    {
        int commentCount = 0;
        prevPos = inputPos;
        prevLine = inputLine;
        while (true) {
            char c = inputPos < inputLength ? input[inputPos] : 0;
            inputPos++;
            if (c == 0x0a) {
                inputLine++;
            } else if (!literal && c == '/' && peekAt(inputPos) == '*') {
                commentCount++;
                inputPos++;
                continue;
            } else if (!literal && c == '*' && peekAt(inputPos) == '/') {
                if (--commentCount < 0) {
                    fail("Unexpected '*/' outside of comment");
                }
                inputPos++;
                continue;
            }
            if (commentCount > 0) {
                if (c == 0) {
                    fail("End of script encountered inside comment");
                }
                continue;
            }
            return c;
        }
    }

    char ScriptCompiler::peekAt(int32_t position) const
    {
        return position >= 0 && position < inputLength ? input[position] : 0;
    }

    void ScriptCompiler::ungetChar()
    {
        inputPos = prevPos;
        inputLine = prevLine;
    }

    char ScriptCompiler::peekChar() const
    {
        return inputPos < inputLength ? input[inputPos] : 0;
    }

    char ScriptCompiler::getLeadingChar()
    {
        char c;
        while (true) {
            do {
                c = getChar();
            } while (c == 0x20 || c == 0x09 || c == 0x0d || c == 0x0a);
            if (c == '/' && peekAt(inputPos) == '/') {
                do {
                    c = getChar(true);
                } while (c != 0x0d && c != 0x0a && c != 0x00);
                continue;
            }
            return c;
        }
    }

    bool ScriptCompiler::isEol(char c)
    {
        return c == 0x0d || c == 0x0a || c == 0;
    }

    void ScriptCompiler::emitConstant(Token& token)
    {
        switch (token.type) {
        case CptInt:
            if (token.intValue == 0) {
                emit(ExIntZero);
            } else if (token.intValue == 1) {
                emit(ExIntOne);
            } else if (token.intValue >= 0 && token.intValue <= 255) {
                emit(ExIntConstByte);
                emit(static_cast<uint8_t>(token.intValue));
            } else {
                emit(ExIntConst);
                emitDword(static_cast<uint32_t>(token.intValue));
            }
            break;
        case CptByte:
            emit(ExByteConst);
            emit(token.byteValue);
            break;
        case CptBool:
            emit(token.boolValue ? ExTrue : ExFalse);
            break;
        case CptFloat:
            emit(ExFloatConst);
            emitFloat(token.floatValue);
            break;
        case CptString:
            if (isAnsi(token.stringValue)) {
                emit(ExStringConst);
                for (char c : token.stringValue) {
                    emit(static_cast<uint8_t>(c));
                }
                emit(0);
            } else {
                emit(ExUnicodeStringConst);
                for (char c : token.stringValue) {
                    emitWord(static_cast<uint8_t>(c));
                }
                emitWord(0);
            }
            break;
        case CptObject:
            if (token.object == nullptr) {
                emit(ExNoObject);
            } else {
                UClass* actor = lookup.findClass("Actor");
                if (actor && token.propertyClass()->isChildOf(actor)) {
                    fail("Illegal actor constant");
                }
                emit(ExObjectConst);
                emitObject(token.object);
            }
            break;
        case CptName:
            emit(ExNameConst);
            emitName(token.nameValue);
            break;
        case CptStruct:
            if (token.isVector()) {
                emit(ExVectorConst);
                for (float value : token.vectorValue) {
                    emitFloat(value);
                }
            } else if (token.isRotator()) {
                emit(ExRotationConst);
                for (int32_t value : token.rotatorValue) {
                    emitDword(static_cast<uint32_t>(value));
                }
            } else {
                fail("Not yet implemented");
            }
            break;
        default:
            fail("Internal EmitConstant token type error " + std::to_string(token.type));
        }
    }

    void ScriptCompiler::emitStackNodeLinkFunction(UFunction* node, bool forceFinal, bool global)
    {
        bool isFinal = (node->functionFlags & FUNC_Final) || forceFinal;
        if (isFinal && node->nativeIndex && node->nativeIndex < 256) {
            emit(static_cast<uint8_t>(node->nativeIndex));
        } else if (isFinal && node->nativeIndex) {
            emit(static_cast<uint8_t>(ExExtendedNative + node->nativeIndex / 256));
            emit(static_cast<uint8_t>(node->nativeIndex % 256));
        } else if (isFinal) {
            emit(ExFinalFunction);
            emitObject(node);
        } else if (global) {
            emit(ExGlobalFunction);
            emitName(node->name);
        } else {
            emit(ExVirtualFunction);
            emitName(node->name);
        }
    }

    void ScriptCompiler::emitAddressToFixupLater(NestInfo* target, FixupType type, Name name)
    {
        target->fixupList.insert(target->fixupList.begin(), { type, codeTop(), name });
        emitWord(0);
    }

    void ScriptCompiler::emitAddressToChainLater(NestInfo* target)
    {
        target->codeChain = codeTop();
        emitWord(0);
    }

    void ScriptCompiler::emitChainUpdate(NestInfo* target)
    {
        if (target->codeChain != IndexNone) {
            patchWord(target->codeChain, static_cast<uint16_t>(codeTop()));
            target->codeChain = IndexNone;
        }
    }

    void ScriptCompiler::emitSize(int32_t size, const char* tag)
    {
        uint8_t value = static_cast<uint8_t>(size);
        if (value != size) {
            fail(std::string(tag) + ": Variable is too large (" + std::to_string(size) + " bytes, 255 max)");
        }
        emit(value);
    }

    void ScriptCompiler::emitLet(const PropertyBase& type)
    {
        if (type.flags & CPF_Const) {
            fail("Can't assign Const variables");
        }
        if (type.arrayDim != 1) {
            fail("Can only assign individual booleans, not boolean arrays");
        }
        emit(type.type == CptBool ? ExLetBool : ExLet);
    }

    bool ScriptCompiler::getToken(Token& token, const PropertyBase* hint, bool noConsts)
    {
        token.tokenName = Name();
        char c = getLeadingChar();
        char p = peekChar();
        if (c == 0) {
            ungetChar();
            return false;
        }
        token.startPos = prevPos;
        token.startLine = prevLine;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') {
            std::string text;
            do {
                text += c;
                if (static_cast<int>(text.size()) > NameSize) {
                    fail("Identifer length exceeds maximum of " + std::to_string(NameSize));
                }
                c = getChar();
            } while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_');
            ungetChar();
            token.identifier = text;
            token.kind = TokenIdentifier;
            token.tokenName = Name::find(text);
            const Names& n = names();
            if (token.tokenName == n.vect && !noConsts && matchSymbol("(")) {
                float value[3] = {};
                if (!getConstFloat(value[0])) {
                    fail("Missing X component of vector");
                }
                if (!matchSymbol(",")) {
                    fail("Missing ',' in vector");
                }
                if (!getConstFloat(value[1])) {
                    fail("Missing Y component of vector");
                }
                if (!matchSymbol(",")) {
                    fail("Missing ',' in vector");
                }
                if (!getConstFloat(value[2])) {
                    fail("Missing Z component of vector");
                }
                if (!matchSymbol(")")) {
                    fail("Missing ')' in vector");
                }
                token.setConstVector(lookup, value);
                return true;
            }
            if (token.tokenName == n.rot && !noConsts && matchSymbol("(")) {
                int32_t value[3] = {};
                if (!getConstInt(value[0])) {
                    fail("Missing Pitch component of rotation");
                }
                if (!matchSymbol(",")) {
                    fail("Missing ',' in rotation");
                }
                if (!getConstInt(value[1])) {
                    fail("Missing Yaw component of rotation");
                }
                if (!matchSymbol(",")) {
                    fail("Missing ',' in rotation");
                }
                if (!getConstInt(value[2])) {
                    fail("Missing Roll component of rotation");
                }
                if (!matchSymbol(")")) {
                    fail("Missing ')' in rotation");
                }
                token.setConstRotation(lookup, value);
                return true;
            }
            if (token.tokenName == n.trueName && !noConsts) {
                token.setConstBool(true);
                return true;
            }
            if (token.tokenName == n.falseName && !noConsts) {
                token.setConstBool(false);
                return true;
            }
            if (token.tokenName == n.arrayCount && !noConsts) {
                Token typeToken;
                requireSizeOfParm(typeToken, "'ArrayCount'");
                if (typeToken.arrayDim > 1) {
                    token.setConstInt(typeToken.arrayDim);
                } else {
                    fail("ArrayCount argument is not an array");
                }
                return true;
            }
            if (token.matches("None") && !noConsts) {
                token.setConstObject(nullptr);
                return true;
            }
            if (hint && hint->type == CptByte && hint->enumType() && !token.tokenName.none() && !noConsts) {
                const std::vector<Name>& entries = hint->enumType()->names;
                auto it = std::find(entries.begin(), entries.end(), token.tokenName);
                if (it != entries.end()) {
                    token.setConstByte(hint->enumType(), static_cast<uint8_t>(it - entries.begin()));
                    return true;
                }
            }
            if (!noConsts && peekSymbol("'")) {
                UClass* type = lookup.findClass(token.identifier);
                UClass* actor = lookup.findClass("Actor");
                if (type && !(actor && type->isChildOf(actor)) && !noConsts && matchSymbol("'")) {
                    std::string path;
                    do {
                        Token nameToken;
                        if (!getIdentifier(nameToken, true)) {
                            fail("Missing " + type->name.str() + " name");
                        }
                        if (!path.empty()) {
                            path += ".";
                        }
                        path += nameToken.identifier;
                    } while (matchSymbol("."));
                    UObject* object = lookup.findObject(type, path);
                    if (!object) {
                        fail("Can't find " + type->name.str() + " '" + path + "'");
                    }
                    if (!matchSymbol("'")) {
                        fail("Missing single quote after " + type->name.str() + " name");
                    }
                    token.setConstObject(object);
                    return true;
                }
            }
            return true;
        }
        if ((c >= '0' && c <= '9') || (((c == '+' || c == '-') && (p >= '0' && p <= '9')) && !noConsts)) {
            bool isFloat = false;
            bool isHex = false;
            std::string text;
            do {
                if (c == '.') {
                    isFloat = true;
                }
                if (c == 'X') {
                    isHex = true;
                }
                text += c;
                if (static_cast<int>(text.size()) >= NameSize) {
                    fail("Number length exceeds maximum of " + std::to_string(NameSize));
                }
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(getChar())));
            } while ((c >= '0' && c <= '9') || c == '.' || c == 'X' || (c >= 'A' && c <= 'F'));
            token.identifier = text;
            ungetChar();
            if (isFloat) {
                token.setConstFloat(static_cast<float>(std::atof(text.c_str())));
            } else if (isHex) {
                token.setConstInt(static_cast<int32_t>(std::strtol(text.c_str(), nullptr, 0)));
            } else {
                token.setConstInt(std::atoi(text.c_str()));
            }
            return true;
        }
        if (c == '\'' && !noConsts) {
            std::string text;
            c = getChar();
            while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == ' ') {
                text += c;
                if (static_cast<int>(text.size()) > NameSize) {
                    fail("Name length exceeds maximum of " + std::to_string(NameSize));
                }
                c = getChar();
            }
            if (c != '\'') {
                fail("Illegal character in name");
            }
            token.identifier = text;
            token.setConstName(Name(text));
            return true;
        }
        if (c == '"') {
            std::string text;
            c = getChar(true);
            while (c != '"' && !isEol(c)) {
                if (c == '\\') {
                    c = getChar(true);
                    if (isEol(c)) {
                        break;
                    }
                }
                text += c;
                if (static_cast<int>(text.size()) >= MaxStringConstSize) {
                    fail("String constant exceeds maximum of " + std::to_string(MaxStringConstSize) + " characters");
                }
                c = getChar(true);
            }
            if (c != '"') {
                fail("Unterminated string constant");
            }
            token.setConstString(text);
            return true;
        }
        std::string text(1, c);
        char d = getChar();
        auto pair = [&](char first, char second) {
            return c == first && d == second;
        };
        if (pair('<', '<') || pair('>', '>') || pair('!', '=') || pair('<', '=') || pair('>', '=') || pair('+', '+') || pair('-', '-') || pair('+', '=') || pair('-', '=') || pair('*', '=') || pair('/', '=') || pair('&', '&') || pair('|', '|') || pair('^', '^') || pair('=', '=') || pair('*', '*') || pair('~', '=')) {
            text += d;
            if (c == '>' && d == '>') {
                if (getChar() == '>') {
                    text += '>';
                } else {
                    ungetChar();
                }
            }
        } else {
            ungetChar();
        }
        token.identifier = text;
        token.kind = TokenSymbol;
        token.tokenName = Name::find(text);
        return true;
    }

    bool ScriptCompiler::getRawToken(Token& token)
    {
        std::string text;
        char c = getLeadingChar();
        while (!isEol(c)) {
            if ((c == '/' && peekChar() == '/') || (c == '/' && peekChar() == '*')) {
                break;
            }
            text += c;
            if (static_cast<int>(text.size()) >= MaxStringConstSize) {
                fail("Identifier exceeds maximum of " + std::to_string(NameSize) + " characters");
            }
            c = getChar(true);
        }
        ungetChar();
        while (!text.empty() && (text.back() == ' ' || text.back() == 9)) {
            text.pop_back();
        }
        token.setConstString(text);
        return !text.empty();
    }

    bool ScriptCompiler::getIdentifier(Token& token, bool noConsts)
    {
        if (!getToken(token, nullptr, noConsts)) {
            return false;
        }
        if (token.kind == TokenIdentifier) {
            return true;
        }
        ungetToken(token);
        return false;
    }

    bool ScriptCompiler::getSymbol(Token& token)
    {
        if (!getToken(token)) {
            return false;
        }
        if (token.kind == TokenSymbol) {
            return true;
        }
        ungetToken(token);
        return false;
    }

    bool ScriptCompiler::getConstInt(int32_t& result, const char* tag)
    {
        Token token;
        if (getToken(token)) {
            if (token.getConstInt(result)) {
                return true;
            }
            ungetToken(token);
        }
        if (tag) {
            fail(std::string(tag) + ": Missing constant integer");
        }
        return false;
    }

    bool ScriptCompiler::getConstFloat(float& result, const char* tag)
    {
        Token token;
        if (getToken(token)) {
            if (token.getConstFloat(result)) {
                return true;
            }
            ungetToken(token);
        }
        if (tag) {
            fail(std::string(tag) + ": Missing constant integer");
        }
        return false;
    }

    bool ScriptCompiler::matchIdentifier(Name match)
    {
        Token token;
        if (!getToken(token)) {
            return false;
        }
        if (token.kind == TokenIdentifier && token.tokenName == match) {
            return true;
        }
        ungetToken(token);
        return false;
    }

    bool ScriptCompiler::matchSymbol(const char* match)
    {
        Token token;
        if (!getToken(token, nullptr, true)) {
            return false;
        }
        if (token.kind == TokenSymbol && iequals(token.identifier, match)) {
            return true;
        }
        ungetToken(token);
        return false;
    }

    bool ScriptCompiler::peekSymbol(const char* match)
    {
        Token token;
        if (!getToken(token, nullptr, true)) {
            return false;
        }
        ungetToken(token);
        return token.kind == TokenSymbol && iequals(token.identifier, match);
    }

    bool ScriptCompiler::peekIdentifier(Name match)
    {
        Token token;
        if (!getToken(token, nullptr, true)) {
            return false;
        }
        ungetToken(token);
        return token.kind == TokenIdentifier && token.tokenName == match;
    }

    void ScriptCompiler::ungetToken(const Token& token)
    {
        inputPos = token.startPos;
        inputLine = token.startLine;
    }

    void ScriptCompiler::requireSymbol(const char* match, const std::string& tag)
    {
        if (!matchSymbol(match)) {
            fail(std::string("Missing '") + match + "' in " + tag);
        }
    }

    void ScriptCompiler::requireIdentifier(Name match, const std::string& tag)
    {
        if (!matchIdentifier(match)) {
            fail("Missing '" + match.str() + "' in " + tag);
        }
    }

    RetryPoint ScriptCompiler::initRetry()
    {
        RetryPoint retry;
        retry.input = input;
        retry.inputLength = inputLength;
        retry.inputPos = inputPos;
        retry.inputLine = inputLine;
        retry.codeTop = codeTop();
        return retry;
    }

    void ScriptCompiler::performRetry(const RetryPoint& retry, bool binary, bool text)
    {
        if (text) {
            input = retry.input;
            inputLength = retry.inputLength;
            inputPos = retry.inputPos;
            inputLine = retry.inputLine;
        }
        if (binary) {
            code().resize(static_cast<size_t>(retry.codeTop));
        }
    }

    void ScriptCompiler::codeSwitcheroo(const RetryPoint& low, const RetryPoint& high)
    {
        std::vector<uint8_t>& script = code();
        std::rotate(script.begin() + low.codeTop, script.begin() + high.codeTop, script.end());
    }
}
