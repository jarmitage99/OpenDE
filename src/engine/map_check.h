#pragma once

#include "core/object.h"

#include <string>
#include <vector>

namespace dx {
    class ULevel;

    namespace mapcheck {
        enum class Severity : uint8_t {
            Error,
            Warning,
        };

        struct Issue {
            Severity severity = Severity::Warning;
            UObject* actor = nullptr;
            std::string message;
        };

        std::vector<Issue> check(ULevel* level);
        const char* severityName(Severity severity);
    }
}
