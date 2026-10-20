#include "compiler/script_package.h"

#include "core/objects.h"
#include "core/package_writer.h"

#include <cstring>
#include <stdexcept>

namespace dx::compiler {
    namespace {
        constexpr uint32_t SavedFlags = 0x067F0025;
        constexpr uint32_t LoadForClient = 0x00010000;
        constexpr uint32_t LoadForServer = 0x00020000;
        constexpr uint32_t LoadForEdit = 0x00040000;

        class ScriptWriter {
        public:
            ScriptWriter(PackageWriter& owner, Writer& target, const std::vector<uint8_t>& script)
                : package(owner)
                , out(target)
                , code(script)
            {
            }

            void run()
            {
                while (at < code.size()) {
                    expr();
                }
            }

        private:
            uint8_t u8()
            {
                require(1);
                uint8_t value = code[at++];
                out.u8(value);
                return value;
            }

            uint16_t u16()
            {
                require(2);
                uint16_t value = 0;
                std::memcpy(&value, code.data() + at, 2);
                at += 2;
                out.i16(static_cast<int16_t>(value));
                return value;
            }

            uint32_t raw32()
            {
                require(4);
                uint32_t value = 0;
                std::memcpy(&value, code.data() + at, 4);
                at += 4;
                return value;
            }

            void u32()
            {
                out.u32(raw32());
            }

            void object()
            {
                package.writeObject(out, objects::fromHandle(raw32()));
            }

            void name()
            {
                package.writeName(out, Name::fromIndex(raw32()));
            }

            void require(size_t count) const
            {
                if (at + count > code.size()) {
                    throw std::runtime_error("bytecode overrun");
                }
            }

            void parms()
            {
                while (expr() != 0x16) {
                }
            }

            uint8_t expr()
            {
                uint8_t token = u8();
                if (token >= 0x39 && token < 0x60) {
                    expr();
                    return token;
                }
                if (token >= 0x70) {
                    parms();
                    return token;
                }
                if (token >= 0x60) {
                    u8();
                    parms();
                    return token;
                }
                switch (token) {
                case 0x0f:
                case 0x14:
                case 0x10:
                case 0x1a:
                    expr();
                    expr();
                    break;
                case 0x06:
                    u16();
                    break;
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x29:
                case 0x20:
                    object();
                    break;
                case 0x2d:
                case 0x0b:
                case 0x16:
                case 0x25:
                case 0x26:
                case 0x27:
                case 0x28:
                case 0x2a:
                case 0x17:
                case 0x30:
                case 0x08:
                case 0x31:
                    break;
                case 0x0e:
                case 0x04:
                case 0x0d:
                    expr();
                    break;
                case 0x1c:
                    object();
                    parms();
                    break;
                case 0x1b:
                case 0x38:
                    name();
                    parms();
                    break;
                case 0x12:
                case 0x19:
                    expr();
                    u16();
                    u8();
                    expr();
                    break;
                case 0x11:
                    for (int i = 0; i < 4; i++) {
                        expr();
                    }
                    break;
                case 0x1d:
                case 0x1e:
                    u32();
                    break;
                case 0x1f:
                    while (u8() != 0) {
                    }
                    break;
                case 0x34:
                    while (u16() != 0) {
                    }
                    break;
                case 0x21:
                    name();
                    break;
                case 0x22:
                case 0x23:
                    u32();
                    u32();
                    u32();
                    break;
                case 0x24:
                case 0x2c:
                    u8();
                    break;
                case 0x13:
                case 0x2e:
                case 0x36:
                    object();
                    expr();
                    break;
                case 0x07:
                case 0x09:
                case 0x18:
                    u16();
                    expr();
                    break;
                case 0x2f:
                    expr();
                    u16();
                    break;
                case 0x05:
                    u8();
                    expr();
                    break;
                case 0x0a:
                    if (u16() != 0xffff) {
                        expr();
                    }
                    break;
                case 0x0c:
                    while (true) {
                        uint32_t label = raw32();
                        package.writeName(out, Name::fromIndex(label));
                        u32();
                        if (Name::fromIndex(label).none()) {
                            break;
                        }
                    }
                    break;
                case 0x32:
                case 0x33:
                    object();
                    expr();
                    expr();
                    break;
                default:
                    throw std::runtime_error("unknown bytecode token " + std::to_string(token));
                }
                return token;
            }

            PackageWriter& package;
            Writer& out;
            const std::vector<uint8_t>& code;
            size_t at = 0;
        };

        void writeField(PackageWriter& package, Writer& out, UField* field)
        {
            package.writeObject(out, field->superField);
            package.writeObject(out, field->next);
        }

        void writeStruct(PackageWriter& package, Writer& out, UStruct* structType)
        {
            writeField(package, out, structType);
            package.writeObject(out, structType->scriptText);
            package.writeObject(out, structType->children);
            package.writeName(out, structType->friendlyName);
            out.u32(structType->line);
            out.u32(structType->textPos);
            out.u32(static_cast<uint32_t>(structType->script.size()));
            ScriptWriter(package, out, structType->script).run();
        }

