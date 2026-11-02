#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace dx::compiler {
    bool parseCommand(std::string_view& stream, std::string_view match);
    bool parseString(std::string_view stream, std::string_view match, std::string& value);
    bool parseInt(std::string_view stream, std::string_view match, int32_t& value);
    bool parseByte(std::string_view stream, std::string_view match, uint8_t& value);
    bool parseFloat(std::string_view stream, std::string_view match, float& value);
    bool parseBool(std::string_view stream, std::string_view match, bool& value);
}
