#pragma once

#include "engine/resources.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace dx {
    class EditorHistory {
    public:
        void begin(ULevel* level, const std::vector<UObject*>& objects, const std::string& label);
        void commit();
        void cancel();
        bool undo();
        bool redo();
        void clear();
        size_t undoCount() const;
        size_t redoCount() const;
        bool recording() const;
        std::string nextUndo() const;
        std::string nextRedo() const;

    private:
        struct NativeState {
            std::vector<Vector> vectors;
            std::vector<Vector> points;
            std::vector<BspNode> nodes;
            std::vector<BspSurf> surfs;
            std::vector<BspVert> verts;
            int32_t numSharedSides = 0;
            std::vector<ZoneProperties> zones;
            UPolys* polys = nullptr;
            std::vector<LightMapIndex> lightMaps;
            std::vector<uint8_t> lightBits;
            std::vector<BoundingBox> nodeBounds;
            std::vector<int32_t> leafHulls;
            std::vector<ConvexLeaf> leaves;
            std::vector<UObject*> lights;
            int32_t rootOutside = 0;
            int32_t linked = 0;
            BoundingBox bounds;
            Plane sphere;
            std::vector<Poly> polyList;
        };

        struct Snapshot {
            UObject* object = nullptr;
            std::shared_ptr<uint8_t> before;
            std::shared_ptr<uint8_t> after;
            std::shared_ptr<NativeState> nativeBefore;
            std::shared_ptr<NativeState> nativeAfter;
            bool destroyedBefore = false;
            bool destroyedAfter = false;
        };

        struct Transaction {
            std::string label;
            ULevel* level = nullptr;
            std::vector<UObject*> actorsBefore;
            std::vector<UObject*> actorsAfter;
            std::vector<Snapshot> snapshots;
        };

        static std::shared_ptr<uint8_t> capture(UObject* object);
        static void restore(UObject* object, const std::shared_ptr<uint8_t>& data);
        static std::shared_ptr<NativeState> captureNative(UObject* object);
        static void restoreNative(UObject* object, const std::shared_ptr<NativeState>& state);
        void apply(const Transaction& transaction, bool forward);

        std::vector<Transaction> done;
        std::vector<Transaction> undone;
        std::unique_ptr<Transaction> pending;
    };
}
