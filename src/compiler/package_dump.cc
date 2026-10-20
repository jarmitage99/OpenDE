#include "compiler/package_dump.h"

#include "engine/package.h"
#include "engine/reader.h"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

namespace dx::compiler {
    namespace {
        constexpr uint32_t HasStack = 0x02000000;
        constexpr uint32_t PropertyNative = 0x00001000;
        constexpr uint32_t PropertyTransient = 0x00002000;
        constexpr uint32_t PropertyNet = 0x00000020;
        constexpr uint32_t FunctionNet = 0x00000040;

        std::string lower(std::string text)
        {
            for (char& c : text) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return text;
        }

        std::string hex(const uint8_t* bytes, size_t count)
        {
            std::string text;
            char buffer[4];
            for (size_t i = 0; i < count; i++) {
                std::snprintf(buffer, sizeof(buffer), "%02x", bytes[i]);
                text += buffer;
            }
            return text;
        }

        std::string number(uint64_t value)
        {
            return std::to_string(value);
        }

        struct Location {
            const Package* package = nullptr;
            int32_t ref = 0;
        };

        struct FieldInfo {
            std::string name;
            std::string cls;
            int32_t arrayDim = 1;
            uint32_t flags = 0;
            Location type;
            Location inner;
            int32_t count = 0;
        };

        class Packages {
        public:
            explicit Packages(std::filesystem::path systemDirectory)
                : system(std::move(systemDirectory))
            {
            }

            const Package* open(const std::string& name)
            {
                std::string key = lower(name);
                auto it = loaded.find(key);
                if (it != loaded.end()) {
                    return it->second.get();
                }
                std::unique_ptr<Package> package;
                std::filesystem::path path = system / (name + ".u");
                if (std::filesystem::exists(path)) {
                    package = std::make_unique<Package>(path);
                }
                const Package* result = package.get();
                loaded.emplace(key, std::move(package));
                return result;
            }

            void add(const std::string& name, std::unique_ptr<Package> package)
            {
                loaded[lower(name)] = std::move(package);
            }

        private:
            std::filesystem::path system;
            std::map<std::string, std::unique_ptr<Package>> loaded;
        };

        std::string objectPath(const Package& package, int32_t ref)
        {
            if (ref == 0) {
                return "None";
            }
            if (ref > 0) {
                const ExportEntry& entry = package.exportAt(ref);
                std::string name = package.nameAt(entry.name);
                return entry.outer ? objectPath(package, entry.outer) + "." + name : package.name() + "." + name;
            }
            const ImportEntry& entry = package.importAt(ref);
            std::string name = package.nameAt(entry.name);
            return entry.outer ? objectPath(package, entry.outer) + "." + name : name;
        }

        std::string exportPath(const Package& package, int32_t ref)
        {
            const ExportEntry& entry = package.exportAt(ref);
            std::string name = package.nameAt(entry.name);
            return entry.outer ? exportPath(package, entry.outer) + "." + name : name;
        }

        std::string exportClass(const Package& package, int32_t ref)
        {
            const ExportEntry& entry = package.exportAt(ref);
            if (entry.classRef == 0) {
                return "Class";
            }
            if (entry.classRef < 0) {
                return package.nameAt(package.importAt(entry.classRef).name);
            }
            return package.nameAt(package.exportAt(entry.classRef).name);
        }

        class Dumper {
        public:
            Dumper(Packages& set, const Package& file)
                : packages(set)
                , package(file)
            {
            }

            std::map<std::string, DumpedExport> run()
            {
                std::map<std::string, DumpedExport> result;
                for (size_t i = 0; i < package.exports().size(); i++) {
                    int32_t ref = static_cast<int32_t>(i + 1);
                    const ExportEntry& entry = package.exportAt(ref);
                    DumpedExport dumped;
                    dumped.className = exportClass(package, ref);
                    dumped.super = objectPath(package, entry.superRef);
                    dumped.flags = entry.flags;
                    if (entry.size > 0) {
                        dumped.body = body(ref, dumped.className);
                    }
                    result[exportPath(package, ref)] = dumped;
                }
                return result;
            }

