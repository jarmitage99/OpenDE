#include "engine/surface_select.h"

#include "engine/actor.h"
#include "engine/props_engine.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace dx {
    namespace editorops {
        namespace {
            constexpr float ParallelThreshold = 0.02f;
            constexpr float PlaneThreshold = 0.10f;

            const Poly* masterPoly(const UModel& model, int32_t index)
            {
                const BspSurf& surf = model.surfs[static_cast<size_t>(index)];
                if (!surf.actor || !surf.actor->data || surf.brushPoly < 0) {
                    return nullptr;
                }
                UModel* brush = cast<UModel>(actor::Brush(surf.actor));
                if (!brush || !brush->polys || static_cast<size_t>(surf.brushPoly) >= brush->polys->polys.size()) {
                    return nullptr;
                }
                return &brush->polys->polys[static_cast<size_t>(surf.brushPoly)];
            }

            UObject* brushModel(const UModel& model, int32_t index)
            {
                UObject* owner = model.surfs[static_cast<size_t>(index)].actor;
                if (!owner || !owner->data) {
                    return nullptr;
                }
                return actor::Brush(owner);
            }

            bool coplanar(const UModel& model, const BspSurf& a, const BspSurf& b)
            {
                const Vector& normalA = model.vectors[static_cast<size_t>(a.vNormal)];
                const Vector& normalB = model.vectors[static_cast<size_t>(b.vNormal)];
                if (std::abs(normalA.dot(normalB) - 1.0f) > ParallelThreshold) {
                    return false;
                }
                Vector offset = model.points[static_cast<size_t>(b.pBase)] - model.points[static_cast<size_t>(a.pBase)];
                return std::abs(offset.dot(normalA)) <= PlaneThreshold;
            }

            bool adjacentAllowed(const UModel& model, int32_t surfIndex, Adjacent type, const std::set<int32_t>& coplanars)
            {
                float z = std::abs(model.vectors[static_cast<size_t>(model.surfs[static_cast<size_t>(surfIndex)].vNormal)].z);
                switch (type) {
                case Adjacent::All:
                    return true;
                case Adjacent::Coplanars:
                    return coplanars.count(surfIndex) != 0;
                case Adjacent::Walls:
                    return z < 0.10f;
                case Adjacent::Floors:
                    return z > 0.85f;
                case Adjacent::Slants:
                    return z >= 0.10f && z <= 0.85f;
                }
                return false;
            }

            int tagAdjacents(const UModel& model, std::set<int32_t>& selection, Adjacent type, const std::set<int32_t>& coplanars)
            {
                std::set<int32_t> points;
                for (const BspNode& node : model.nodes) {
                    if (!selection.count(node.surf)) {
                        continue;
                    }
                    for (int32_t i = 0; i < node.numVertices; i++) {
                        points.insert(model.verts[static_cast<size_t>(node.vertPool + i)].vertex);
                    }
                }
                int added = 0;
                for (const BspNode& node : model.nodes) {
                    if (selection.count(node.surf) || !adjacentAllowed(model, node.surf, type, coplanars)) {
                        continue;
                    }
                    for (int32_t i = 0; i < node.numVertices; i++) {
                        if (points.count(model.verts[static_cast<size_t>(node.vertPool + i)].vertex)) {
                            selection.insert(node.surf);
                            added++;
                            break;
                        }
                    }
                }
                return added;
            }
        }

        void selectAllSurfs(const UModel& model, std::set<int32_t>& selection)
        {
            for (size_t i = 0; i < model.surfs.size(); i++) {
                selection.insert(static_cast<int32_t>(i));
            }
        }

        void selectReverse(const UModel& model, std::set<int32_t>& selection)
        {
            std::set<int32_t> reversed;
            for (size_t i = 0; i < model.surfs.size(); i++) {
                if (!selection.count(static_cast<int32_t>(i))) {
                    reversed.insert(static_cast<int32_t>(i));
                }
            }
            selection = reversed;
        }

        void selectMatchingGroups(const UModel& model, std::set<int32_t>& selection)
        {
            std::set<Name> groups;
            for (int32_t index : selection) {
                UObject* owner = model.surfs[static_cast<size_t>(index)].actor;
                if (owner && owner->data) {
                    groups.insert(actor::Group(owner));
                }
            }
            for (size_t i = 0; i < model.surfs.size(); i++) {
                UObject* owner = model.surfs[i].actor;
                if (owner && owner->data && groups.count(actor::Group(owner))) {
                    selection.insert(static_cast<int32_t>(i));
                }
            }
        }

        void selectMatchingItems(const UModel& model, std::set<int32_t>& selection)
        {
            std::set<UObject*> brushes;
            std::set<Name> items;
            for (int32_t index : selection) {
                UObject* brush = brushModel(model, index);
                if (brush) {
                    brushes.insert(brush);
                }
                const Poly* master = masterPoly(model, index);
                if (master) {
                    items.insert(master->itemName);
                }
            }
            for (size_t i = 0; i < model.surfs.size(); i++) {
                int32_t index = static_cast<int32_t>(i);
                const Poly* master = masterPoly(model, index);
                if (master && items.count(master->itemName) && brushes.count(brushModel(model, index))) {
                    selection.insert(index);
                }
            }
        }

        void selectMatchingBrush(const UModel& model, std::set<int32_t>& selection)
        {
            std::set<UObject*> owners;
            for (int32_t index : selection) {
                UObject* owner = model.surfs[static_cast<size_t>(index)].actor;
                if (owner) {
                    owners.insert(owner);
                }
            }
            for (size_t i = 0; i < model.surfs.size(); i++) {
                if (model.surfs[i].actor && owners.count(model.surfs[i].actor)) {
                    selection.insert(static_cast<int32_t>(i));
                }
            }
        }

        void selectMatchingTexture(const UModel& model, std::set<int32_t>& selection)
        {
            std::set<UObject*> textures;
            for (int32_t index : selection) {
                textures.insert(model.surfs[static_cast<size_t>(index)].texture);
            }
            for (size_t i = 0; i < model.surfs.size(); i++) {
                if (textures.count(model.surfs[i].texture)) {
                    selection.insert(static_cast<int32_t>(i));
                }
            }
        }

        void selectAdjacent(const UModel& model, std::set<int32_t>& selection, Adjacent type)
        {
            std::set<int32_t> coplanars;
            if (type == Adjacent::Coplanars) {
                for (int32_t index : selection) {
                    for (size_t j = 0; j < model.surfs.size(); j++) {
                        if (coplanar(model, model.surfs[static_cast<size_t>(index)], model.surfs[j])) {
                            coplanars.insert(static_cast<int32_t>(j));
                        }
                    }
                }
            }
            while (tagAdjacents(model, selection, type, coplanars) > 0) {
            }
        }

        void combineWithMemory(std::set<int32_t>& selection, const std::set<int32_t>& memory, MemoryOp op)
        {
            std::set<int32_t> result;
            if (op == MemoryOp::Or) {
                std::set_union(selection.begin(), selection.end(), memory.begin(), memory.end(), std::inserter(result, result.end()));
            } else if (op == MemoryOp::And) {
                std::set_intersection(selection.begin(), selection.end(), memory.begin(), memory.end(), std::inserter(result, result.end()));
            } else {
                std::set_symmetric_difference(selection.begin(), selection.end(), memory.begin(), memory.end(), std::inserter(result, result.end()));
            }
            selection = result;
        }

        std::set<int32_t> surfsUsingTexture(const UModel& model, const UObject* texture)
        {
            std::set<int32_t> result;
            for (size_t i = 0; i < model.surfs.size(); i++) {
                if (model.surfs[i].texture == texture) {
                    result.insert(static_cast<int32_t>(i));
                }
            }
            return result;
        }

        std::string textureGroup(const UObject* texture)
        {
            std::string group;
            for (const UObject* outer = texture ? texture->outer : nullptr; outer && outer->outer; outer = outer->outer) {
                group = group.empty() ? outer->name.str() : outer->name.str() + "." + group;
            }
            return group;
        }

        std::vector<std::string> textureGroups(const std::vector<UObject*>& textures)
        {
            std::set<std::string> unique;
            for (const UObject* texture : textures) {
                unique.insert(textureGroup(texture));
            }
            return std::vector<std::string>(unique.begin(), unique.end());
        }

        std::string textureFormatName(uint8_t format)
        {
            static const char* names[] = { "P8", "RGB32", "RGB64", "DXT1", "RGB24" };
            if (format < sizeof(names) / sizeof(names[0])) {
                return names[format];
            }
            return "Unknown";
        }
    }
}
