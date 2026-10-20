#pragma once

#include "core/math.h"
#include "core/object.h"
#include "core/vm.h"
#include "engine/canvas.h"
#include "engine/props_extension.h"
#include "extension/extension.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace dx {
    class UTexture;
    class UFont;
}

namespace dx::extension {
    class UWindow;
    class URootWindow;
    class UGC;

    constexpr uint8_t HALIGN_Left = 0;
    constexpr uint8_t HALIGN_Center = 1;
    constexpr uint8_t HALIGN_Right = 2;
    constexpr uint8_t HALIGN_Full = 3;
    constexpr uint8_t VALIGN_Top = 0;
    constexpr uint8_t VALIGN_Center = 1;
    constexpr uint8_t VALIGN_Bottom = 2;
    constexpr uint8_t VALIGN_Full = 3;

    constexpr uint8_t DSTY_None = 0;
    constexpr uint8_t DSTY_Normal = 1;
    constexpr uint8_t DSTY_Masked = 2;
    constexpr uint8_t DSTY_Translucent = 3;
    constexpr uint8_t DSTY_Modulated = 4;

    struct State {
        float viewportWidth = 800;
        float viewportHeight = 600;
        URootWindow* root = nullptr;
        UGC* gc = nullptr;
        UGC* windowGc = nullptr;
        Canvas2D* canvas = nullptr;
        Canvas2D* frameCanvas = nullptr;
        std::array<bool, 256> keys {};
        int32_t keyboardGrabs = 0;
        double time = 0;
        ViewportDrawer viewportDrawer;
        ActorDrawer actorDrawer;
        Projector projector;
        SoundPlayer soundPlayer;
    };

    State& state();

    float virtualScale();
    float virtualHeight();
    float virtualWidth();
    float extendedVirtualWidth();

    UGC* sharedGC();
    UObject* defaultFont();
    UClass* extensionClass(const char* path);

    template <typename T>
    bool optional(Frame& stack, T& out)
    {
        if (*stack.code == vm::EX_EndFunctionParms) {
            return false;
        }
        if (*stack.code == vm::EX_Nothing) {
            stack.code++;
            return false;
        }
        stack.step(stack.object, &out);
        return true;
    }

    bool optionalBool(Frame& stack, bool& out);
    bool optionalBoolDefault(Frame& stack, bool fallback);

    LinearColor toLinear(const Color& color);
    float textureWidth(UObject* texture);
    float textureHeight(UObject* texture);
    float fontHeight(UObject* font);
    void playSound(UWindow* window, UObject* sound, float volume, float pitch);
    std::string formatNumber(const std::string& format, double value);

    void registerWindowNatives();
    void registerGCNatives();
    void registerTextWindowNatives();
    void registerContainerNatives();
    void registerListNatives();
    void registerScaleNatives();
    void registerComputerNatives();
    void registerFlagNatives();
    void registerConSysNatives();
    void registerTextParserNatives();
    void registerFactories();
    void registerConSysFactories();
}
