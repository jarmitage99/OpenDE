#pragma once

#include "engine/resources.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace dx {
    namespace editorops {
        enum class Adjacent {
            All,
            Coplanars,
            Walls,
            Floors,
            Slants,
        };

        enum class MemoryOp {
            Or,
            And,
            Xor,
        };

        void selectAllSurfs(const UModel& model, std::set<int32_t>& selection);
        void selectReverse(const UModel& model, std::set<int32_t>& selection);
        void selectMatchingGroups(const UModel& model, std::set<int32_t>& selection);
        void selectMatchingItems(const UModel& model, std::set<int32_t>& selection);
        void selectMatchingBrush(const UModel& model, std::set<int32_t>& selection);
        void selectMatchingTexture(const UModel& model, std::set<int32_t>& selection);
        void selectAdjacent(const UModel& model, std::set<int32_t>& selection, Adjacent type);
        void combineWithMemory(std::set<int32_t>& selection, const std::set<int32_t>& memory, MemoryOp op);
        std::set<int32_t> surfsUsingTexture(const UModel& model, const UObject* texture);
        std::string textureGroup(const UObject* texture);
        std::vector<std::string> textureGroups(const std::vector<UObject*>& textures);
        std::string textureFormatName(uint8_t format);
    }
}