        void writeStateData(Writer& out, UState* state)
        {
            out.u64(state->probeMask);
            out.u64(state->ignoreMask);
            out.i16(static_cast<int16_t>(state->labelTableOffset));
            out.u32(state->stateFlags);
        }

        void writeProperty(PackageWriter& package, Writer& out, UProperty* property)
        {
            writeField(package, out, property);
            out.i32(property->arrayDim);
            out.u32(property->propertyFlags);
            package.writeName(out, property->category);
            if (property->propertyFlags & CPF_Net) {
                out.i16(static_cast<int16_t>(property->replicationOffset));
            }
            switch (property->type) {
            case PropType::Byte:
                package.writeObject(out, property->enumType);
                break;
            case PropType::Object:
                package.writeObject(out, property->objectClass);
                break;
            case PropType::Class:
                package.writeObject(out, property->objectClass);
                package.writeObject(out, property->metaClass);
                break;
            case PropType::Struct:
                package.writeObject(out, property->structType);
                break;
            case PropType::Array:
                package.writeObject(out, property->inner);
                break;
            case PropType::FixedArray:
                package.writeObject(out, property->inner);
                out.i32(property->fixedCount);
                break;
            case PropType::Map:
                package.writeObject(out, property->inner);
                package.writeObject(out, property->mapValue);
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

        void serialize(PackageWriter& package, UObject* object, Writer& out)
        {
            if (UClass* cls = cast<UClass>(object)) {
                writeStruct(package, out, cls);
                writeStateData(out, cls);
                out.u32(cls->classFlags);
                for (uint32_t part : cls->guid) {
                    out.u32(part);
                }
                out.index(static_cast<int32_t>(cls->dependencies.size()));
                for (const ClassDependency& dependency : cls->dependencies) {
                    package.writeObject(out, dependency.cls);
                    out.u32(dependency.deep);
                    out.u32(dependency.scriptTextCrc);
                }
                out.index(static_cast<int32_t>(cls->packageImports.size()));
                for (Name imported : cls->packageImports) {
                    package.writeName(out, imported);
                }
                package.writeObject(out, cls->within);
                package.writeName(out, cls->configName);
                UClass* parent = cls->superClass();
                package.writeTaggedProperties(out, cls, cls->defaultObject->data, parent ? parent->defaultObject->data : nullptr, parent ? parent->size : 0);
            } else if (UFunction* function = cast<UFunction>(object)) {
                writeStruct(package, out, function);
                out.i16(static_cast<int16_t>(function->nativeIndex));
                out.u8(function->precedence);
                out.u32(function->functionFlags);
                if (function->functionFlags & FUNC_Net) {
                    out.i16(static_cast<int16_t>(function->replicationOffset));
                }
            } else if (UState* state = cast<UState>(object)) {
                writeStruct(package, out, state);
                writeStateData(out, state);
            } else if (UStruct* structType = cast<UStruct>(object)) {
                writeStruct(package, out, structType);
            } else if (UEnum* enumType = cast<UEnum>(object)) {
                writeField(package, out, enumType);
                out.index(static_cast<int32_t>(enumType->names.size()));
                for (Name entry : enumType->names) {
                    package.writeName(out, entry);
                }
            } else if (UConst* constant = cast<UConst>(object)) {
                writeField(package, out, constant);
                out.string(constant->value);
            } else if (UTextBuffer* text = cast<UTextBuffer>(object)) {
                out.u32(0);
                out.u32(0);
                out.string(text->text);
            } else if (UProperty* property = cast<UProperty>(object)) {
                writeProperty(package, out, property);
            }
        }
    }

    std::vector<UObject*> packageContents(UObject* package)
    {
        std::vector<UObject*> contents;
        for (UObject* object : objects::all()) {
            if (!object || object == package || object->destroyed || (object->flags & RF_Transient)) {
                continue;
            }
            if ((object->cls && object->cls->defaultObject == object) || objects::isIntrinsic(object)) {
                continue;
            }
            if (object->outermost() == package) {
                contents.push_back(object);
            }
        }
        return contents;
    }

    bool saveScriptPackage(UObject* package, const std::filesystem::path& path, std::string* error)
    {
        PackageWriter writer(package->name);
        for (UObject* object : packageContents(package)) {
            writer.addExport(object, object->outer == package ? nullptr : object->outer);
        }
        writer.setNativeSerializer(serialize);
        writer.setFlagSelector([](UObject* object) {
            uint32_t flags = object->flags & SavedFlags;
            if (!(flags & RF_NotForClient)) {
                flags |= LoadForClient;
            }
            if (!(flags & RF_NotForServer)) {
                flags |= LoadForServer;
            }
            if (!(flags & RF_NotForEdit)) {
                flags |= LoadForEdit;
            }
            return flags;
        });
        return writer.save(path, error);
    }
}
