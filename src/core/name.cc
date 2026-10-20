#include "core/name.h"

#include <cctype>
#include <unordered_map>
#include <vector>

namespace dx {
    namespace {
        struct NameTable {
            std::vector<std::string> names;
            std::unordered_map<std::string, uint32_t> lookup;

            NameTable()
            {
                names.push_back("None");
                lookup.emplace("none", 0);
            }

            uint32_t lookupIndex(std::string_view text) const
            {
                std::string key(text);
                for (char& c : key) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                auto it = lookup.find(key);
                return it != lookup.end() ? it->second : 0;
            }

            uint32_t intern(std::string_view text)
            {
                std::string key(text);
                for (char& c : key) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                auto it = lookup.find(key);
                if (it != lookup.end()) {
                    return it->second;
                }
                uint32_t index = static_cast<uint32_t>(names.size());
                names.emplace_back(text);
                lookup.emplace(std::move(key), index);
                return index;
            }
        };

        NameTable& table()
        {
            static NameTable instance;
            return instance;
        }
    }

    Name::Name(std::string_view text)
        : id(text.empty() ? 0 : table().intern(text))
    {
    }

    Name::Name(const char* text)
        : Name(std::string_view(text))
    {
    }

    Name::Name(const std::string& text)
        : Name(std::string_view(text))
    {
    }

    Name Name::fromIndex(uint32_t index)
    {
        Name name;
        if (index < table().names.size()) {
            name.id = index;
        }
        return name;
    }

    Name Name::find(std::string_view text)
    {
        return fromIndex(text.empty() ? 0 : table().lookupIndex(text));
    }

    const std::string& Name::str() const
    {
        return table().names[id];
    }

    const char* Name::c_str() const
    {
        return table().names[id].c_str();
    }

    uint32_t Name::index() const
    {
        return id;
    }

    bool Name::none() const
    {
        return id == 0;
    }

    bool Name::operator==(const Name& other) const
    {
        return id == other.id;
    }

    bool Name::operator!=(const Name& other) const
    {
        return id != other.id;
    }

    bool Name::operator<(const Name& other) const
    {
        return id < other.id;
    }
}
