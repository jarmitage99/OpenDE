#include "engine/actor_search.h"

#include "engine/props_engine.h"

#include <algorithm>
#include <cctype>

namespace dx {
    namespace actorsearch {
        namespace {
            bool contains(const std::string& text, const std::string& part)
            {
                auto found = std::search(text.begin(), text.end(), part.begin(), part.end(), [](char a, char b) {
                    return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
                });
                return found != text.end();
            }
        }

        bool matches(UObject* actor, const Query& query)
        {
            if (!query.name.empty() && !contains(actor->name.str(), query.name)) {
                return false;
            }
            if (!query.className.empty() && !contains(actor->cls->name.str(), query.className)) {
                return false;
            }
            if (!query.tag.empty() && (actor::Tag(actor).none() || !contains(actor::Tag(actor).str(), query.tag))) {
                return false;
            }
            if (!query.event.empty() && (actor::Event(actor).none() || !contains(actor::Event(actor).str(), query.event))) {
                return false;
            }
            return true;
        }

        std::vector<UObject*> find(const std::vector<UObject*>& actors, const Query& query)
        {
            std::vector<UObject*> result;
            for (UObject* actor : actors) {
                if (matches(actor, query)) {
                    result.push_back(actor);
                }
            }
            return result;
        }

        UObject* findNext(const std::vector<UObject*>& actors, const Query& query, UObject* after)
        {
            auto start = std::find(actors.begin(), actors.end(), after);
            size_t first = start == actors.end() ? 0 : static_cast<size_t>(start - actors.begin()) + 1;
            for (size_t step = 0; step < actors.size(); step++) {
                UObject* actor = actors[(first + step) % actors.size()];
                if (matches(actor, query)) {
                    return actor;
                }
            }
            return nullptr;
        }
    }
}
