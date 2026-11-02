#pragma once

#include "engine/resources.h"

#include <array>
#include <vector>

namespace dx {
    namespace editorviews {
        enum class PaneKind {
            Top,
            Front,
            Side,
            Perspective,
        };

        struct PathLink {
            Vector start;
            Vector end;
            bool monster = false;
        };

        struct PaneRect {
            float x = 0;
            float y = 0;
            float width = 0;
            float height = 0;
        };

        bool monsterPath(const ReachSpec& spec);
        std::vector<PathLink> pathLinks(const ULevel& level);
        UTexture* actorIcon(UObject* actor);
        float iconSize(float textureWidth, float drawScale, float zoom);
        std::array<PaneRect, 4> paneRects(float x, float y, float width, float height, int maximized);
    }
}