        private:
            Location resolve(const Package* source, int32_t ref)
            {
                if (ref >= 0) {
                    return { source, ref };
                }
                std::vector<std::string> chain;
                int32_t current = ref;
                while (current < 0) {
                    const ImportEntry& entry = source->importAt(current);
                    chain.insert(chain.begin(), source->nameAt(entry.name));
                    current = entry.outer;
                }
                const Package* target = packages.open(chain.front());
                if (!target) {
                    return {};
                }
                int32_t outer = 0;
                int32_t found = 0;
                for (size_t level = 1; level < chain.size(); level++) {
                    found = 0;
                    for (size_t i = 0; i < target->exports().size(); i++) {
                        const ExportEntry& entry = target->exports()[i];
                        if (entry.outer == outer && iequals(target->nameAt(entry.name), chain[level])) {
                            found = static_cast<int32_t>(i + 1);
                            break;
                        }
                    }
                    if (!found) {
                        return {};
                    }
                    outer = found;
                }
                return { target, found };
            }

            Reader open(const Package& source, int32_t ref, size_t& end)
            {
                const ExportEntry& entry = source.exportAt(ref);
                end = static_cast<size_t>(entry.offset) + static_cast<size_t>(entry.size);
                return Reader(source.data(), static_cast<size_t>(entry.offset));
            }

            void skipTagged(const Package& source, Reader& reader)
            {
                while (true) {
                    int32_t nameIndex = reader.index();
                    if (iequals(source.nameAt(nameIndex), "None")) {
                        return;
                    }
                    uint8_t info = reader.u8();
                    if ((info & 0x0f) == 10) {
                        reader.index();
                    }
                    size_t size = tagSize(reader, info);
                    if ((info & 0x0f) != 3 && (info & 0x80)) {
                        arrayIndex(reader);
                    }
                    if ((info & 0x0f) == 3) {
                        size = 0;
                    }
                    reader.skip(size);
                }
            }

            FieldInfo fieldInfo(Location location)
            {
                FieldInfo info;
                const Package& source = *location.package;
                info.cls = exportClass(source, location.ref);
                info.name = source.nameAt(source.exportAt(location.ref).name);
                size_t end = 0;
                Reader reader = open(source, location.ref, end);
                skipTagged(source, reader);
                reader.index();
                reader.index();
                info.arrayDim = reader.i32();
                info.flags = reader.u32();
                reader.index();
                if (info.flags & PropertyNet) {
                    reader.i16();
                }
                if (info.cls == "ByteProperty" || info.cls == "ObjectProperty" || info.cls == "StructProperty") {
                    info.type = resolve(&source, reader.index());
                } else if (info.cls == "ClassProperty") {
                    info.type = resolve(&source, reader.index());
                    reader.index();
                } else if (info.cls == "ArrayProperty") {
                    info.inner = resolve(&source, reader.index());
                } else if (info.cls == "FixedArrayProperty") {
                    info.inner = resolve(&source, reader.index());
                    info.count = reader.i32();
                }
                return info;
            }

            std::vector<FieldInfo> layout(Location location)
            {
                std::vector<FieldInfo> fields;
                while (location.package && location.ref > 0) {
                    const Package& source = *location.package;
                    size_t end = 0;
                    Reader reader = open(source, location.ref, end);
                    const ExportEntry& entry = source.exportAt(location.ref);
                    if (entry.flags & HasStack) {
                        int32_t node = reader.index();
                        reader.index();
                        reader.u64();
                        reader.i32();
                        if (node != 0) {
                            reader.index();
                        }
                    }
                    if (entry.classRef != 0) {
                        skipTagged(source, reader);
                    }
                    int32_t super = reader.index();
                    reader.index();
                    reader.index();
                    int32_t child = reader.index();
                    while (child > 0) {
                        std::string childClass = exportClass(source, child);
                        size_t childEnd = 0;
                        Reader childReader = open(source, child, childEnd);
                        skipTagged(source, childReader);
                        childReader.index();
                        int32_t next = childReader.index();
                        if (childClass.size() > 8 && childClass.compare(childClass.size() - 8, 8, "Property") == 0) {
                            fields.push_back(fieldInfo({ &source, child }));
                        }
                        child = next;
                    }
                    location = resolve(&source, super);
                }
                return fields;
            }

            static size_t tagSize(Reader& reader, uint8_t info)
            {
                switch ((info >> 4) & 0x07) {
                case 0:
                    return 1;
                case 1:
                    return 2;
                case 2:
                    return 4;
                case 3:
                    return 12;
                case 4:
                    return 16;
                case 5:
                    return reader.u8();
                case 6:
                    return static_cast<uint16_t>(reader.i16());
                default:
                    return reader.u32();
                }
            }

