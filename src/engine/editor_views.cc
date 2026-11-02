#include "engine/editor_views.h"

#include "core/objects.h"
#include "engine/actor.h"
#include "engine/props_engine.h"

#include <algorithm>

namespace dx {
    namespace editorviews {
        bool monsterPath(const ReachSpec& spec)
        {
            return spec.collisionRadius >= 18 && spec.collisionHeight >= 39 && !(spec.reachFlags & 2);
        }

        std::vector<PathLink> pathLinks(const ULevel& level)
        {
            std::vector<PathLink> links;
            for (const ReachSpec& spec : level.reachSpecs) {
                if (!spec.start || !spec.end || spec.pruned || !spec.start->data || !spec.end->data) {
                    continue;
                }
                links.push_back({ actor::Location(spec.start), actor::Location(spec.end), monsterPath(spec) });
            }
            return links;
        }

        UTexture* actorIcon(UObject* actor)
        {
            if (!actor || !actor->data) {
                return nullptr;
            }
            UTexture* texture = cast<UTexture>(actor::Texture(actor));
            if (!texture) {
                return nullptr;
            }
            uint8_t drawType = actor::DrawType(actor);
            bool sprite = drawType == DT_Sprite || drawType == DT_SpriteAnimOnce || drawType == DT_None;
            bool meshless = drawType == DT_Mesh && !actor::Mesh(actor);
            if (sprite || meshless || actor::bHidden(actor)) {
                return texture;
            }
            return nullptr;
        }

        float iconSize(float textureWidth, float drawScale, float zoom)
        {
            return std::max(textureWidth * drawScale * zoom, 6.0f);
        }

        std::array<PaneRect, 4> paneRects(float x, float y, float width, float height, int maximized)
        {
            std::array<PaneRect, 4> rects {};
            if (maximized >= 0 && maximized < 4) {
                rects[static_cast<size_t>(maximized)] = { x, y, width, height };
                return rects;
            }
            float halfWidth = width * 0.5f;
            float halfHeight = height * 0.5f;
            for (size_t i = 0; i < 4; i++) {
                rects[i] = { x + static_cast<float>(i % 2) * halfWidth, y + static_cast<float>(i / 2) * halfHeight, halfWidth, halfHeight };
            }
            return rects;
        }
    }
}
