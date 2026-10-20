#pragma once

#include "core/math.h"
#include "engine/canvas.h"

#include <cstdint>
#include <functional>

namespace dx {
    class UObject;

    namespace extension {
        struct ViewportRequest {
            float x = 0;
            float y = 0;
            float width = 0;
            float height = 0;
            Vector location;
            Rotator rotation;
            float fov = 90;
            UObject* originActor = nullptr;
            UObject* watchActor = nullptr;
            bool showActor = false;
            bool showWeapons = false;
            bool clearZ = true;
        };

        struct ActorDrawRequest {
            UObject* actor = nullptr;
            float x = 0;
            float y = 0;
            float width = 0;
            float height = 0;
            bool clearZ = false;
            bool constrain = false;
            bool unlit = false;
            float drawScale = 1;
            float scaleGlow = 1;
            UObject* skin = nullptr;
        };

        using ViewportDrawer = std::function<void(Canvas2D& canvas, const ViewportRequest& request)>;
        using ActorDrawer = std::function<void(Canvas2D& canvas, const ActorDrawRequest& request)>;
        using Projector = std::function<bool(const Vector& location, float& pixelX, float& pixelY)>;
        using SoundPlayer = std::function<void(UObject* sound, UObject* source, float volume, float pitch)>;

        void registerNatives();
        void tick(float deltaSeconds);
        void render(Canvas2D& canvas);
        bool keyEvent(uint8_t key, uint8_t action, float delta);
        bool charEvent(char character);
        void mouseMove(float x, float y);
        bool cursorPosition(float& x, float& y);
        bool mouseButton(uint8_t key, bool pressed);
        UObject* rootWindow();
        bool wantsMouse();

        void setViewportSize(float width, float height);
        void setFrameCanvas(Canvas2D* canvas);
        void setViewportDrawer(ViewportDrawer drawer);
        void setActorDrawer(ActorDrawer drawer);
        void setProjector(Projector projector);
        void setSoundPlayer(SoundPlayer player);
        void reset();
    }
}