            static int32_t arrayIndex(Reader& reader)
            {
                uint8_t first = reader.u8();
                if ((first & 0xc0) == 0xc0) {
                    int32_t value = (first & 0x3f) << 24;
                    value |= reader.u8() << 16;
                    value |= reader.u8() << 8;
                    return value | reader.u8();
                }
                if (first & 0x80) {
                    return ((first & 0x7f) << 8) | reader.u8();
                }
                return first;
            }

            std::string ref(Reader& reader)
            {
                return "{" + objectPath(package, reader.index()) + "}";
            }

            std::string name(Reader& reader)
            {
                return "'" + package.nameAt(reader.index()) + "'";
            }

            std::string item(Reader& reader, const FieldInfo& field)
            {
                const std::string& cls = field.cls;
                if (cls == "ByteProperty" || cls == "BoolProperty") {
                    return number(reader.u8());
                }
                if (cls == "IntProperty" || cls == "FloatProperty") {
                    return number(reader.u32());
                }
                if (cls == "ObjectProperty" || cls == "ClassProperty") {
                    return ref(reader);
                }
                if (cls == "NameProperty") {
                    return name(reader);
                }
                if (cls == "StrProperty") {
                    return "\"" + reader.string() + "\"";
                }
                if (cls == "StructProperty") {
                    return structValue(reader, field.type);
                }
                if (cls == "FixedArrayProperty") {
                    FieldInfo inner = fieldInfo(field.inner);
                    std::string text = "[";
                    for (int32_t i = 0; i < field.count; i++) {
                        text += item(reader, inner) + ";";
                    }
                    return text + "]";
                }
                throw std::runtime_error("cannot decode struct member of class " + cls);
            }

            std::string structValue(Reader& reader, Location location)
            {
                std::string text = "(";
                for (const FieldInfo& field : layout(location)) {
                    if (field.flags & (PropertyNative | PropertyTransient)) {
                        continue;
                    }
                    for (int32_t i = 0; i < field.arrayDim; i++) {
                        text += field.name + "=" + item(reader, field) + ",";
                    }
                }
                return text + ")";
            }

            std::string tagged(Reader& reader, Location owner)
            {
                std::string text = "props[";
                std::vector<FieldInfo> fields;
                bool fieldsLoaded = false;
                while (true) {
                    std::string propertyName = package.nameAt(reader.index());
                    if (iequals(propertyName, "None")) {
                        break;
                    }
                    uint8_t info = reader.u8();
                    uint8_t type = info & 0x0f;
                    std::string structName;
                    if (type == 10) {
                        structName = package.nameAt(reader.index());
                    }
                    size_t size = tagSize(reader, info);
                    int32_t index = 0;
                    if (type != 3 && (info & 0x80)) {
                        index = arrayIndex(reader);
                    }
                    text += propertyName + "[" + number(static_cast<uint64_t>(index)) + "]:" + number(type) + "=";
                    size_t start = reader.tell();
                    switch (type) {
                    case 3:
                        text += (info & 0x80) ? "True" : "False";
                        size = 0;
                        break;
                    case 5:
                    case 8:
                        text += ref(reader);
                        break;
                    case 6:
                        text += name(reader);
                        break;
                    case 10: {
                        if (!fieldsLoaded) {
                            fields = layout(owner);
                            fieldsLoaded = true;
                        }
                        const FieldInfo* field = nullptr;
                        for (const FieldInfo& candidate : fields) {
                            if (iequals(candidate.name, propertyName) && candidate.cls == "StructProperty") {
                                field = &candidate;
                                break;
                            }
                        }
                        if (!field) {
                            throw std::runtime_error("no struct property " + propertyName);
                        }
                        text += structName + structValue(reader, field->type);
                        break;
                    }
                    default: {
                        std::vector<uint8_t> raw(size);
                        reader.bytes(raw.data(), size);
                        text += hex(raw.data(), size);
                        break;
                    }
                    }
                    if (reader.tell() != start + size) {
                        throw std::runtime_error("tag size mismatch for " + propertyName);
                    }
                    text += " ";
                }
                return text + "]";
            }

