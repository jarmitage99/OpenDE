#pragma once

#include "core/object.h"

#include <string>
#include <vector>

namespace dx {
    namespace actorsearch {
        struct Query {
            std::string name;
            std::string className;
            std::string tag;
            std::string event;
        };

        bool matches(UObject* actor, const Query& query);
        std::vector<UObject*> find(const std::vector<UObject*>& actors, const Query& query);
        UObject* findNext(const std::vector<UObject*>& actors, const Query& query, UObject* after);
    }
}
