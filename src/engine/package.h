#pragma once

#include "engine/reader.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace dx {
    bool iequals(std::string_view a, std::string_view b);

    struct NameEntry {
        std::string name;
        uint32_t flags = 0;
    };

    struct ImportEntry {
        int32_t classPackage = 0;
        int32_t className = 0;
        int32_t outer = 0;
        int32_t name = 0;
    };

    struct ExportEntry {
        int32_t classRef = 0;
        int32_t superRef = 0;
        int32_t outer = 0;
        int32_t name = 0;
        uint32_t flags = 0;
        int32_t size = 0;
        int32_t offset = 0;
    };

    enum class PropertyType : uint8_t {
        Byte = 1,
        Int = 2,
        Bool = 3,
        Float = 4,
        Object = 5,
        Name = 6,
        String = 7,
        Class = 8,
        Array = 9,
        Struct = 10,
        Vector = 11,
        Rotator = 12,
        Str = 13,
        Map = 14,
        FixedArray = 15,
    };

    struct Property {
        std::string name;
        PropertyType type = PropertyType::Byte;
        std::string structName;
        int32_t arrayIndex = 0;
        bool boolValue = false;
        size_t offset = 0;
        size_t size = 0;
    };

    constexpr uint32_t RF_HasStack = 0x02000000;

    class Package;

    class Object {
    public:
        Object(const Package& package, std::vector<Property> properties, size_t nativeOffset);

        const std::vector<Property>& properties() const;
        const Property* find(std::string_view name, int32_t arrayIndex = 0) const;
        std::optional<uint8_t> byteValue(std::string_view name, int32_t arrayIndex = 0) const;
        std::optional<int32_t> intValue(std::string_view name, int32_t arrayIndex = 0) const;
        std::optional<float> floatValue(std::string_view name, int32_t arrayIndex = 0) const;
        std::optional<bool> boolValue(std::string_view name, int32_t arrayIndex = 0) const;
        std::optional<int32_t> objectValue(std::string_view name, int32_t arrayIndex = 0) const;
        std::optional<std::string> nameValue(std::string_view name, int32_t arrayIndex = 0) const;
        std::optional<std::array<float, 3>> vectorValue(std::string_view name, int32_t arrayIndex = 0) const;
        Reader native() const;

    private:
        Reader valueReader(const Property& property) const;

        const Package& owner;
        std::vector<Property> props;
        size_t nativeStart;
    };

    class Package {
    public:
        explicit Package(const std::filesystem::path& path);

        const std::string& name() const;
        uint16_t version() const;
        uint16_t licensee() const;
        const std::vector<NameEntry>& names() const;
        const std::vector<ImportEntry>& imports() const;
        const std::vector<ExportEntry>& exports() const;
        std::span<const uint8_t> data() const;

        const std::string& nameAt(int32_t index) const;
        const ImportEntry& importAt(int32_t ref) const;
        const ExportEntry& exportAt(int32_t ref) const;
        std::string objectName(int32_t ref) const;
        std::string classOf(int32_t ref) const;
        std::string groupOf(int32_t ref) const;
        int32_t findExport(std::string_view className, std::string_view objectName) const;
        Object load(int32_t ref) const;

    private:
        std::string packageName;
        std::vector<uint8_t> bytes;
        uint16_t fileVersion = 0;
        uint16_t licenseeMode = 0;
        std::vector<NameEntry> nameTable;
        std::vector<ImportEntry> importTable;
        std::vector<ExportEntry> exportTable;
    };
}
