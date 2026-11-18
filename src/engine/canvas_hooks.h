#pragma once

#include "core/math.h"
#include "core/object.h"
#include "engine/canvas.h"

#include <functional>
#include <string>
#include <vector>

namespace dx {
    struct CanvasHooks {
        Canvas2D* target = nullptr;
        std::function<void(UObject* actor, bool clearZ)> drawActor;
        std::function<void(int x, int y, int width, int height, UObject* camera, const Vector& location, const Rotator& rotation, float fov, bool clearZ)> drawPortal;
        std::function<void(const std::string&)> clipboardCopy;
        std::function<std::string()> clipboardPaste;
        std::function<std::vector<uint8_t>(int width, int height)> snapshot;
    };

    CanvasHooks& canvasHooks();
    LinearColor canvasColor(UObject* canvas);
    UTexture* snapshotTexture(UObject* outer, Name name, int width, int height);
}
