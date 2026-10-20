#include "engine/editor_history.h"

#include <algorithm>
#include <unordered_set>

namespace dx {
    std::shared_ptr<uint8_t> EditorHistory::capture(UObject* object)
    {
        if (!object || !object->cls || !object->data) {
            return {};
        }
        UClass* cls = object->cls;
        uint8_t* copy = new uint8_t[std::max<size_t>(cls->size, 1)];
        cls->initValue(copy);
        cls->copyValue(copy, object->data);
        return std::shared_ptr<uint8_t>(copy, [cls](uint8_t* memory) {
            cls->destroyValue(memory);
            delete[] memory;
        });
    }

    void EditorHistory::restore(UObject* object, const std::shared_ptr<uint8_t>& data)
    {
        if (!object || !object->cls || !object->data || !data) {
            return;
        }
        object->cls->copyValue(object->data, data.get());
    }

    std::shared_ptr<EditorHistory::NativeState> EditorHistory::captureNative(UObject* object)
    {
        if (UModel* model = cast<UModel>(object)) {
            auto state = std::make_shared<NativeState>();
            state->vectors = model->vectors;
            state->points = model->points;
            state->nodes = model->nodes;
            state->surfs = model->surfs;
            state->verts = model->verts;
            state->numSharedSides = model->numSharedSides;
            state->zones = model->zones;
            state->polys = model->polys;
            state->lightMaps = model->lightMaps;
            state->lightBits = model->lightBits;
            state->nodeBounds = model->nodeBounds;
            state->leafHulls = model->leafHulls;
            state->leaves = model->leaves;
            state->lights = model->lights;
            state->rootOutside = model->rootOutside;
            state->linked = model->linked;
            state->bounds = model->bounds;
            state->sphere = model->sphere;
            return state;
        }
        if (UPolys* polys = cast<UPolys>(object)) {
            auto state = std::make_shared<NativeState>();
            state->polyList = polys->polys;
            return state;
        }
        return {};
    }

    void EditorHistory::restoreNative(UObject* object, const std::shared_ptr<NativeState>& state)
    {
        if (!state) {
            return;
        }
        if (UModel* model = cast<UModel>(object)) {
            model->vectors = state->vectors;
            model->points = state->points;
            model->nodes = state->nodes;
            model->surfs = state->surfs;
            model->verts = state->verts;
            model->numSharedSides = state->numSharedSides;
            model->zones = state->zones;
            model->polys = state->polys;
            model->lightMaps = state->lightMaps;
            model->lightBits = state->lightBits;
            model->nodeBounds = state->nodeBounds;
            model->leafHulls = state->leafHulls;
            model->leaves = state->leaves;
            model->lights = state->lights;
            model->rootOutside = state->rootOutside;
            model->linked = state->linked;
            model->bounds = state->bounds;
            model->sphere = state->sphere;
            model->revision++;
        } else if (UPolys* polys = cast<UPolys>(object)) {
            polys->polys = state->polyList;
        }
    }

    void EditorHistory::begin(ULevel* level, const std::vector<UObject*>& objects, const std::string& label)
    {
        pending = std::make_unique<Transaction>();
        pending->label = label;
        pending->level = level;
        if (level) {
            pending->actorsBefore = level->actors;
        }
        std::unordered_set<UObject*> seen;
        for (UObject* object : objects) {
            if (!object || !seen.insert(object).second) {
                continue;
            }
            Snapshot snapshot;
            snapshot.object = object;
            snapshot.before = capture(object);
            snapshot.nativeBefore = captureNative(object);
            snapshot.destroyedBefore = object->destroyed;
            pending->snapshots.push_back(snapshot);
        }
    }

    void EditorHistory::commit()
    {
        if (!pending) {
            return;
        }
        if (pending->level) {
            pending->actorsAfter = pending->level->actors;
        }
        for (Snapshot& snapshot : pending->snapshots) {
            snapshot.after = capture(snapshot.object);
            snapshot.nativeAfter = captureNative(snapshot.object);
            snapshot.destroyedAfter = snapshot.object->destroyed;
        }
        done.push_back(std::move(*pending));
        pending.reset();
        undone.clear();
    }

    void EditorHistory::cancel()
    {
        pending.reset();
    }

    void EditorHistory::apply(const Transaction& transaction, bool forward)
    {
        if (transaction.level) {
            const std::vector<UObject*>& target = forward ? transaction.actorsAfter : transaction.actorsBefore;
            const std::vector<UObject*>& other = forward ? transaction.actorsBefore : transaction.actorsAfter;
            std::unordered_set<UObject*> kept(target.begin(), target.end());
            for (UObject* object : other) {
                if (object && !kept.count(object)) {
                    object->destroyed = true;
                }
            }
            for (UObject* object : target) {
                if (object) {
                    object->destroyed = false;
                }
            }
            transaction.level->actors = target;
        }
        for (const Snapshot& snapshot : transaction.snapshots) {
            restore(snapshot.object, forward ? snapshot.after : snapshot.before);
            restoreNative(snapshot.object, forward ? snapshot.nativeAfter : snapshot.nativeBefore);
            snapshot.object->destroyed = forward ? snapshot.destroyedAfter : snapshot.destroyedBefore;
        }
    }

    bool EditorHistory::undo()
    {
        if (done.empty()) {
            return false;
        }
        Transaction transaction = std::move(done.back());
        done.pop_back();
        apply(transaction, false);
        undone.push_back(std::move(transaction));
        return true;
    }

    bool EditorHistory::redo()
    {
        if (undone.empty()) {
            return false;
        }
        Transaction transaction = std::move(undone.back());
        undone.pop_back();
        apply(transaction, true);
        done.push_back(std::move(transaction));
        return true;
    }

    void EditorHistory::clear()
    {
        done.clear();
        undone.clear();
        pending.reset();
    }

    size_t EditorHistory::undoCount() const
    {
        return done.size();
    }

    size_t EditorHistory::redoCount() const
    {
        return undone.size();
    }

    bool EditorHistory::recording() const
    {
        return pending != nullptr;
    }

    std::string EditorHistory::nextUndo() const
    {
        return done.empty() ? std::string() : done.back().label;
    }

    std::string EditorHistory::nextRedo() const
    {
        return undone.empty() ? std::string() : undone.back().label;
    }
}
