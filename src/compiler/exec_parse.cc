#include "compiler/exec_parse.h"

#include "engine/package.h"

#include <cctype>
#include <cstdlib>

namespace dx::compiler {
    namespace {
        size_t findNoCase(std::string_view stream, std::string_view match)
        {
            if (match.size() > stream.size()) {
                return std::string_view::npos;
            }
            for (size_t i = 0; i + match.size() <= stream.size(); i++) {
                if (iequals(stream.substr(i, match.size()), match)) {
                    return i;
                }
            }
            return std::string_view::npos;
        }

        bool rawValue(std::string_view stream, std::string_view match, std::string_view& value)
        {
            size_t found = findNoCase(stream, match);
            if (found == std::string_view::npos) {
                return false;
            }
            value = stream.substr(found + match.size());
            return true;
        }
    }

    bool parseCommand(std::string_view& stream, std::string_view match)
    {
        while (!stream.empty() && (stream.front() == ' ' || stream.front() == '\t')) {
            stream.remove_prefix(1);
        }
        if (stream.size() < match.size() || !iequals(stream.substr(0, match.size()), match)) {
            return false;
        }
        if (stream.size() > match.size() && std::isalnum(static_cast<unsigned char>(stream[match.size()]))) {
            return false;
        }
        stream.remove_prefix(match.size());
        while (!stream.empty() && (stream.front() == ' ' || stream.front() == '\t')) {
            stream.remove_prefix(1);
        }
        return true;
    }

    bool parseString(std::string_view stream, std::string_view match, std::string& value)
    {
        std::string_view start;
        if (!rawValue(stream, match, start)) {
            return false;
        }
        if (!start.empty() && start.front() == '"') {
            start.remove_prefix(1);
            value = std::string(start.substr(0, start.find('"')));
            return true;
        }
        value = std::string(start.substr(0, start.find_first_of(" \r\n\t,")));
        return true;
    }

    bool parseInt(std::string_view stream, std::string_view match, int32_t& value)
    {
        std::string_view start;
        if (!rawValue(stream, match, start)) {
            return false;
        }
        value = std::atoi(std::string(start).c_str());
        return true;
    }

    bool parseByte(std::string_view stream, std::string_view match, uint8_t& value)
    {
        std::string_view start;
        if (!rawValue(stream, match, start)) {
            return false;
        }
        value = static_cast<uint8_t>(std::atoi(std::string(start).c_str()));
        return value != 0 || (!start.empty() && std::isdigit(static_cast<unsigned char>(start.front())));
    }

    bool parseFloat(std::string_view stream, std::string_view match, float& value)
    {
        std::string_view start;
        if (!rawValue(stream, match, start)) {
            return false;
        }
        value = static_cast<float>(std::atof(std::string(start).c_str()));
        return true;
    }

    bool parseBool(std::string_view stream, std::string_view match, bool& value)
    {
        std::string text;
        if (!parseString(stream, match, text)) {
            return false;
        }
        value = iequals(text, "On") || iequals(text, "True") || iequals(text, "1");
        return true;
    }
}
