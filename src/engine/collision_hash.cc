#include "engine/collision_hash.h"

#include "engine/actor.h"
#include "engine/props_engine.h"
#include "engine/resources.h"

#include <algorithm>
#include <cmath>

namespace dx {
    namespace {
        constexpr float cellSize = 256.0f;
        constexpr int32_t maxPlacedCells = 64;
        constexpr int32_t maxQueryCells = 1024;
        constexpr float brushMargin = 16.0f;

        bool brushBounds(UObject* actor, Vector& min, Vector& max)
        {
            UModel* model = dynamic_cast<UModel*>(actor::Brush(actor));
            if (!model || !model->bounds.valid) {
                return false;
            }
            Coords axes = rotationAxes(actor::Rotation(actor));
            Vector location = actor::Location(actor);
            Vector prePivot = actor::PrePivot(actor);
            Vector scale { 1, 1, 1 };
            Vector* mainScale = reinterpret_cast<Vector*>(brush::MainScale(actor));
            if (mainScale->x != 0 && mainScale->y != 0 && mainScale->z != 0) {
                scale = *mainScale;
            }
            const BoundingBox& bounds = model->bounds;
            for (int i = 0; i < 8; i++) {
                Vector corner { (i & 1) ? bounds.max.x : bounds.min.x, (i & 2) ? bounds.max.y : bounds.min.y, (i & 4) ? bounds.max.z : bounds.min.z };
                Vector local { (corner.x - prePivot.x) * scale.x, (corner.y - prePivot.y) * scale.y, (corner.z - prePivot.z) * scale.z };
                Vector point = location + untransformVector(axes, local);
                if (i == 0) {
                    min = point;
                    max = point;
                } else {
                    min = { std::min(min.x, point.x), std::min(min.y, point.y), std::min(min.z, point.z) };
                    max = { std::max(max.x, point.x), std::max(max.y, point.y), std::max(max.z, point.z) };
                }
            }
            min = { min.x - brushMargin, min.y - brushMargin, min.z - brushMargin };
            max = { max.x + brushMargin, max.y + brushMargin, max.z + brushMargin };
            return true;
        }
    }

    int64_t CollisionHash::cellKey(int32_t x, int32_t y)
    {
        return (static_cast<int64_t>(x) << 32) ^ static_cast<int64_t>(static_cast<uint32_t>(y));
    }

    int32_t CollisionHash::cellOf(float value)
    {
        float cell = std::floor(value / cellSize);
        return static_cast<int32_t>(std::clamp(cell, -1000000.0f, 1000000.0f));
    }

    void CollisionHash::clear()
    {
        entries.clear();
        freeSlots.clear();
        slots.clear();
        cells.clear();
        alwaysSlots.clear();
        nextOrder = 0;
    }

    void CollisionHash::insert(uint32_t slot)
    {
        Entry& entry = entries[slot];
        if (entry.always) {
            alwaysSlots.push_back(slot);
        } else {
            for (int32_t x = entry.x0; x <= entry.x1; x++) {
                for (int32_t y = entry.y0; y <= entry.y1; y++) {
                    cells[cellKey(x, y)].push_back(slot);
                }
            }
        }
        entry.placed = true;
    }

    void CollisionHash::erase(uint32_t slot)
    {
        Entry& entry = entries[slot];
        if (!entry.placed) {
            return;
        }
        auto drop = [slot](std::vector<uint32_t>& list) {
            auto it = std::find(list.begin(), list.end(), slot);
            if (it != list.end()) {
                *it = list.back();
                list.pop_back();
            }
        };
        if (entry.always) {
            drop(alwaysSlots);
        } else {
            for (int32_t x = entry.x0; x <= entry.x1; x++) {
                for (int32_t y = entry.y0; y <= entry.y1; y++) {
                    auto it = cells.find(cellKey(x, y));
                    if (it == cells.end()) {
                        continue;
                    }
                    drop(it->second);
                    if (it->second.empty()) {
                        cells.erase(it);
                    }
                }
            }
        }
        entry.placed = false;
    }

    void CollisionHash::place(uint32_t slot, const Vector& min, const Vector& max, bool bounded)
    {
        Entry& entry = entries[slot];
        if (entry.placed && entry.bounded == bounded && (!bounded || (entry.min == min && entry.max == max))) {
            return;
        }
        erase(slot);
        entry.min = min;
        entry.max = max;
        entry.bounded = bounded;
        entry.always = !bounded;
        if (bounded) {
            entry.x0 = cellOf(min.x);
            entry.y0 = cellOf(min.y);
            entry.x1 = cellOf(max.x);
            entry.y1 = cellOf(max.y);
            int64_t count = (static_cast<int64_t>(entry.x1) - entry.x0 + 1) * (static_cast<int64_t>(entry.y1) - entry.y0 + 1);
            if (count > maxPlacedCells) {
                entry.always = true;
            }
        }
        insert(slot);
    }

