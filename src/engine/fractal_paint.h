#pragma once

#include "core/object.h"

#include <cstdint>

namespace dx::fractalpaint {
    constexpr int32_t DeleteRange = 12;
    constexpr int32_t MaxDrops = 256;

    struct PaintState {
        int32_t lastX = 0;
        int32_t lastY = 0;
        bool lastLeft = false;
        bool lastRight = false;
        int32_t lightPinX = 0;
        int32_t lightPinY = 0;
    };

    bool paintable(UObject* texture);
    void addSpark(UObject* texture, PaintState& state, int32_t x, int32_t y);
    void closeSpark(UObject* texture);
    void deleteSparks(UObject* texture, int32_t x, int32_t y, int32_t range);
    void drawSparkLine(UObject* texture, PaintState& state, int32_t startX, int32_t startY, int32_t destX, int32_t destY);
    void addDrop(UObject* texture, int32_t x, int32_t y);
    void deleteDrops(UObject* texture, int32_t x, int32_t y, int32_t range);
    void paint(UObject* texture, PaintState& state, int32_t x, int32_t y, bool left, bool right);
}
