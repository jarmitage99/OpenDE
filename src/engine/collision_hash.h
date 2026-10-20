#pragma once

#include "core/math.h"
#include "core/object.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace dx {
    class CollisionHash {
    public:
        void clear();
        void update(UObject* actor);
        void remove(UObject* actor);
        void sync(const std::vector<UObject*>& actors);
        const std::vector<UObject*>& query(const Vector& boxMin, const Vector& boxMax);

    private:
        struct Entry {
            UObject* actor = nullptr;
            uint64_t order = 0;
            Vector min;
            Vector max;
            int32_t x0 = 0;
            int32_t y0 = 0;
            int32_t x1 = 0;
            int32_t y1 = 0;
            bool always = false;
            bool bounded = false;
            bool placed = false;
            bool live = false;
            uint32_t stamp = 0;
            uint32_t seen = 0;
        };

        static int64_t cellKey(int32_t x, int32_t y);
        static int32_t cellOf(float value);
        void insert(uint32_t slot);
        void erase(uint32_t slot);
        void place(uint32_t slot, const Vector& min, const Vector& max, bool bounded);

        std::vector<Entry> entries;
        std::vector<uint32_t> freeSlots;
        std::unordered_map<UObject*, uint32_t> slots;
        std::unordered_map<int64_t, std::vector<uint32_t>> cells;
        std::vector<uint32_t> alwaysSlots;
        std::vector<uint32_t> found;
        std::vector<UObject*> result;
        uint64_t nextOrder = 0;
        uint32_t queryStamp = 0;
        uint32_t syncStamp = 0;
    };
}