    void CollisionHash::update(UObject* actor)
    {
        if (!actor) {
            return;
        }
        if (actor->destroyed || !actor->data || !actor::bCollideActors(actor)) {
            remove(actor);
            return;
        }
        uint32_t slot = 0;
        auto it = slots.find(actor);
        if (it == slots.end()) {
            if (freeSlots.empty()) {
                slot = static_cast<uint32_t>(entries.size());
                entries.emplace_back();
            } else {
                slot = freeSlots.back();
                freeSlots.pop_back();
            }
            Entry& entry = entries[slot];
            entry = Entry();
            entry.actor = actor;
            entry.order = nextOrder++;
            entry.live = true;
            slots[actor] = slot;
        } else {
            slot = it->second;
        }
        if (isBrush(actor)) {
            Vector min;
            Vector max;
            if (brushBounds(actor, min, max)) {
                place(slot, min, max, true);
            } else {
                place(slot, {}, {}, false);
            }
            return;
        }
        Vector location = actor::Location(actor);
        float radius = actor::CollisionRadius(actor);
        float height = actor::CollisionHeight(actor);
        place(slot, { location.x - radius, location.y - radius, location.z - height }, { location.x + radius, location.y + radius, location.z + height }, true);
    }

    void CollisionHash::remove(UObject* actor)
    {
        auto it = slots.find(actor);
        if (it == slots.end()) {
            return;
        }
        uint32_t slot = it->second;
        erase(slot);
        entries[slot] = Entry();
        freeSlots.push_back(slot);
        slots.erase(it);
    }

    void CollisionHash::sync(const std::vector<UObject*>& actors)
    {
        syncStamp++;
        for (size_t i = 0; i < actors.size(); i++) {
            UObject* actor = actors[i];
            if (!actor) {
                continue;
            }
            update(actor);
            auto it = slots.find(actor);
            if (it != slots.end()) {
                entries[it->second].order = i;
                entries[it->second].seen = syncStamp;
            }
        }
        std::vector<UObject*> stale;
        for (const Entry& entry : entries) {
            if (entry.live && entry.seen != syncStamp) {
                stale.push_back(entry.actor);
            }
        }
        for (UObject* actor : stale) {
            remove(actor);
        }
        nextOrder = actors.size();
    }

    const std::vector<UObject*>& CollisionHash::query(const Vector& boxMin, const Vector& boxMax)
    {
        queryStamp++;
        found.clear();
        auto consider = [&](uint32_t slot) {
            Entry& entry = entries[slot];
            if (entry.stamp == queryStamp) {
                return;
            }
            entry.stamp = queryStamp;
            if (entry.bounded && (entry.max.x < boxMin.x || entry.min.x > boxMax.x || entry.max.y < boxMin.y || entry.min.y > boxMax.y || entry.max.z < boxMin.z || entry.min.z > boxMax.z)) {
                return;
            }
            found.push_back(slot);
        };
        int32_t x0 = cellOf(boxMin.x);
        int32_t y0 = cellOf(boxMin.y);
        int32_t x1 = cellOf(boxMax.x);
        int32_t y1 = cellOf(boxMax.y);
        int64_t count = (static_cast<int64_t>(x1) - x0 + 1) * (static_cast<int64_t>(y1) - y0 + 1);
        if (count > maxQueryCells || count > static_cast<int64_t>(cells.size())) {
            for (uint32_t slot = 0; slot < entries.size(); slot++) {
                if (entries[slot].live && entries[slot].placed) {
                    consider(slot);
                }
            }
        } else {
            for (int32_t x = x0; x <= x1; x++) {
                for (int32_t y = y0; y <= y1; y++) {
                    auto it = cells.find(cellKey(x, y));
                    if (it == cells.end()) {
                        continue;
                    }
                    for (uint32_t slot : it->second) {
                        consider(slot);
                    }
                }
            }
            for (uint32_t slot : alwaysSlots) {
                consider(slot);
            }
        }
        std::sort(found.begin(), found.end(), [this](uint32_t a, uint32_t b) {
            return entries[a].order < entries[b].order;
        });
        result.clear();
        for (uint32_t slot : found) {
            result.push_back(entries[slot].actor);
        }
        return result;
    }
}