            uint8_t expr(Reader& reader, std::string& out, size_t& memory)
            {
                uint8_t token = reader.u8();
                memory++;
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "%02x", token);
                out += buffer;
                out += " ";
                auto u16 = [&]() {
                    out += number(static_cast<uint16_t>(reader.i16())) + " ";
                    memory += 2;
                };
                auto u8 = [&]() {
                    out += number(reader.u8()) + " ";
                    memory += 1;
                };
                auto u32 = [&]() {
                    out += number(reader.u32()) + " ";
                    memory += 4;
                };
                auto object = [&]() {
                    out += ref(reader) + " ";
                    memory += 4;
                };
                auto nameRef = [&]() {
                    out += name(reader) + " ";
                    memory += 4;
                };
                auto parms = [&]() {
                    while (expr(reader, out, memory) != 0x16) {
                    }
                };
                if (token >= 0x39 && token < 0x60) {
                    expr(reader, out, memory);
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
                    expr(reader, out, memory);
                    expr(reader, out, memory);
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
                    expr(reader, out, memory);
                    break;
                case 0x1c:
                    object();
                    parms();
                    break;
                case 0x1b:
                case 0x38:
                    nameRef();
                    parms();
                    break;
                case 0x12:
                case 0x19:
                    expr(reader, out, memory);
                    u16();
                    u8();
                    expr(reader, out, memory);
                    break;
                case 0x11:
                    for (int i = 0; i < 4; i++) {
                        expr(reader, out, memory);
                    }
                    break;
                case 0x1d:
                case 0x1e:
                    u32();
                    break;
                case 0x1f:
                    while (true) {
                        uint8_t c = reader.u8();
                        memory++;
                        out += number(c) + ",";
                        if (c == 0) {
                            break;
                        }
                    }
                    out += " ";
                    break;
                case 0x34:
                    while (true) {
                        uint16_t c = static_cast<uint16_t>(reader.i16());
                        memory += 2;
                        out += number(c) + ",";
                        if (c == 0) {
                            break;
                        }
                    }
                    out += " ";
                    break;
                case 0x21:
                    nameRef();
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
                    expr(reader, out, memory);
                    break;
                case 0x07:
                case 0x09:
                case 0x18:
                    u16();
                    expr(reader, out, memory);
                    break;
                case 0x2f:
                    expr(reader, out, memory);
                    u16();
                    break;
                case 0x05:
                    u8();
                    expr(reader, out, memory);
                    break;
                case 0x0a: {
                    uint16_t next = static_cast<uint16_t>(reader.i16());
                    memory += 2;
                    out += number(next) + " ";
                    if (next != 0xffff) {
                        expr(reader, out, memory);
                    }
                    break;
                }
                case 0x0c:
                    while (true) {
                        std::string label = package.nameAt(reader.index());
                        uint32_t offset = reader.u32();
                        memory += 8;
                        out += "'" + label + "'@" + number(offset) + " ";
                        if (iequals(label, "None")) {
                            break;
                        }
                    }
                    break;
                case 0x32:
                case 0x33:
                    object();
                    expr(reader, out, memory);
                    expr(reader, out, memory);
                    break;
                default:
                    throw std::runtime_error("unknown bytecode token " + number(token));
                }
                return token;
            }

            std::string field(Reader& reader)
            {
                std::string text = "super=" + ref(reader);
                text += " next=" + ref(reader);
                return text;
            }

            std::string structBody(Reader& reader)
            {
                std::string text = field(reader);
                text += " text=" + ref(reader);
                text += " children=" + ref(reader);
                text += " friendly=" + name(reader);
                text += " line=" + number(reader.u32());
                text += " pos=" + number(reader.u32());
                uint32_t scriptSize = reader.u32();
                text += " script(" + number(scriptSize) + ")=";
                size_t memory = 0;
                while (memory < scriptSize) {
                    expr(reader, text, memory);
                }
                if (memory != scriptSize) {
                    throw std::runtime_error("script size mismatch");
                }
                return text;
            }

