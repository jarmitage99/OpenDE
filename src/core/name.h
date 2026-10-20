#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace dx {
    class Name {
    public:
        Name() = default;
        Name(std::string_view text);
        Name(const char* text);
        Name(const std::string& text);

        static Name fromIndex(uint32_t index);
        static Name find(std::string_view text);

        const std::string& str() const;
        const char* c_str() const;
        uint32_t index() const;
        bool none() const;
        bool operator==(const Name& other) const;
        bool operator!=(const Name& other) const;
        bool operator<(const Name& other) const;

    private:
        uint32_t id = 0;
    };
}

template <>
struct std::hash<dx::Name> {
    size_t operator()(const dx::Name& name) const
    {
        return std::hash<uint32_t>()(name.index());
    }
};
