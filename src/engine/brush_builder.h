#pragma once

#include "engine/resources.h"

#include <string>
#include <vector>

namespace dx {
    namespace brushbuilder {
        struct Constraints {
            bool gridEnabled = false;
            Vector gridSize { 16, 16, 16 };
            bool rotGridEnabled = false;
            Rotator rotGridSize { 1024, 1024, 1024 };
        };

        struct BuilderPolygon {
            std::vector<int32_t> indices;
            int32_t direction = 1;
            Name item;
            int32_t flags = 0;
        };

        void registerNatives();
        bool finishBrush(UObject* brushActor, const std::vector<Vector>& vertices, std::vector<BuilderPolygon> polys, Name group, bool mergeCoplanars);
        void setTarget(UObject* builderBrush);
        Constraints& constraints();
        std::vector<UClass*> builderClasses();
        UObject* builderFor(UClass* cls);
        bool build(UObject* builder);
        std::string lastMessage();
        void validateBrush(UModel& brush, bool force);
    }
}
