#pragma once

#include "core/object.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dx {
    class Writer {
    public:
        void u8(uint8_t value);
        void i16(int16_t value);
        void i32(int32_t value);
        void u32(uint32_t value);
        void u64(uint64_t value);
        void f32(float value);
        void index(int32_t value);
        void string(const std::string& value);
        void bytes(const void* source, size_t count);
        void patchU32(size_t offset, uint32_t value);
        size_t size() const;
        const std::vector<uint8_t>& data() const;

    private:
        std::vector<uint8_t> buffer;
    };

    class PackageWriter {
    public:
        using ReferenceFilter = std::function<bool(UObject*)>;
        using NativeSerializer = std::function<void(PackageWriter&, UObject*, Writer&)>;
        using FlagSelector = std::function<uint32_t(UObject*)>;

        explicit PackageWriter(Name packageName);

        int32_t addExport(UObject* object, UObject* outer);
        bool isExport(UObject* object) const;
        void setFilter(ReferenceFilter filter);
        void setNativeSerializer(NativeSerializer serializer);
        void setFlagSelector(FlagSelector selector);

        int32_t objectRef(UObject* object);
        int32_t nameIndex(Name name);
        void writeObject(Writer& writer, UObject* object);
        void writeName(Writer& writer, Name name);
        void writeTaggedProperties(Writer& writer, UStruct* structType, const uint8_t* data, const uint8_t* defaults, size_t defaultsSize = SIZE_MAX);
        bool save(const std::filesystem::path& path, std::string* error);

    private:
        struct ImportRecord {
            Name classPackage;
            Name className;
            int32_t outer = 0;
            Name name;
        };

        struct ExportRecord {
            UObject* object = nullptr;
            UObject* outer = nullptr;
        };

        int32_t importRef(UObject* object);
        bool writeValue(Writer& writer, UProperty* property, const uint8_t* memory);
        void writeExportData(Writer& writer, UObject* object, uint32_t& flags);

        Name package;
        std::vector<Name> names;
        std::unordered_map<uint32_t, int32_t> nameLookup;
        std::vector<ImportRecord> imports;
        std::unordered_map<UObject*, int32_t> importLookup;
        std::vector<ExportRecord> exports;
        std::unordered_map<UObject*, int32_t> exportLookup;
        ReferenceFilter filter;
        NativeSerializer nativeSerializer;
        FlagSelector flagSelector;
    };
}