            std::string body(int32_t ref, const std::string& className)
            {
                size_t end = 0;
                Reader reader = open(package, ref, end);
                const ExportEntry& entry = package.exportAt(ref);
                std::string text;
                try {
                    if (entry.flags & HasStack) {
                        int32_t node = reader.index();
                        text += "stack node=" + objectPath(package, node);
                        text += " state=" + this->ref(reader);
                        text += " probe=" + number(reader.u64());
                        text += " latent=" + number(static_cast<uint32_t>(reader.i32()));
                        if (node != 0) {
                            text += " offset=" + number(static_cast<uint32_t>(reader.index()));
                        }
                        text += "\n";
                    }
                    if (className != "Class") {
                        Location owner = resolve(&package, entry.classRef);
                        text += tagged(reader, owner) + "\n";
                    }
                    if (className == "Class") {
                        text += structBody(reader);
                        text += " probe=" + number(reader.u64());
                        text += " ignore=" + number(reader.u64());
                        text += " labels=" + number(static_cast<uint16_t>(reader.i16()));
                        text += " stateflags=" + number(reader.u32());
                        text += " classflags=" + number(reader.u32());
                        reader.skip(16);
                        int32_t dependencies = reader.index();
                        text += " deps=";
                        for (int32_t i = 0; i < dependencies; i++) {
                            text += this->ref(reader);
                            text += ":" + number(reader.u32());
                            text += ":" + number(reader.u32()) + ",";
                        }
                        int32_t imports = reader.index();
                        text += " imports=";
                        for (int32_t i = 0; i < imports; i++) {
                            text += name(reader) + ",";
                        }
                        text += " within=" + this->ref(reader);
                        text += " config=" + name(reader);
                        text += "\n" + tagged(reader, { &package, ref });
                    } else if (className == "Function") {
                        text += structBody(reader);
                        text += " native=" + number(static_cast<uint16_t>(reader.i16()));
                        text += " precedence=" + number(reader.u8());
                        uint32_t flags = reader.u32();
                        text += " flags=" + number(flags);
                        if (flags & FunctionNet) {
                            text += " rep=" + number(static_cast<uint16_t>(reader.i16()));
                        }
                    } else if (className == "State") {
                        text += structBody(reader);
                        text += " probe=" + number(reader.u64());
                        text += " ignore=" + number(reader.u64());
                        text += " labels=" + number(static_cast<uint16_t>(reader.i16()));
                        text += " stateflags=" + number(reader.u32());
                    } else if (className == "Struct") {
                        text += structBody(reader);
                    } else if (className == "Enum") {
                        text += field(reader);
                        int32_t count = reader.index();
                        text += " names=";
                        for (int32_t i = 0; i < count; i++) {
                            text += name(reader) + ",";
                        }
                    } else if (className == "Const") {
                        text += field(reader);
                        text += " value=\"" + reader.string() + "\"";
                    } else if (className == "TextBuffer") {
                        text += "pos=" + number(reader.u32());
                        text += " top=" + number(reader.u32());
                        text += " text=" + reader.string();
                    } else if (className.size() > 8 && className.compare(className.size() - 8, 8, "Property") == 0) {
                        text += field(reader);
                        text += " dim=" + number(static_cast<uint32_t>(reader.i32()));
                        uint32_t flags = reader.u32();
                        text += " flags=" + number(flags);
                        text += " category=" + name(reader);
                        if (flags & PropertyNet) {
                            text += " rep=" + number(static_cast<uint16_t>(reader.i16()));
                        }
                        if (className == "ByteProperty" || className == "ObjectProperty" || className == "StructProperty" || className == "ArrayProperty") {
                            text += " type=" + this->ref(reader);
                        } else if (className == "ClassProperty" || className == "MapProperty") {
                            text += " type=" + this->ref(reader);
                            text += " meta=" + this->ref(reader);
                        } else if (className == "FixedArrayProperty") {
                            text += " inner=" + this->ref(reader);
                            text += " count=" + number(static_cast<uint32_t>(reader.i32()));
                        }
                    }
                    if (reader.tell() < end) {
                        std::vector<uint8_t> rest(end - reader.tell());
                        reader.bytes(rest.data(), rest.size());
                        text += "\nrest=" + hex(rest.data(), rest.size());
                    }
                    if (reader.tell() != end) {
                        text += "\noverrun";
                    }
                } catch (const std::exception& error) {
                    text += "\nerror: " + std::string(error.what());
                }
                return text;
            }

            Packages& packages;
            const Package& package;
        };
    }

    std::map<std::string, DumpedExport> dumpPackage(const std::filesystem::path& file, const std::filesystem::path& systemDirectory)
    {
        Packages packages(systemDirectory);
        auto owned = std::make_unique<Package>(file);
        const Package& package = *owned;
        std::string name = package.name();
        packages.add(name, std::move(owned));
        Dumper dumper(packages, package);
        return dumper.run();
    }
}
