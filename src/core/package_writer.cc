#include "core/package_writer.h"

#include "core/objects.h"
#include "core/vm.h"

#include <cstring>
#include <fstream>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace dx {
    namespace {
        constexpr uint32_t PackageTag = 0x9E2A83C1;
        constexpr uint16_t PackageVersion = 68;
        constexpr uint32_t PackageFlagsAllowDownload = 0x0001;
        constexpr uint32_t NameFlags = 0x00070010;
        constexpr uint32_t SavedFlagMask = RF_Public | RF_Standalone | RF_Transactional;

        enum TagType : uint8_t {
            TagByte = 1,
            TagInt = 2,
            TagBool = 3,
            TagFloat = 4,
            TagObject = 5,
            TagName = 6,
            TagClass = 8,
            TagStruct = 10,
            TagStr = 13,
        };

        bool tagTypeOf(PropType type, uint8_t& tag)
        {
            switch (type) {
            case PropType::Byte:
                tag = TagByte;
                return true;
            case PropType::Int:
                tag = TagInt;
                return true;
            case PropType::Bool:
                tag = TagBool;
                return true;
            case PropType::Float:
                tag = TagFloat;
                return true;
            case PropType::Object:
                tag = TagObject;
                return true;
            case PropType::Class:
                tag = TagObject;
                return true;
            case PropType::Name:
                tag = TagName;
                return true;
            case PropType::Str:
            case PropType::String:
                tag = TagStr;
                return true;
            case PropType::Struct:
                tag = TagStruct;
                return true;
            case PropType::Array:
            case PropType::Map:
            case PropType::FixedArray:
            case PropType::Pointer:
                return false;
            }
            return false;
        }
    }

    namespace {
        bool isZero(UProperty* property, const uint8_t* value)
        {
            std::vector<uint8_t> zero(property->elementSize());
            property->construct(zero.data());
            bool result = property->equals(value, zero.data());
            property->destroy(zero.data());
            return result;
        }
    }

    void Writer::u8(uint8_t value)
    {
        buffer.push_back(value);
    }

    void Writer::i16(int16_t value)
    {
        bytes(&value, sizeof(value));
    }

    void Writer::i32(int32_t value)
    {
        bytes(&value, sizeof(value));
    }

    void Writer::u32(uint32_t value)
    {
        bytes(&value, sizeof(value));
    }

    void Writer::u64(uint64_t value)
    {
        bytes(&value, sizeof(value));
    }

    void Writer::f32(float value)
    {
        bytes(&value, sizeof(value));
    }

    void Writer::index(int32_t value)
    {
        uint32_t magnitude = value < 0 ? static_cast<uint32_t>(-static_cast<int64_t>(value)) : static_cast<uint32_t>(value);
        uint8_t first = static_cast<uint8_t>(magnitude & 0x3f);
        if (value < 0) {
            first |= 0x80;
        }
        magnitude >>= 6;
        if (magnitude) {
            first |= 0x40;
        }
        u8(first);
        while (magnitude) {
            uint8_t next = static_cast<uint8_t>(magnitude & 0x7f);
            magnitude >>= 7;
            if (magnitude) {
                next |= 0x80;
            }
            u8(next);
        }
    }

    void Writer::string(const std::string& value)
    {
        size_t length = std::strlen(value.c_str());
        if (length == 0) {
            index(0);
            return;
        }
        index(static_cast<int32_t>(length + 1));
        bytes(value.c_str(), length);
        u8(0);
    }

    void Writer::bytes(const void* source, size_t count)
    {
        const uint8_t* begin = static_cast<const uint8_t*>(source);
        buffer.insert(buffer.end(), begin, begin + count);
    }

    void Writer::patchU32(size_t offset, uint32_t value)
    {
        std::memcpy(buffer.data() + offset, &value, sizeof(value));
    }

    size_t Writer::size() const
    {
        return buffer.size();
    }

    const std::vector<uint8_t>& Writer::data() const
    {
        return buffer;
    }

    PackageWriter::PackageWriter(Name packageName)
        : package(packageName)
    {
        nameIndex(Name("None"));
    }

    int32_t PackageWriter::addExport(UObject* object, UObject* outer)
    {
        auto it = exportLookup.find(object);
        if (it != exportLookup.end()) {
            return it->second;
        }
        exports.push_back({ object, outer });
        int32_t ref = static_cast<int32_t>(exports.size());
        exportLookup.emplace(object, ref);
        return ref;
    }

    bool PackageWriter::isExport(UObject* object) const
    {
        return exportLookup.count(object) != 0;
    }

    void PackageWriter::setFilter(ReferenceFilter referenceFilter)
    {
        filter = std::move(referenceFilter);
    }

    void PackageWriter::setNativeSerializer(NativeSerializer serializer)
    {
        nativeSerializer = std::move(serializer);
    }

    void PackageWriter::setFlagSelector(FlagSelector selector)
    {
        flagSelector = std::move(selector);
    }

    int32_t PackageWriter::nameIndex(Name name)
    {
        auto it = nameLookup.find(name.index());
        if (it != nameLookup.end()) {
            return it->second;
        }
        int32_t index = static_cast<int32_t>(names.size());
        names.push_back(name);
        nameLookup.emplace(name.index(), index);
        return index;
    }

    int32_t PackageWriter::objectRef(UObject* object)
    {
        if (!object || object->destroyed) {
            return 0;
        }
        auto it = exportLookup.find(object);
        if (it != exportLookup.end()) {
            return it->second;
        }
        if (filter && !filter(object)) {
            return 0;
        }
        return importRef(object);
    }

    int32_t PackageWriter::importRef(UObject* object)
    {
        auto it = importLookup.find(object);
        if (it != importLookup.end()) {
            return it->second;
        }
        ImportRecord record;
        record.name = object->name;
        if (!object->outer) {
            if (!objects::findLinker(object->name) && !objects::loadPackage(object->name)) {
                return 0;
            }
            record.classPackage = Name("Core");
            record.className = Name("Package");
        } else {
            if ((!object->linker && !cast<UClass>(object)) || !object->cls) {
                return 0;
            }
            int32_t outerRef = exportLookup.count(object->outer) ? 0 : importRef(object->outer);
            if (outerRef == 0) {
                return 0;
            }
            record.outer = outerRef;
            record.className = object->cls->name;
            UObject* classRoot = object->cls->outermost();
            record.classPackage = classRoot && classRoot != object->cls ? classRoot->name : Name("Core");
        }
        nameIndex(record.classPackage);
        nameIndex(record.className);
        nameIndex(record.name);
        imports.push_back(record);
        int32_t ref = -static_cast<int32_t>(imports.size());
        importLookup.emplace(object, ref);
        return ref;
    }

    void PackageWriter::writeObject(Writer& writer, UObject* object)
    {
        writer.index(objectRef(object));
    }

    void PackageWriter::writeName(Writer& writer, Name name)
    {
        writer.index(nameIndex(name));
    }

    bool PackageWriter::writeValue(Writer& writer, UProperty* property, const uint8_t* memory)
    {
        switch (property->type) {
        case PropType::Byte:
            writer.u8(*memory);
            return true;
        case PropType::Int:
            writer.i32(*reinterpret_cast<const int32_t*>(memory));
            return true;
        case PropType::Bool:
            writer.u8(*reinterpret_cast<const uint32_t*>(memory) != 0 ? 1 : 0);
            return true;
        case PropType::Float:
            writer.f32(*reinterpret_cast<const float*>(memory));
            return true;
        case PropType::Object:
        case PropType::Class:
            writeObject(writer, *reinterpret_cast<UObject* const*>(memory));
            return true;
        case PropType::Name:
            writeName(writer, Name::fromIndex(*reinterpret_cast<const uint32_t*>(memory)));
            return true;
        case PropType::Str:
        case PropType::String:
            writer.string(*reinterpret_cast<const std::string*>(memory));
            return true;
        case PropType::Struct:
            if (!property->structType) {
                return false;
            }
            for (UProperty* member : property->structType->fieldOrder()) {
                if (member->propertyFlags & (CPF_Native | CPF_Transient)) {
                    continue;
                }
                for (int32_t i = 0; i < member->arrayDim; i++) {
                    if (!writeValue(writer, member, memory + member->offset + static_cast<size_t>(i) * member->elementSize())) {
                        return false;
                    }
                }
            }
            return true;
        case PropType::Array:
        case PropType::Map:
        case PropType::FixedArray:
        case PropType::Pointer:
            return false;
        }
        return false;
    }

    void PackageWriter::writeTaggedProperties(Writer& writer, UStruct* structType, const uint8_t* data, const uint8_t* defaults, size_t defaultsSize)
    {
        if (structType && data) {
            const std::vector<UProperty*> properties = structType->fieldOrder();
            for (size_t p = 0; p < properties.size(); p++) {
                UProperty* property = properties[p];
                if (property->propertyFlags & (CPF_Transient | CPF_Native)) {
                    continue;
                }
                bool shadowed = false;
                for (size_t earlier = 0; earlier < p; earlier++) {
                    if (properties[earlier]->name == property->name) {
                        shadowed = true;
                    }
                }
                uint8_t tag = 0;
                if (shadowed || !tagTypeOf(property->type, tag)) {
                    continue;
                }
                for (int32_t index = 0; index < property->arrayDim; index++) {
                    size_t at = property->offset + static_cast<size_t>(index) * property->elementSize();
                    if (defaults && at + property->elementSize() <= defaultsSize) {
                        if (property->equals(data + at, defaults + at)) {
                            continue;
                        }
                    } else if (isZero(property, data + at)) {
                        continue;
                    }
                    if (tag == TagBool) {
                        if (index > 0) {
                            continue;
                        }
                        bool value = *reinterpret_cast<const uint32_t*>(data + at) != 0;
                        writeName(writer, property->name);
                        writer.u8(static_cast<uint8_t>(TagBool | (value ? 0x80 : 0)));
                        continue;
                    }
                    Writer value;
                    if (!writeValue(value, property, data + at)) {
                        continue;
                    }
                    size_t size = value.size();
                    uint8_t sizeCode = 7;
                    if (size == 1) {
                        sizeCode = 0;
                    } else if (size == 2) {
                        sizeCode = 1;
                    } else if (size == 4) {
                        sizeCode = 2;
                    } else if (size == 12) {
                        sizeCode = 3;
                    } else if (size == 16) {
                        sizeCode = 4;
                    } else if (size <= 0xff) {
                        sizeCode = 5;
                    } else if (size <= 0xffff) {
                        sizeCode = 6;
                    }
                    writeName(writer, property->name);
                    writer.u8(static_cast<uint8_t>(tag | (sizeCode << 4) | (index > 0 ? 0x80 : 0)));
                    if (tag == TagStruct) {
                        writeName(writer, property->structType->name);
                    }
                    if (sizeCode == 5) {
                        writer.u8(static_cast<uint8_t>(size));
                    } else if (sizeCode == 6) {
                        writer.i16(static_cast<int16_t>(static_cast<uint16_t>(size)));
                    } else if (sizeCode == 7) {
                        writer.u32(static_cast<uint32_t>(size));
                    }
                    if (index > 0) {
                        if (index < 0x80) {
                            writer.u8(static_cast<uint8_t>(index));
                        } else if (index < 0x4000) {
                            writer.u8(static_cast<uint8_t>(0x80 | (index >> 8)));
                            writer.u8(static_cast<uint8_t>(index & 0xff));
                        } else {
                            writer.u8(static_cast<uint8_t>(0xc0 | ((index >> 24) & 0x3f)));
                            writer.u8(static_cast<uint8_t>((index >> 16) & 0xff));
                            writer.u8(static_cast<uint8_t>((index >> 8) & 0xff));
                            writer.u8(static_cast<uint8_t>(index & 0xff));
                        }
                    }
                    writer.bytes(value.data().data(), value.size());
                }
            }
        }
        writeName(writer, Name("None"));
    }

    void PackageWriter::writeExportData(Writer& writer, UObject* object, uint32_t& flags)
    {
        StateFrame* frame = object->stateFrame.get();
        if (frame && (frame->state || frame->node)) {
            flags |= RF_HasStackFlag;
            UStruct* node = frame->node;
            writer.index(objectRef(node));
            writer.index(objectRef(frame->state ? static_cast<UObject*>(frame->state) : object->cls));
            uint64_t base = frame->state ? (frame->state->probeMask | object->cls->probeMask) & frame->state->ignoreMask : object->cls->probeMask & object->cls->ignoreMask;
            writer.u64(base & ~frame->disabledProbes);
            writer.i32(frame->latentAction);
            if (objectRef(node) != 0) {
                int32_t offset = -1;
                if (frame->code && node && frame->code >= node->script.data() && frame->code < node->script.data() + node->script.size()) {
                    offset = static_cast<int32_t>(frame->code - node->script.data());
                }
                writer.index(offset);
            }
        }
        if (!cast<UClass>(object)) {
            UClass* cls = object->cls;
            const uint8_t* defaults = cls && cls->defaultObject && cls->defaultObject != object ? cls->defaultObject->data : nullptr;
            writeTaggedProperties(writer, cls, object->data, defaults);
        }
        if (nativeSerializer) {
            nativeSerializer(*this, object, writer);
        }
    }

    bool PackageWriter::save(const std::filesystem::path& path, std::string* error)
    {
        try {
            std::vector<Writer> bodies(exports.size());
            std::vector<uint32_t> flags(exports.size());
            for (size_t i = 0; i < exports.size(); i++) {
                UObject* object = exports[i].object;
                flags[i] = flagSelector ? flagSelector(object) : (object->flags & SavedFlagMask) | RF_Public;
                writeExportData(bodies[i], object, flags[i]);
            }
            std::vector<int32_t> classRefs(exports.size());
            std::vector<int32_t> superRefs(exports.size());
            std::vector<int32_t> outerRefs(exports.size());
            for (size_t i = 0; i < exports.size(); i++) {
                UObject* object = exports[i].object;
                classRefs[i] = cast<UClass>(object) ? 0 : objectRef(object->cls);
                UStruct* structType = cast<UStruct>(object);
                superRefs[i] = structType ? objectRef(structType->superField) : 0;
                auto outer = exportLookup.find(exports[i].outer);
                outerRefs[i] = outer != exportLookup.end() ? outer->second : 0;
                nameIndex(exports[i].object->name);
            }

            Writer file;
            file.u32(PackageTag);
            file.i16(static_cast<int16_t>(PackageVersion));
            file.i16(0);
            file.u32(PackageFlagsAllowDownload);
            size_t countsAt = file.size();
            for (int i = 0; i < 6; i++) {
                file.u32(0);
            }
            std::random_device entropy;
            for (int i = 0; i < 4; i++) {
                file.u32(entropy());
            }
            file.u32(1);
            file.u32(static_cast<uint32_t>(exports.size()));
            file.u32(static_cast<uint32_t>(names.size()));

            size_t nameOffset = file.size();
            for (Name name : names) {
                file.string(name.str());
                file.u32(NameFlags);
            }

            std::vector<size_t> offsets(exports.size());
            for (size_t i = 0; i < exports.size(); i++) {
                offsets[i] = file.size();
                file.bytes(bodies[i].data().data(), bodies[i].size());
            }

            size_t importOffset = file.size();
            for (const ImportRecord& record : imports) {
                file.index(nameIndex(record.classPackage));
                file.index(nameIndex(record.className));
                file.i32(record.outer);
                file.index(nameIndex(record.name));
            }

            size_t exportOffset = file.size();
            for (size_t i = 0; i < exports.size(); i++) {
                file.index(classRefs[i]);
                file.index(superRefs[i]);
                file.i32(outerRefs[i]);
                file.index(nameIndex(exports[i].object->name));
                file.u32(flags[i]);
                file.index(static_cast<int32_t>(bodies[i].size()));
                if (bodies[i].size() > 0) {
                    file.index(static_cast<int32_t>(offsets[i]));
                }
            }

            file.patchU32(countsAt, static_cast<uint32_t>(names.size()));
            file.patchU32(countsAt + 4, static_cast<uint32_t>(nameOffset));
            file.patchU32(countsAt + 8, static_cast<uint32_t>(exports.size()));
            file.patchU32(countsAt + 12, static_cast<uint32_t>(exportOffset));
            file.patchU32(countsAt + 16, static_cast<uint32_t>(imports.size()));
            file.patchU32(countsAt + 20, static_cast<uint32_t>(importOffset));

            std::filesystem::create_directories(path.parent_path());
            std::filesystem::path temporary = path;
            temporary += ".tmp";
            {
                std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
                if (!out) {
                    throw std::runtime_error("cannot write " + temporary.string());
                }
                out.write(reinterpret_cast<const char*>(file.data().data()), static_cast<std::streamsize>(file.size()));
                if (!out) {
                    throw std::runtime_error("write failed for " + temporary.string());
                }
            }
            std::filesystem::rename(temporary, path);
            return true;
        } catch (const std::exception& failure) {
            if (error) {
                *error = failure.what();
            }
            return false;
        }
    }
}
