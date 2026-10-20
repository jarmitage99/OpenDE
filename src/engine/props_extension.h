#pragma once

#include "core/math.h"
#include "core/object.h"

#include <string>

namespace dx {
    namespace window {
        struct Offsets {
            size_t windowType = 0;
            size_t parentOwner = 0;
            size_t bIsVisible = 0;
            size_t bIsSensitive = 0;
            size_t bIsSelectable = 0;
            size_t bIsInitialized = 0;
            size_t bBeingDestroyed = 0;
            size_t lockCount = 0;
            size_t bDrawRawBackground = 0;
            size_t bStretchBackground = 0;
            size_t bSmoothBackground = 0;
            size_t backgroundStyle = 0;
            size_t TextColor = 0;
            size_t textPlane = 0;
            size_t normalFont = 0;
            size_t boldFont = 0;
            size_t textVSpacing = 0;
            size_t tileColor = 0;
            size_t tilePlane = 0;
            size_t bSpecialText = 0;
            size_t bTextTranslucent = 0;
            size_t X = 0;
            size_t Y = 0;
            size_t Width = 0;
            size_t Height = 0;
            size_t winHAlign = 0;
            size_t winVAlign = 0;
            size_t hMargin0 = 0;
            size_t hMargin1 = 0;
            size_t vMargin0 = 0;
            size_t vMargin1 = 0;
            size_t hardcodedWidth = 0;
            size_t hardcodedHeight = 0;
            size_t holdX = 0;
            size_t holdY = 0;
            size_t holdWidth = 0;
            size_t holdHeight = 0;
            size_t acceleratorKey = 0;
            size_t bNeedsReconfigure = 0;
            size_t bNeedsQuery = 0;
            size_t bConfigured = 0;
            size_t bLastWidthSpecified = 0;
            size_t bLastHeightSpecified = 0;
            size_t lastSpecifiedWidth = 0;
            size_t lastSpecifiedHeight = 0;
            size_t lastQueryWidth = 0;
            size_t lastQueryHeight = 0;
            size_t bTickEnabled = 0;
            size_t maxClicks = 0;
            size_t winClipRect = 0;
            size_t Background = 0;
            size_t defaultCursor = 0;
            size_t defaultCursorShadow = 0;
            size_t defaultHotX = 0;
            size_t defaultHotY = 0;
            size_t defaultCursorColor = 0;
            size_t focusSound = 0;
            size_t unfocusSound = 0;
            size_t visibleSound = 0;
            size_t invisibleSound = 0;
            size_t SoundVolume = 0;
            size_t clientObject = 0;
            size_t rowMajorIndex = 0;
            size_t colMajorIndex = 0;
            size_t att = 0;
            size_t firstTimer = 0;
            size_t freeTimer = 0;
            size_t wGc = 0;
            size_t gGc = 0;
            size_t winParent = 0;
            size_t firstChild = 0;
            size_t lastChild = 0;
            size_t prevSibling = 0;
            size_t nextSibling = 0;
        };
    }

    extern window::Offsets offsets_window;

    namespace window {
        inline uint8_t& windowType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_window.windowType);
        }

        inline UObject*& parentOwner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.parentOwner);
        }

        inline uint32_t& bIsVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bIsVisible);
        }

        inline uint32_t& bIsSensitive(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bIsSensitive);
        }

        inline uint32_t& bIsSelectable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bIsSelectable);
        }

        inline uint32_t& bIsInitialized(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bIsInitialized);
        }

        inline uint32_t& bBeingDestroyed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bBeingDestroyed);
        }

        inline int32_t& lockCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_window.lockCount);
        }

        inline uint32_t& bDrawRawBackground(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bDrawRawBackground);
        }

        inline uint32_t& bStretchBackground(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bStretchBackground);
        }

        inline uint32_t& bSmoothBackground(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bSmoothBackground);
        }

        inline uint8_t& backgroundStyle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_window.backgroundStyle);
        }

        inline Color& TextColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_window.TextColor);
        }

        inline Plane& textPlane(UObject* o)
        {
            return *reinterpret_cast<Plane*>(o->data + offsets_window.textPlane);
        }

        inline UObject*& normalFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.normalFont);
        }

        inline UObject*& boldFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.boldFont);
        }

        inline float& textVSpacing(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.textVSpacing);
        }

        inline Color& tileColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_window.tileColor);
        }

        inline Plane& tilePlane(UObject* o)
        {
            return *reinterpret_cast<Plane*>(o->data + offsets_window.tilePlane);
        }

        inline uint32_t& bSpecialText(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bSpecialText);
        }

        inline uint32_t& bTextTranslucent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bTextTranslucent);
        }

        inline float& X(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.X);
        }

        inline float& Y(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.Y);
        }

        inline float& Width(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.Width);
        }

        inline float& Height(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.Height);
        }

        inline uint8_t& winHAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_window.winHAlign);
        }

        inline uint8_t& winVAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_window.winVAlign);
        }

        inline float& hMargin0(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.hMargin0);
        }

        inline float& hMargin1(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.hMargin1);
        }

        inline float& vMargin0(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.vMargin0);
        }

        inline float& vMargin1(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.vMargin1);
        }

        inline float& hardcodedWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.hardcodedWidth);
        }

        inline float& hardcodedHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.hardcodedHeight);
        }

        inline float& holdX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.holdX);
        }

        inline float& holdY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.holdY);
        }

        inline float& holdWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.holdWidth);
        }

        inline float& holdHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.holdHeight);
        }

        inline int32_t& acceleratorKey(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_window.acceleratorKey);
        }

        inline uint32_t& bNeedsReconfigure(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bNeedsReconfigure);
        }

        inline uint32_t& bNeedsQuery(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bNeedsQuery);
        }

        inline uint32_t& bConfigured(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bConfigured);
        }

        inline uint32_t& bLastWidthSpecified(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bLastWidthSpecified);
        }

        inline uint32_t& bLastHeightSpecified(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bLastHeightSpecified);
        }

        inline float& lastSpecifiedWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.lastSpecifiedWidth);
        }

        inline float& lastSpecifiedHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.lastSpecifiedHeight);
        }

        inline float& lastQueryWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.lastQueryWidth);
        }

        inline float& lastQueryHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.lastQueryHeight);
        }

        inline uint32_t& bTickEnabled(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_window.bTickEnabled);
        }

        inline int32_t& maxClicks(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_window.maxClicks);
        }

        inline uint8_t* winClipRect(UObject* o)
        {
            return o->data + offsets_window.winClipRect;
        }

        inline UObject*& Background(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.Background);
        }

        inline UObject*& defaultCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.defaultCursor);
        }

        inline UObject*& defaultCursorShadow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.defaultCursorShadow);
        }

        inline float& defaultHotX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.defaultHotX);
        }

        inline float& defaultHotY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.defaultHotY);
        }

        inline Color& defaultCursorColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_window.defaultCursorColor);
        }

        inline UObject*& focusSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.focusSound);
        }

        inline UObject*& unfocusSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.unfocusSound);
        }

        inline UObject*& visibleSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.visibleSound);
        }

        inline UObject*& invisibleSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.invisibleSound);
        }

        inline float& SoundVolume(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_window.SoundVolume);
        }

        inline UObject*& clientObject(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.clientObject);
        }

        inline int32_t& rowMajorIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_window.rowMajorIndex);
        }

        inline int32_t& colMajorIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_window.colMajorIndex);
        }

        inline int32_t& att(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_window.att);
        }

        inline int32_t& firstTimer(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_window.firstTimer);
        }

        inline int32_t& freeTimer(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_window.freeTimer);
        }

        inline UObject*& wGc(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.wGc);
        }

        inline UObject*& gGc(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.gGc);
        }

        inline UObject*& winParent(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.winParent);
        }

        inline UObject*& firstChild(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.firstChild);
        }

        inline UObject*& lastChild(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.lastChild);
        }

        inline UObject*& prevSibling(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.prevSibling);
        }

        inline UObject*& nextSibling(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_window.nextSibling);
        }

    }

    namespace flag {
        struct Offsets {
            size_t FlagName = 0;
            size_t flagHash = 0;
            size_t FlagBase = 0;
            size_t nextFlag = 0;
            size_t flagType = 0;
            size_t expiration = 0;
        };
    }

    extern flag::Offsets offsets_flag;

    namespace flag {
        inline Name& FlagName(UObject* o)
        {
            return *reinterpret_cast<Name*>(o->data + offsets_flag.FlagName);
        }

        inline int32_t& flagHash(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_flag.flagHash);
        }

        inline UObject*& FlagBase(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_flag.FlagBase);
        }

        inline UObject*& nextFlag(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_flag.nextFlag);
        }

        inline uint8_t& flagType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_flag.flagType);
        }

        inline int32_t& expiration(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_flag.expiration);
        }

    }

    namespace gc {
        struct Offsets {
            size_t Canvas = 0;
            size_t gcClipRect = 0;
            size_t Style = 0;
            size_t bSmoothed = 0;
            size_t bDrawEnabled = 0;
            size_t bMasked = 0;
            size_t bTranslucent = 0;
            size_t bModulated = 0;
            size_t bTextTranslucent = 0;
            size_t PolyFlags = 0;
            size_t textPolyFlags = 0;
            size_t tileColor = 0;
            size_t tilePlane = 0;
            size_t TextColor = 0;
            size_t textPlane = 0;
            size_t normalFont = 0;
            size_t boldFont = 0;
            size_t underlineTexture = 0;
            size_t underlineHeight = 0;
            size_t baselineOffset = 0;
            size_t textVSpacing = 0;
            size_t HAlign = 0;
            size_t VAlign = 0;
            size_t bWordWrap = 0;
            size_t bParseMetachars = 0;
            size_t hMultiplier = 0;
            size_t vMultiplier = 0;
            size_t bFree = 0;
            size_t gcCount = 0;
            size_t gcStack = 0;
            size_t gcFree = 0;
            size_t gcOwner = 0;
        };
    }

    extern gc::Offsets offsets_gc;

    namespace gc {
        inline UObject*& Canvas(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gc.Canvas);
        }

        inline uint8_t* gcClipRect(UObject* o)
        {
            return o->data + offsets_gc.gcClipRect;
        }

        inline uint8_t& Style(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_gc.Style);
        }

        inline uint32_t& bSmoothed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bSmoothed);
        }

        inline uint32_t& bDrawEnabled(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bDrawEnabled);
        }

        inline uint32_t& bMasked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bMasked);
        }

        inline uint32_t& bTranslucent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bTranslucent);
        }

        inline uint32_t& bModulated(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bModulated);
        }

        inline uint32_t& bTextTranslucent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bTextTranslucent);
        }

        inline int32_t& PolyFlags(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gc.PolyFlags);
        }

        inline int32_t& textPolyFlags(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gc.textPolyFlags);
        }

        inline Color& tileColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_gc.tileColor);
        }

        inline Plane& tilePlane(UObject* o)
        {
            return *reinterpret_cast<Plane*>(o->data + offsets_gc.tilePlane);
        }

        inline Color& TextColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_gc.TextColor);
        }

        inline Plane& textPlane(UObject* o)
        {
            return *reinterpret_cast<Plane*>(o->data + offsets_gc.textPlane);
        }

        inline UObject*& normalFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gc.normalFont);
        }

        inline UObject*& boldFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gc.boldFont);
        }

        inline UObject*& underlineTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gc.underlineTexture);
        }

        inline float& underlineHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_gc.underlineHeight);
        }

        inline float& baselineOffset(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_gc.baselineOffset);
        }

        inline float& textVSpacing(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_gc.textVSpacing);
        }

        inline uint8_t& HAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_gc.HAlign);
        }

        inline uint8_t& VAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_gc.VAlign);
        }

        inline uint32_t& bWordWrap(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bWordWrap);
        }

        inline uint32_t& bParseMetachars(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bParseMetachars);
        }

        inline int32_t& hMultiplier(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gc.hMultiplier);
        }

        inline int32_t& vMultiplier(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gc.vMultiplier);
        }

        inline uint32_t& bFree(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_gc.bFree);
        }

        inline int32_t& gcCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_gc.gcCount);
        }

        inline UObject*& gcStack(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gc.gcStack);
        }

        inline UObject*& gcFree(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gc.gcFree);
        }

        inline UObject*& gcOwner(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_gc.gcOwner);
        }

    }

    namespace buttonwindow {
        struct Offsets {
            size_t bButtonPressed = 0;
            size_t bMousePressed = 0;
            size_t bAutoRepeat = 0;
            size_t bEnableRightMouseClick = 0;
            size_t activateDelay = 0;
            size_t initialDelay = 0;
            size_t repeatRate = 0;
            size_t curTexture = 0;
            size_t curTileColor = 0;
            size_t curTextColor = 0;
            size_t pressSound = 0;
            size_t clickSound = 0;
            size_t activateTimer = 0;
            size_t repeatTime = 0;
            size_t lastInputKey = 0;
            size_t Info = 0;
        };
    }

    extern buttonwindow::Offsets offsets_buttonwindow;

    namespace buttonwindow {
        inline uint32_t& bButtonPressed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_buttonwindow.bButtonPressed);
        }

        inline uint32_t& bMousePressed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_buttonwindow.bMousePressed);
        }

        inline uint32_t& bAutoRepeat(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_buttonwindow.bAutoRepeat);
        }

        inline uint32_t& bEnableRightMouseClick(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_buttonwindow.bEnableRightMouseClick);
        }

        inline float& activateDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_buttonwindow.activateDelay);
        }

        inline float& initialDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_buttonwindow.initialDelay);
        }

        inline float& repeatRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_buttonwindow.repeatRate);
        }

        inline UObject*& curTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_buttonwindow.curTexture);
        }

        inline Color& curTileColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_buttonwindow.curTileColor);
        }

        inline Color& curTextColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_buttonwindow.curTextColor);
        }

        inline UObject*& pressSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_buttonwindow.pressSound);
        }

        inline UObject*& clickSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_buttonwindow.clickSound);
        }

        inline int32_t& activateTimer(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_buttonwindow.activateTimer);
        }

        inline float& repeatTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_buttonwindow.repeatTime);
        }

        inline uint8_t& lastInputKey(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_buttonwindow.lastInputKey);
        }

        inline uint8_t* Info(UObject* o)
        {
            return o->data + offsets_buttonwindow.Info;
        }

    }

    namespace extensionobject {
        struct Offsets {
        };
    }

    extern extensionobject::Offsets offsets_extensionobject;

    namespace extensionobject {

    }

    namespace textwindow {
        struct Offsets {
            size_t HAlign = 0;
            size_t VAlign = 0;
            size_t bWordWrap = 0;
            size_t bTextIsAccelerator = 0;
            size_t hMargin = 0;
            size_t vMargin = 0;
            size_t minLines = 0;
            size_t MaxLines = 0;
            size_t MinWidth = 0;
            size_t Text = 0;
        };
    }

    extern textwindow::Offsets offsets_textwindow;

    namespace textwindow {
        inline uint8_t& HAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_textwindow.HAlign);
        }

        inline uint8_t& VAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_textwindow.VAlign);
        }

        inline uint32_t& bWordWrap(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_textwindow.bWordWrap);
        }

        inline uint32_t& bTextIsAccelerator(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_textwindow.bTextIsAccelerator);
        }

        inline float& hMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_textwindow.hMargin);
        }

        inline float& vMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_textwindow.vMargin);
        }

        inline int32_t& minLines(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_textwindow.minLines);
        }

        inline int32_t& MaxLines(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_textwindow.MaxLines);
        }

        inline float& MinWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_textwindow.MinWidth);
        }

        inline std::string& Text(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_textwindow.Text);
        }

    }

    namespace tabgroupwindow {
        struct Offsets {
            size_t rowMajorWindowList = 0;
            size_t colMajorWindowList = 0;
            size_t bSizeParentToChildren = 0;
            size_t bSizeChildrenToParent = 0;
            size_t tabGroupIndex = 0;
            size_t firstAbsX = 0;
            size_t firstAbsY = 0;
        };
    }

    extern tabgroupwindow::Offsets offsets_tabgroupwindow;

    namespace tabgroupwindow {
        inline uint8_t* rowMajorWindowList(UObject* o)
        {
            return o->data + offsets_tabgroupwindow.rowMajorWindowList;
        }

        inline uint8_t* colMajorWindowList(UObject* o)
        {
            return o->data + offsets_tabgroupwindow.colMajorWindowList;
        }

        inline uint32_t& bSizeParentToChildren(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_tabgroupwindow.bSizeParentToChildren);
        }

        inline uint32_t& bSizeChildrenToParent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_tabgroupwindow.bSizeChildrenToParent);
        }

        inline int32_t& tabGroupIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_tabgroupwindow.tabGroupIndex);
        }

        inline float& firstAbsX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_tabgroupwindow.firstAbsX);
        }

        inline float& firstAbsY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_tabgroupwindow.firstAbsY);
        }

    }

    namespace scalewindow {
        struct Offsets {
            size_t orientation = 0;
            size_t scaleTexture = 0;
            size_t thumbTexture = 0;
            size_t tickTexture = 0;
            size_t preCapTexture = 0;
            size_t postCapTexture = 0;
            size_t bRepeatScaleTexture = 0;
            size_t bRepeatThumbTexture = 0;
            size_t bDrawEndTicks = 0;
            size_t bStretchScale = 0;
            size_t bSpanThumb = 0;
            size_t borderPattern = 0;
            size_t scaleBorderSize = 0;
            size_t thumbBorderSize = 0;
            size_t scaleBorderColor = 0;
            size_t thumbBorderColor = 0;
            size_t scaleStyle = 0;
            size_t thumbStyle = 0;
            size_t tickStyle = 0;
            size_t scaleColor = 0;
            size_t thumbColor = 0;
            size_t tickColor = 0;
            size_t scaleWidth = 0;
            size_t scaleHeight = 0;
            size_t ThumbWidth = 0;
            size_t ThumbHeight = 0;
            size_t tickWidth = 0;
            size_t tickHeight = 0;
            size_t preCapWidth = 0;
            size_t preCapHeight = 0;
            size_t postCapWidth = 0;
            size_t postCapHeight = 0;
            size_t startOffset = 0;
            size_t endOffset = 0;
            size_t marginWidth = 0;
            size_t marginHeight = 0;
            size_t numPositions = 0;
            size_t currentPos = 0;
            size_t spanRange = 0;
            size_t thumbStep = 0;
            size_t fromValue = 0;
            size_t toValue = 0;
            size_t valueFmt = 0;
            size_t initialDelay = 0;
            size_t repeatRate = 0;
            size_t initialPos = 0;
            size_t setSound = 0;
            size_t clickSound = 0;
            size_t dragSound = 0;
            size_t scaleX = 0;
            size_t scaleY = 0;
            size_t scaleW = 0;
            size_t scaleH = 0;
            size_t thumbX = 0;
            size_t thumbY = 0;
            size_t thumbW = 0;
            size_t thumbH = 0;
            size_t tickX = 0;
            size_t tickY = 0;
            size_t tickW = 0;
            size_t tickH = 0;
            size_t preCapXOff = 0;
            size_t preCapYOff = 0;
            size_t preCapW = 0;
            size_t preCapH = 0;
            size_t postCapXOff = 0;
            size_t postCapYOff = 0;
            size_t postCapW = 0;
            size_t postCapH = 0;
            size_t absStartScale = 0;
            size_t absEndScale = 0;
            size_t bDraggingThumb = 0;
            size_t mousePos = 0;
            size_t repeatDir = 0;
            size_t RemainingTime = 0;
            size_t enumStrings = 0;
        };
    }

    extern scalewindow::Offsets offsets_scalewindow;

    namespace scalewindow {
        inline uint8_t& orientation(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scalewindow.orientation);
        }

        inline UObject*& scaleTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.scaleTexture);
        }

        inline UObject*& thumbTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.thumbTexture);
        }

        inline UObject*& tickTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.tickTexture);
        }

        inline UObject*& preCapTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.preCapTexture);
        }

        inline UObject*& postCapTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.postCapTexture);
        }

        inline uint32_t& bRepeatScaleTexture(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scalewindow.bRepeatScaleTexture);
        }

        inline uint32_t& bRepeatThumbTexture(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scalewindow.bRepeatThumbTexture);
        }

        inline uint32_t& bDrawEndTicks(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scalewindow.bDrawEndTicks);
        }

        inline uint32_t& bStretchScale(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scalewindow.bStretchScale);
        }

        inline uint32_t& bSpanThumb(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scalewindow.bSpanThumb);
        }

        inline UObject*& borderPattern(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.borderPattern);
        }

        inline float& scaleBorderSize(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.scaleBorderSize);
        }

        inline float& thumbBorderSize(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.thumbBorderSize);
        }

        inline Color& scaleBorderColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_scalewindow.scaleBorderColor);
        }

        inline Color& thumbBorderColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_scalewindow.thumbBorderColor);
        }

        inline uint8_t& scaleStyle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scalewindow.scaleStyle);
        }

        inline uint8_t& thumbStyle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scalewindow.thumbStyle);
        }

        inline uint8_t& tickStyle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scalewindow.tickStyle);
        }

        inline Color& scaleColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_scalewindow.scaleColor);
        }

        inline Color& thumbColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_scalewindow.thumbColor);
        }

        inline Color& tickColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_scalewindow.tickColor);
        }

        inline float& scaleWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.scaleWidth);
        }

        inline float& scaleHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.scaleHeight);
        }

        inline float& ThumbWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.ThumbWidth);
        }

        inline float& ThumbHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.ThumbHeight);
        }

        inline float& tickWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.tickWidth);
        }

        inline float& tickHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.tickHeight);
        }

        inline float& preCapWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.preCapWidth);
        }

        inline float& preCapHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.preCapHeight);
        }

        inline float& postCapWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.postCapWidth);
        }

        inline float& postCapHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.postCapHeight);
        }

        inline float& startOffset(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.startOffset);
        }

        inline float& endOffset(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.endOffset);
        }

        inline float& marginWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.marginWidth);
        }

        inline float& marginHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.marginHeight);
        }

        inline int32_t& numPositions(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scalewindow.numPositions);
        }

        inline int32_t& currentPos(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scalewindow.currentPos);
        }

        inline int32_t& spanRange(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scalewindow.spanRange);
        }

        inline int32_t& thumbStep(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scalewindow.thumbStep);
        }

        inline float& fromValue(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.fromValue);
        }

        inline float& toValue(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.toValue);
        }

        inline std::string& valueFmt(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_scalewindow.valueFmt);
        }

        inline float& initialDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.initialDelay);
        }

        inline float& repeatRate(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.repeatRate);
        }

        inline int32_t& initialPos(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_scalewindow.initialPos);
        }

        inline UObject*& setSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.setSound);
        }

        inline UObject*& clickSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.clickSound);
        }

        inline UObject*& dragSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalewindow.dragSound);
        }

        inline float& scaleX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.scaleX);
        }

        inline float& scaleY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.scaleY);
        }

        inline float& scaleW(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.scaleW);
        }

        inline float& scaleH(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.scaleH);
        }

        inline float& thumbX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.thumbX);
        }

        inline float& thumbY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.thumbY);
        }

        inline float& thumbW(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.thumbW);
        }

        inline float& thumbH(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.thumbH);
        }

        inline float& tickX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.tickX);
        }

        inline float& tickY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.tickY);
        }

        inline float& tickW(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.tickW);
        }

        inline float& tickH(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.tickH);
        }

        inline float& preCapXOff(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.preCapXOff);
        }

        inline float& preCapYOff(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.preCapYOff);
        }

        inline float& preCapW(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.preCapW);
        }

        inline float& preCapH(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.preCapH);
        }

        inline float& postCapXOff(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.postCapXOff);
        }

        inline float& postCapYOff(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.postCapYOff);
        }

        inline float& postCapW(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.postCapW);
        }

        inline float& postCapH(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.postCapH);
        }

        inline float& absStartScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.absStartScale);
        }

        inline float& absEndScale(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.absEndScale);
        }

        inline uint32_t& bDraggingThumb(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scalewindow.bDraggingThumb);
        }

        inline float& mousePos(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.mousePos);
        }

        inline uint8_t& repeatDir(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scalewindow.repeatDir);
        }

        inline float& RemainingTime(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalewindow.RemainingTime);
        }

        inline uint8_t* enumStrings(UObject* o)
        {
            return o->data + offsets_scalewindow.enumStrings;
        }

    }

    namespace rootwindow {
        struct Offsets {
            size_t parentPawn = 0;
            size_t nextRootWindow = 0;
            size_t MouseX = 0;
            size_t MouseY = 0;
            size_t prevMouseX = 0;
            size_t prevMouseY = 0;
            size_t lastMouseWindow = 0;
            size_t bMouseMoved = 0;
            size_t bMouseMoveLocked = 0;
            size_t bMouseButtonLocked = 0;
            size_t bCursorVisible = 0;
            size_t defaultEditCursor = 0;
            size_t DefaultMoveCursor = 0;
            size_t defaultHorizontalMoveCursor = 0;
            size_t defaultVerticalMoveCursor = 0;
            size_t defaultTopLeftMoveCursor = 0;
            size_t defaultTopRightMoveCursor = 0;
            size_t bPositionalSound = 0;
            size_t grabbedWindow = 0;
            size_t FocusWindow = 0;
            size_t handleMouseRef = 0;
            size_t handleKeyboardRef = 0;
            size_t initCount = 0;
            size_t bRender = 0;
            size_t bClipRender = 0;
            size_t bStretchRawBackground = 0;
            size_t renderX = 0;
            size_t renderY = 0;
            size_t renderWidth = 0;
            size_t renderHeight = 0;
            size_t rawBackground = 0;
            size_t rawBackgroundWidth = 0;
            size_t rawBackgroundHeight = 0;
            size_t rawColor = 0;
            size_t TickCycles = 0;
            size_t paintCycles = 0;
            size_t bShowStats = 0;
            size_t bShowFrames = 0;
            size_t debugTexture = 0;
            size_t frameTimer = 0;
            size_t multiClickTimeout = 0;
            size_t maxMouseDist = 0;
            size_t clickCount = 0;
            size_t lastButtonType = 0;
            size_t lastButtonPress = 0;
            size_t lastButtonWindow = 0;
            size_t firstButtonMouseX = 0;
            size_t firstButtonMouseY = 0;
            size_t keyDownMap = 0;
            size_t hMultiplier = 0;
            size_t vMultiplier = 0;
            size_t snapshotWidth = 0;
            size_t snapshotHeight = 0;
            size_t rootFrame = 0;
        };
    }

    extern rootwindow::Offsets offsets_rootwindow;

    namespace rootwindow {
        inline UObject*& parentPawn(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.parentPawn);
        }

        inline UObject*& nextRootWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.nextRootWindow);
        }

        inline float& MouseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.MouseX);
        }

        inline float& MouseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.MouseY);
        }

        inline float& prevMouseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.prevMouseX);
        }

        inline float& prevMouseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.prevMouseY);
        }

        inline UObject*& lastMouseWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.lastMouseWindow);
        }

        inline uint32_t& bMouseMoved(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bMouseMoved);
        }

        inline uint32_t& bMouseMoveLocked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bMouseMoveLocked);
        }

        inline uint32_t& bMouseButtonLocked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bMouseButtonLocked);
        }

        inline uint32_t& bCursorVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bCursorVisible);
        }

        inline UObject*& defaultEditCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.defaultEditCursor);
        }

        inline UObject*& DefaultMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.DefaultMoveCursor);
        }

        inline UObject*& defaultHorizontalMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.defaultHorizontalMoveCursor);
        }

        inline UObject*& defaultVerticalMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.defaultVerticalMoveCursor);
        }

        inline UObject*& defaultTopLeftMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.defaultTopLeftMoveCursor);
        }

        inline UObject*& defaultTopRightMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.defaultTopRightMoveCursor);
        }

        inline uint32_t& bPositionalSound(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bPositionalSound);
        }

        inline UObject*& grabbedWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.grabbedWindow);
        }

        inline UObject*& FocusWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.FocusWindow);
        }

        inline int32_t& handleMouseRef(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.handleMouseRef);
        }

        inline int32_t& handleKeyboardRef(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.handleKeyboardRef);
        }

        inline int32_t& initCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.initCount);
        }

        inline uint32_t& bRender(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bRender);
        }

        inline uint32_t& bClipRender(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bClipRender);
        }

        inline uint32_t& bStretchRawBackground(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bStretchRawBackground);
        }

        inline float& renderX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.renderX);
        }

        inline float& renderY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.renderY);
        }

        inline float& renderWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.renderWidth);
        }

        inline float& renderHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.renderHeight);
        }

        inline UObject*& rawBackground(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.rawBackground);
        }

        inline float& rawBackgroundWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.rawBackgroundWidth);
        }

        inline float& rawBackgroundHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.rawBackgroundHeight);
        }

        inline Color& rawColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_rootwindow.rawColor);
        }

        inline int32_t& TickCycles(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.TickCycles);
        }

        inline int32_t& paintCycles(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.paintCycles);
        }

        inline uint32_t& bShowStats(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bShowStats);
        }

        inline uint32_t& bShowFrames(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_rootwindow.bShowFrames);
        }

        inline UObject*& debugTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.debugTexture);
        }

        inline float& frameTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.frameTimer);
        }

        inline float& multiClickTimeout(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.multiClickTimeout);
        }

        inline float& maxMouseDist(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.maxMouseDist);
        }

        inline int32_t& clickCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.clickCount);
        }

        inline int32_t& lastButtonType(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.lastButtonType);
        }

        inline float& lastButtonPress(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.lastButtonPress);
        }

        inline UObject*& lastButtonWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_rootwindow.lastButtonWindow);
        }

        inline float& firstButtonMouseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.firstButtonMouseX);
        }

        inline float& firstButtonMouseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_rootwindow.firstButtonMouseY);
        }

        inline uint8_t& keyDownMap(UObject* o, int index)
        {
            return reinterpret_cast<uint8_t*>(o->data + offsets_rootwindow.keyDownMap)[index];
        }

        inline int32_t& hMultiplier(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.hMultiplier);
        }

        inline int32_t& vMultiplier(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.vMultiplier);
        }

        inline int32_t& snapshotWidth(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.snapshotWidth);
        }

        inline int32_t& snapshotHeight(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.snapshotHeight);
        }

        inline int32_t& rootFrame(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_rootwindow.rootFrame);
        }

    }

    namespace togglewindow {
        struct Offsets {
            size_t enableSound = 0;
            size_t disableSound = 0;
        };
    }

    extern togglewindow::Offsets offsets_togglewindow;

    namespace togglewindow {
        inline UObject*& enableSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_togglewindow.enableSound);
        }

        inline UObject*& disableSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_togglewindow.disableSound);
        }

    }

    namespace modalwindow {
        struct Offsets {
            size_t focusMode = 0;
            size_t preferredFocus = 0;
            size_t tabGroupWindowList = 0;
            size_t acceleratorTable = 0;
            size_t bDirtyAccelerators = 0;
        };
    }

    extern modalwindow::Offsets offsets_modalwindow;

    namespace modalwindow {
        inline uint8_t& focusMode(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_modalwindow.focusMode);
        }

        inline UObject*& preferredFocus(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_modalwindow.preferredFocus);
        }

        inline uint8_t* tabGroupWindowList(UObject* o)
        {
            return o->data + offsets_modalwindow.tabGroupWindowList;
        }

        inline UObject*& acceleratorTable(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_modalwindow.acceleratorTable)[index];
        }

        inline uint32_t& bDirtyAccelerators(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_modalwindow.bDirtyAccelerators);
        }

    }

    namespace playerpawnext {
        struct Offsets {
            size_t FlagBase = 0;
            size_t RootWindow = 0;
            size_t actorList = 0;
            size_t actorCount = 0;
        };
    }

    extern playerpawnext::Offsets offsets_playerpawnext;

    namespace playerpawnext {
        inline UObject*& FlagBase(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawnext.FlagBase);
        }

        inline UObject*& RootWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_playerpawnext.RootWindow);
        }

        inline uint8_t* actorList(UObject* o)
        {
            return o->data + offsets_playerpawnext.actorList;
        }

        inline int32_t& actorCount(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_playerpawnext.actorCount);
        }

    }

    namespace flagbase {
        struct Offsets {
            size_t hashTable = 0;
            size_t defaultFlagExpiration = 0;
        };
    }

    extern flagbase::Offsets offsets_flagbase;

    namespace flagbase {
        inline UObject*& hashTable(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_flagbase.hashTable)[index];
        }

        inline int32_t& defaultFlagExpiration(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_flagbase.defaultFlagExpiration);
        }

    }

    namespace largetextwindow {
        struct Offsets {
            size_t vSpace = 0;
            size_t lineHeight = 0;
            size_t textParams = 0;
            size_t rowData = 0;
            size_t queryTextParams = 0;
            size_t queryRowData = 0;
            size_t tempRowData = 0;
        };
    }

    extern largetextwindow::Offsets offsets_largetextwindow;

    namespace largetextwindow {
        inline float& vSpace(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_largetextwindow.vSpace);
        }

        inline float& lineHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_largetextwindow.lineHeight);
        }

        inline uint8_t* textParams(UObject* o)
        {
            return o->data + offsets_largetextwindow.textParams;
        }

        inline uint8_t* rowData(UObject* o)
        {
            return o->data + offsets_largetextwindow.rowData;
        }

        inline uint8_t* queryTextParams(UObject* o)
        {
            return o->data + offsets_largetextwindow.queryTextParams;
        }

        inline uint8_t* queryRowData(UObject* o)
        {
            return o->data + offsets_largetextwindow.queryRowData;
        }

        inline uint8_t* tempRowData(UObject* o)
        {
            return o->data + offsets_largetextwindow.tempRowData;
        }

    }

    namespace scalemanagerwindow {
        struct Offsets {
            size_t decButton = 0;
            size_t incButton = 0;
            size_t valueField = 0;
            size_t Scale = 0;
            size_t orientation = 0;
            size_t bStretchScaleField = 0;
            size_t bStretchValueField = 0;
            size_t marginWidth = 0;
            size_t marginHeight = 0;
            size_t Spacing = 0;
            size_t childHAlign = 0;
            size_t childVAlign = 0;
        };
    }

    extern scalemanagerwindow::Offsets offsets_scalemanagerwindow;

    namespace scalemanagerwindow {
        inline UObject*& decButton(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalemanagerwindow.decButton);
        }

        inline UObject*& incButton(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalemanagerwindow.incButton);
        }

        inline UObject*& valueField(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalemanagerwindow.valueField);
        }

        inline UObject*& Scale(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scalemanagerwindow.Scale);
        }

        inline uint8_t& orientation(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scalemanagerwindow.orientation);
        }

        inline uint32_t& bStretchScaleField(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scalemanagerwindow.bStretchScaleField);
        }

        inline uint32_t& bStretchValueField(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scalemanagerwindow.bStretchValueField);
        }

        inline float& marginWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalemanagerwindow.marginWidth);
        }

        inline float& marginHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalemanagerwindow.marginHeight);
        }

        inline float& Spacing(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scalemanagerwindow.Spacing);
        }

        inline uint8_t& childHAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scalemanagerwindow.childHAlign);
        }

        inline uint8_t& childVAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_scalemanagerwindow.childVAlign);
        }

    }

    namespace editwindow {
        struct Offsets {
            size_t bEditable = 0;
            size_t bSingleLine = 0;
            size_t bUppercaseOnly = 0;
            size_t insertPos = 0;
            size_t insertHookPos = 0;
            size_t insertType = 0;
            size_t selectStart = 0;
            size_t selectEnd = 0;
            size_t maxSize = 0;
            size_t insertTexture = 0;
            size_t insertColor = 0;
            size_t selectTexture = 0;
            size_t selectColor = 0;
            size_t inverseColor = 0;
            size_t editCursor = 0;
            size_t editCursorShadow = 0;
            size_t editCursorColor = 0;
            size_t typeSound = 0;
            size_t deleteSound = 0;
            size_t enterSound = 0;
            size_t moveSound = 0;
            size_t bufferList = 0;
            size_t currentUndo = 0;
            size_t maxUndos = 0;
            size_t unchangedUndo = 0;
            size_t dragDelay = 0;
            size_t blinkDelay = 0;
            size_t blinkStart = 0;
            size_t blinkPeriod = 0;
            size_t bCursorShowing = 0;
            size_t bDragging = 0;
            size_t bSelectWords = 0;
            size_t insertX = 0;
            size_t insertY = 0;
            size_t insertWidth = 0;
            size_t insertHeight = 0;
            size_t insertPrefWidth = 0;
            size_t insertPrefHeight = 0;
            size_t showAreaX = 0;
            size_t showAreaY = 0;
            size_t showAreaWidth = 0;
            size_t showAreaHeight = 0;
            size_t insertPreferredCol = 0;
            size_t lastConfigWidth = 0;
            size_t lastConfigHeight = 0;
            size_t selectStartRow = 0;
            size_t selectEndRow = 0;
            size_t selectStartX = 0;
            size_t selectEndX = 0;
        };
    }

    extern editwindow::Offsets offsets_editwindow;

    namespace editwindow {
        inline uint32_t& bEditable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_editwindow.bEditable);
        }

        inline uint32_t& bSingleLine(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_editwindow.bSingleLine);
        }

        inline uint32_t& bUppercaseOnly(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_editwindow.bUppercaseOnly);
        }

        inline int32_t& insertPos(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.insertPos);
        }

        inline int32_t& insertHookPos(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.insertHookPos);
        }

        inline uint8_t& insertType(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_editwindow.insertType);
        }

        inline int32_t& selectStart(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.selectStart);
        }

        inline int32_t& selectEnd(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.selectEnd);
        }

        inline int32_t& maxSize(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.maxSize);
        }

        inline UObject*& insertTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_editwindow.insertTexture);
        }

        inline Color& insertColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_editwindow.insertColor);
        }

        inline UObject*& selectTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_editwindow.selectTexture);
        }

        inline Color& selectColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_editwindow.selectColor);
        }

        inline Color& inverseColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_editwindow.inverseColor);
        }

        inline UObject*& editCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_editwindow.editCursor);
        }

        inline UObject*& editCursorShadow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_editwindow.editCursorShadow);
        }

        inline Color& editCursorColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_editwindow.editCursorColor);
        }

        inline UObject*& typeSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_editwindow.typeSound);
        }

        inline UObject*& deleteSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_editwindow.deleteSound);
        }

        inline UObject*& enterSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_editwindow.enterSound);
        }

        inline UObject*& moveSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_editwindow.moveSound);
        }

        inline uint8_t* bufferList(UObject* o)
        {
            return o->data + offsets_editwindow.bufferList;
        }

        inline int32_t& currentUndo(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.currentUndo);
        }

        inline int32_t& maxUndos(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.maxUndos);
        }

        inline int32_t& unchangedUndo(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.unchangedUndo);
        }

        inline float& dragDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.dragDelay);
        }

        inline float& blinkDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.blinkDelay);
        }

        inline float& blinkStart(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.blinkStart);
        }

        inline float& blinkPeriod(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.blinkPeriod);
        }

        inline uint32_t& bCursorShowing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_editwindow.bCursorShowing);
        }

        inline uint32_t& bDragging(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_editwindow.bDragging);
        }

        inline uint32_t& bSelectWords(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_editwindow.bSelectWords);
        }

        inline float& insertX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.insertX);
        }

        inline float& insertY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.insertY);
        }

        inline float& insertWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.insertWidth);
        }

        inline float& insertHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.insertHeight);
        }

        inline float& insertPrefWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.insertPrefWidth);
        }

        inline float& insertPrefHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.insertPrefHeight);
        }

        inline float& showAreaX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.showAreaX);
        }

        inline float& showAreaY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.showAreaY);
        }

        inline float& showAreaWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.showAreaWidth);
        }

        inline float& showAreaHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.showAreaHeight);
        }

        inline float& insertPreferredCol(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.insertPreferredCol);
        }

        inline float& lastConfigWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.lastConfigWidth);
        }

        inline float& lastConfigHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.lastConfigHeight);
        }

        inline int32_t& selectStartRow(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.selectStartRow);
        }

        inline int32_t& selectEndRow(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_editwindow.selectEndRow);
        }

        inline float& selectStartX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.selectStartX);
        }

        inline float& selectEndX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_editwindow.selectEndX);
        }

    }

    namespace clipwindow {
        struct Offsets {
            size_t childH = 0;
            size_t childV = 0;
            size_t prefHUnits = 0;
            size_t prefVUnits = 0;
            size_t bForceChildWidth = 0;
            size_t bForceChildHeight = 0;
            size_t bSnapToUnits = 0;
            size_t bFillWindow = 0;
            size_t areaHSize = 0;
            size_t areaVSize = 0;
            size_t childHSize = 0;
            size_t childVSize = 0;
            size_t hMult = 0;
            size_t vMult = 0;
        };
    }

    extern clipwindow::Offsets offsets_clipwindow;

    namespace clipwindow {
        inline int32_t& childH(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_clipwindow.childH);
        }

        inline int32_t& childV(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_clipwindow.childV);
        }

        inline int32_t& prefHUnits(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_clipwindow.prefHUnits);
        }

        inline int32_t& prefVUnits(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_clipwindow.prefVUnits);
        }

        inline uint32_t& bForceChildWidth(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_clipwindow.bForceChildWidth);
        }

        inline uint32_t& bForceChildHeight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_clipwindow.bForceChildHeight);
        }

        inline uint32_t& bSnapToUnits(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_clipwindow.bSnapToUnits);
        }

        inline uint32_t& bFillWindow(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_clipwindow.bFillWindow);
        }

        inline int32_t& areaHSize(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_clipwindow.areaHSize);
        }

        inline int32_t& areaVSize(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_clipwindow.areaVSize);
        }

        inline int32_t& childHSize(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_clipwindow.childHSize);
        }

        inline int32_t& childVSize(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_clipwindow.childVSize);
        }

        inline float& hMult(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_clipwindow.hMult);
        }

        inline float& vMult(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_clipwindow.vMult);
        }

    }

    namespace borderwindow {
        struct Offsets {
            size_t borderLeft = 0;
            size_t borderTopLeft = 0;
            size_t borderTop = 0;
            size_t borderTopRight = 0;
            size_t borderRight = 0;
            size_t borderBottomRight = 0;
            size_t borderBottom = 0;
            size_t borderBottomLeft = 0;
            size_t center = 0;
            size_t MoveCursor = 0;
            size_t hMoveCursor = 0;
            size_t vMoveCursor = 0;
            size_t tlMoveCursor = 0;
            size_t trMoveCursor = 0;
            size_t borderStyle = 0;
            size_t bSmoothBorder = 0;
            size_t bStretchBorder = 0;
            size_t bResizeable = 0;
            size_t bMarginsFromBorder = 0;
            size_t childLeftMargin = 0;
            size_t childRightMargin = 0;
            size_t childTopMargin = 0;
            size_t childBottomMargin = 0;
            size_t leftMargin = 0;
            size_t rightMargin = 0;
            size_t TopMargin = 0;
            size_t BottomMargin = 0;
            size_t bLeftDrag = 0;
            size_t bRightDrag = 0;
            size_t bUpDrag = 0;
            size_t bDownDrag = 0;
            size_t lastMouseX = 0;
            size_t lastMouseY = 0;
            size_t DragX = 0;
            size_t DragY = 0;
            size_t dragWidth = 0;
            size_t dragHeight = 0;
        };
    }

    extern borderwindow::Offsets offsets_borderwindow;

    namespace borderwindow {
        inline UObject*& borderLeft(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.borderLeft);
        }

        inline UObject*& borderTopLeft(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.borderTopLeft);
        }

        inline UObject*& borderTop(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.borderTop);
        }

        inline UObject*& borderTopRight(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.borderTopRight);
        }

        inline UObject*& borderRight(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.borderRight);
        }

        inline UObject*& borderBottomRight(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.borderBottomRight);
        }

        inline UObject*& borderBottom(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.borderBottom);
        }

        inline UObject*& borderBottomLeft(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.borderBottomLeft);
        }

        inline UObject*& center(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.center);
        }

        inline UObject*& MoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.MoveCursor);
        }

        inline UObject*& hMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.hMoveCursor);
        }

        inline UObject*& vMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.vMoveCursor);
        }

        inline UObject*& tlMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.tlMoveCursor);
        }

        inline UObject*& trMoveCursor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_borderwindow.trMoveCursor);
        }

        inline uint8_t& borderStyle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_borderwindow.borderStyle);
        }

        inline uint32_t& bSmoothBorder(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_borderwindow.bSmoothBorder);
        }

        inline uint32_t& bStretchBorder(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_borderwindow.bStretchBorder);
        }

        inline uint32_t& bResizeable(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_borderwindow.bResizeable);
        }

        inline uint32_t& bMarginsFromBorder(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_borderwindow.bMarginsFromBorder);
        }

        inline float& childLeftMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.childLeftMargin);
        }

        inline float& childRightMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.childRightMargin);
        }

        inline float& childTopMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.childTopMargin);
        }

        inline float& childBottomMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.childBottomMargin);
        }

        inline float& leftMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.leftMargin);
        }

        inline float& rightMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.rightMargin);
        }

        inline float& TopMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.TopMargin);
        }

        inline float& BottomMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.BottomMargin);
        }

        inline uint32_t& bLeftDrag(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_borderwindow.bLeftDrag);
        }

        inline uint32_t& bRightDrag(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_borderwindow.bRightDrag);
        }

        inline uint32_t& bUpDrag(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_borderwindow.bUpDrag);
        }

        inline uint32_t& bDownDrag(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_borderwindow.bDownDrag);
        }

        inline float& lastMouseX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.lastMouseX);
        }

        inline float& lastMouseY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.lastMouseY);
        }

        inline float& DragX(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.DragX);
        }

        inline float& DragY(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.DragY);
        }

        inline float& dragWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.dragWidth);
        }

        inline float& dragHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_borderwindow.dragHeight);
        }

    }

    namespace checkboxwindow {
        struct Offsets {
            size_t toggleOff = 0;
            size_t toggleOn = 0;
            size_t textureWidth = 0;
            size_t textureHeight = 0;
            size_t checkboxSpacing = 0;
            size_t bRightSide = 0;
            size_t checkboxStyle = 0;
            size_t checkboxColor = 0;
        };
    }

    extern checkboxwindow::Offsets offsets_checkboxwindow;

    namespace checkboxwindow {
        inline UObject*& toggleOff(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_checkboxwindow.toggleOff);
        }

        inline UObject*& toggleOn(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_checkboxwindow.toggleOn);
        }

        inline float& textureWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_checkboxwindow.textureWidth);
        }

        inline float& textureHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_checkboxwindow.textureHeight);
        }

        inline float& checkboxSpacing(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_checkboxwindow.checkboxSpacing);
        }

        inline uint32_t& bRightSide(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_checkboxwindow.bRightSide);
        }

        inline uint8_t& checkboxStyle(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_checkboxwindow.checkboxStyle);
        }

        inline Color& checkboxColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_checkboxwindow.checkboxColor);
        }

    }

    namespace computerwindow {
        struct Offsets {
            size_t eventTimeInterval = 0;
            size_t timeNextEvent = 0;
            size_t timeLastEvent = 0;
            size_t timeCurrent = 0;
            size_t fadeSpeed = 0;
            size_t throttle = 0;
            size_t fadeOutTimer = 0;
            size_t fadeOutStart = 0;
            size_t textCols = 0;
            size_t textRows = 0;
            size_t textFont = 0;
            size_t fontWidth = 0;
            size_t fontHeight = 0;
            size_t FontColor = 0;
            size_t textSound = 0;
            size_t typingSound = 0;
            size_t computerSoundVolume = 0;
            size_t bWordWrap = 0;
            size_t bLastLineWrapped = 0;
            size_t bInvokeComputerStart = 0;
            size_t bComputerStartInvoked = 0;
            size_t bFirstTick = 0;
            size_t backgroundTextures = 0;
            size_t cursorTexture = 0;
            size_t cursorColor = 0;
            size_t cursorWidth = 0;
            size_t cursorHeight = 0;
            size_t cursorBlinkSpeed = 0;
            size_t bCursorVisible = 0;
            size_t bShowCursor = 0;
            size_t cursorNextEvent = 0;
            size_t colGraphicTile = 0;
            size_t TextWindow = 0;
            size_t Player = 0;
            size_t displayBuffer = 0;
            size_t queuedBuffer = 0;
            size_t queuedBufferStart = 0;
            size_t TextX = 0;
            size_t TextY = 0;
            size_t inputKey = 0;
            size_t inputMask = 0;
            size_t inputWindow = 0;
            size_t bWaitingForKey = 0;
            size_t bEchoKey = 0;
            size_t bPauseProcessing = 0;
            size_t bIgnoreTick = 0;
            size_t bGamePaused = 0;
            size_t bIgnoreGamePaused = 0;
        };
    }

    extern computerwindow::Offsets offsets_computerwindow;

    namespace computerwindow {
        inline float& eventTimeInterval(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.eventTimeInterval);
        }

        inline float& timeNextEvent(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.timeNextEvent);
        }

        inline float& timeLastEvent(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.timeLastEvent);
        }

        inline float& timeCurrent(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.timeCurrent);
        }

        inline float& fadeSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.fadeSpeed);
        }

        inline float& throttle(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.throttle);
        }

        inline float& fadeOutTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.fadeOutTimer);
        }

        inline float& fadeOutStart(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.fadeOutStart);
        }

        inline int32_t& textCols(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.textCols);
        }

        inline int32_t& textRows(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.textRows);
        }

        inline UObject*& textFont(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_computerwindow.textFont);
        }

        inline int32_t& fontWidth(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.fontWidth);
        }

        inline int32_t& fontHeight(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.fontHeight);
        }

        inline Color& FontColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_computerwindow.FontColor);
        }

        inline UObject*& textSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_computerwindow.textSound);
        }

        inline UObject*& typingSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_computerwindow.typingSound);
        }

        inline float& computerSoundVolume(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.computerSoundVolume);
        }

        inline uint32_t& bWordWrap(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bWordWrap);
        }

        inline uint32_t& bLastLineWrapped(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bLastLineWrapped);
        }

        inline uint32_t& bInvokeComputerStart(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bInvokeComputerStart);
        }

        inline uint32_t& bComputerStartInvoked(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bComputerStartInvoked);
        }

        inline uint32_t& bFirstTick(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bFirstTick);
        }

        inline UObject*& backgroundTextures(UObject* o, int index)
        {
            return reinterpret_cast<UObject**>(o->data + offsets_computerwindow.backgroundTextures)[index];
        }

        inline UObject*& cursorTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_computerwindow.cursorTexture);
        }

        inline Color& cursorColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_computerwindow.cursorColor);
        }

        inline int32_t& cursorWidth(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.cursorWidth);
        }

        inline int32_t& cursorHeight(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.cursorHeight);
        }

        inline float& cursorBlinkSpeed(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.cursorBlinkSpeed);
        }

        inline uint32_t& bCursorVisible(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bCursorVisible);
        }

        inline uint32_t& bShowCursor(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bShowCursor);
        }

        inline float& cursorNextEvent(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_computerwindow.cursorNextEvent);
        }

        inline Color& colGraphicTile(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_computerwindow.colGraphicTile);
        }

        inline UObject*& TextWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_computerwindow.TextWindow);
        }

        inline UObject*& Player(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_computerwindow.Player);
        }

        inline uint8_t* displayBuffer(UObject* o)
        {
            return o->data + offsets_computerwindow.displayBuffer;
        }

        inline uint8_t* queuedBuffer(UObject* o)
        {
            return o->data + offsets_computerwindow.queuedBuffer;
        }

        inline int32_t& queuedBufferStart(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.queuedBufferStart);
        }

        inline int32_t& TextX(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.TextX);
        }

        inline int32_t& TextY(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_computerwindow.TextY);
        }

        inline std::string& inputKey(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_computerwindow.inputKey);
        }

        inline std::string& inputMask(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_computerwindow.inputMask);
        }

        inline UObject*& inputWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_computerwindow.inputWindow);
        }

        inline uint32_t& bWaitingForKey(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bWaitingForKey);
        }

        inline uint32_t& bEchoKey(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bEchoKey);
        }

        inline uint32_t& bPauseProcessing(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bPauseProcessing);
        }

        inline uint32_t& bIgnoreTick(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bIgnoreTick);
        }

        inline uint32_t& bGamePaused(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bGamePaused);
        }

        inline uint32_t& bIgnoreGamePaused(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_computerwindow.bIgnoreGamePaused);
        }

    }

    namespace listwindow {
        struct Offsets {
            size_t Delimiter = 0;
            size_t inverseColor = 0;
            size_t highlightTexture = 0;
            size_t focusTexture = 0;
            size_t highlightColor = 0;
            size_t focusColor = 0;
            size_t focusThickness = 0;
            size_t rows = 0;
            size_t cols = 0;
            size_t bAutoSort = 0;
            size_t bAutoExpandColumns = 0;
            size_t bMultiSelect = 0;
            size_t colMargin = 0;
            size_t rowMargin = 0;
            size_t bHotKeys = 0;
            size_t hotKeyCol = 0;
            size_t lineSize = 0;
            size_t ActivateSound = 0;
            size_t moveSound = 0;
            size_t numSelected = 0;
            size_t focusLine = 0;
            size_t anchorLine = 0;
            size_t bDragging = 0;
            size_t lastIndex = 0;
            size_t remainingDelay = 0;
            size_t hotKeyString = 0;
            size_t hotKeyTimer = 0;
        };
    }

    extern listwindow::Offsets offsets_listwindow;

    namespace listwindow {
        inline std::string& Delimiter(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_listwindow.Delimiter);
        }

        inline Color& inverseColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_listwindow.inverseColor);
        }

        inline UObject*& highlightTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_listwindow.highlightTexture);
        }

        inline UObject*& focusTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_listwindow.focusTexture);
        }

        inline Color& highlightColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_listwindow.highlightColor);
        }

        inline Color& focusColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_listwindow.focusColor);
        }

        inline float& focusThickness(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_listwindow.focusThickness);
        }

        inline uint8_t* rows(UObject* o)
        {
            return o->data + offsets_listwindow.rows;
        }

        inline uint8_t* cols(UObject* o)
        {
            return o->data + offsets_listwindow.cols;
        }

        inline uint32_t& bAutoSort(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_listwindow.bAutoSort);
        }

        inline uint32_t& bAutoExpandColumns(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_listwindow.bAutoExpandColumns);
        }

        inline uint32_t& bMultiSelect(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_listwindow.bMultiSelect);
        }

        inline float& colMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_listwindow.colMargin);
        }

        inline float& rowMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_listwindow.rowMargin);
        }

        inline uint32_t& bHotKeys(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_listwindow.bHotKeys);
        }

        inline int32_t& hotKeyCol(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_listwindow.hotKeyCol);
        }

        inline float& lineSize(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_listwindow.lineSize);
        }

        inline UObject*& ActivateSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_listwindow.ActivateSound);
        }

        inline UObject*& moveSound(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_listwindow.moveSound);
        }

        inline int32_t& numSelected(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_listwindow.numSelected);
        }

        inline int32_t& focusLine(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_listwindow.focusLine);
        }

        inline int32_t& anchorLine(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_listwindow.anchorLine);
        }

        inline uint32_t& bDragging(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_listwindow.bDragging);
        }

        inline int32_t& lastIndex(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_listwindow.lastIndex);
        }

        inline float& remainingDelay(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_listwindow.remainingDelay);
        }

        inline std::string& hotKeyString(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_listwindow.hotKeyString);
        }

        inline float& hotKeyTimer(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_listwindow.hotKeyTimer);
        }

    }

    namespace radioboxwindow {
        struct Offsets {
            size_t bOneCheck = 0;
            size_t toggleButtons = 0;
            size_t currentSelection = 0;
        };
    }

    extern radioboxwindow::Offsets offsets_radioboxwindow;

    namespace radioboxwindow {
        inline uint32_t& bOneCheck(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_radioboxwindow.bOneCheck);
        }

        inline uint8_t* toggleButtons(UObject* o)
        {
            return o->data + offsets_radioboxwindow.toggleButtons;
        }

        inline UObject*& currentSelection(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_radioboxwindow.currentSelection);
        }

    }

    namespace scrollareawindow {
        struct Offsets {
            size_t hScaleMgr = 0;
            size_t vScaleMgr = 0;
            size_t hScale = 0;
            size_t vScale = 0;
            size_t LeftButton = 0;
            size_t RightButton = 0;
            size_t UpButton = 0;
            size_t DownButton = 0;
            size_t ClipWindow = 0;
            size_t marginWidth = 0;
            size_t marginHeight = 0;
            size_t scrollbarDistance = 0;
            size_t bHideScrollbars = 0;
            size_t bHLastShow = 0;
            size_t bVLastShow = 0;
        };
    }

    extern scrollareawindow::Offsets offsets_scrollareawindow;

    namespace scrollareawindow {
        inline UObject*& hScaleMgr(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.hScaleMgr);
        }

        inline UObject*& vScaleMgr(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.vScaleMgr);
        }

        inline UObject*& hScale(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.hScale);
        }

        inline UObject*& vScale(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.vScale);
        }

        inline UObject*& LeftButton(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.LeftButton);
        }

        inline UObject*& RightButton(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.RightButton);
        }

        inline UObject*& UpButton(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.UpButton);
        }

        inline UObject*& DownButton(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.DownButton);
        }

        inline UObject*& ClipWindow(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_scrollareawindow.ClipWindow);
        }

        inline float& marginWidth(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scrollareawindow.marginWidth);
        }

        inline float& marginHeight(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scrollareawindow.marginHeight);
        }

        inline float& scrollbarDistance(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_scrollareawindow.scrollbarDistance);
        }

        inline uint32_t& bHideScrollbars(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scrollareawindow.bHideScrollbars);
        }

        inline uint32_t& bHLastShow(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scrollareawindow.bHLastShow);
        }

        inline uint32_t& bVLastShow(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_scrollareawindow.bVLastShow);
        }

    }

    namespace extstring {
        struct Offsets {
            size_t speechPage = 0;
            size_t Text = 0;
        };
    }

    extern extstring::Offsets offsets_extstring;

    namespace extstring {
        inline int32_t& speechPage(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_extstring.speechPage);
        }

        inline std::string& Text(UObject* o)
        {
            return *reinterpret_cast<std::string*>(o->data + offsets_extstring.Text);
        }

    }

    namespace textlogwindow {
        struct Offsets {
            size_t textTimeout = 0;
            size_t Lines = 0;
            size_t bTooTall = 0;
            size_t bPaused = 0;
        };
    }

    extern textlogwindow::Offsets offsets_textlogwindow;

    namespace textlogwindow {
        inline float& textTimeout(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_textlogwindow.textTimeout);
        }

        inline uint8_t* Lines(UObject* o)
        {
            return o->data + offsets_textlogwindow.Lines;
        }

        inline uint32_t& bTooTall(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_textlogwindow.bTooTall);
        }

        inline uint32_t& bPaused(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_textlogwindow.bPaused);
        }

    }

    namespace tilewindow {
        struct Offsets {
            size_t orientation = 0;
            size_t hDirection = 0;
            size_t vDirection = 0;
            size_t hMargin = 0;
            size_t vMargin = 0;
            size_t minorSpacing = 0;
            size_t majorSpacing = 0;
            size_t hChildAlign = 0;
            size_t vChildAlign = 0;
            size_t bWrap = 0;
            size_t bFillParent = 0;
            size_t bEqualWidth = 0;
            size_t bEqualHeight = 0;
            size_t rowArray = 0;
        };
    }

    extern tilewindow::Offsets offsets_tilewindow;

    namespace tilewindow {
        inline uint8_t& orientation(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_tilewindow.orientation);
        }

        inline uint8_t& hDirection(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_tilewindow.hDirection);
        }

        inline uint8_t& vDirection(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_tilewindow.vDirection);
        }

        inline float& hMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_tilewindow.hMargin);
        }

        inline float& vMargin(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_tilewindow.vMargin);
        }

        inline float& minorSpacing(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_tilewindow.minorSpacing);
        }

        inline float& majorSpacing(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_tilewindow.majorSpacing);
        }

        inline uint8_t& hChildAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_tilewindow.hChildAlign);
        }

        inline uint8_t& vChildAlign(UObject* o)
        {
            return *reinterpret_cast<uint8_t*>(o->data + offsets_tilewindow.vChildAlign);
        }

        inline uint32_t& bWrap(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_tilewindow.bWrap);
        }

        inline uint32_t& bFillParent(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_tilewindow.bFillParent);
        }

        inline uint32_t& bEqualWidth(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_tilewindow.bEqualWidth);
        }

        inline uint32_t& bEqualHeight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_tilewindow.bEqualHeight);
        }

        inline uint8_t* rowArray(UObject* o)
        {
            return o->data + offsets_tilewindow.rowArray;
        }

    }

    namespace viewportwindow {
        struct Offsets {
            size_t bEnableViewport = 0;
            size_t bClearZ = 0;
            size_t bShowActor = 0;
            size_t bShowWeapons = 0;
            size_t bUseViewRotation = 0;
            size_t bUseEyeHeight = 0;
            size_t bWatchEyeHeight = 0;
            size_t FOV = 0;
            size_t DefaultTexture = 0;
            size_t DefaultColor = 0;
            size_t originActor = 0;
            size_t watchActor = 0;
            size_t Location = 0;
            size_t relLocation = 0;
            size_t Rotation = 0;
            size_t relRotation = 0;
            size_t bOriginActorDestroyed = 0;
            size_t lastLocation = 0;
            size_t lastRotation = 0;
            size_t viewportFrame = 0;
        };
    }

    extern viewportwindow::Offsets offsets_viewportwindow;

    namespace viewportwindow {
        inline uint32_t& bEnableViewport(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_viewportwindow.bEnableViewport);
        }

        inline uint32_t& bClearZ(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_viewportwindow.bClearZ);
        }

        inline uint32_t& bShowActor(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_viewportwindow.bShowActor);
        }

        inline uint32_t& bShowWeapons(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_viewportwindow.bShowWeapons);
        }

        inline uint32_t& bUseViewRotation(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_viewportwindow.bUseViewRotation);
        }

        inline uint32_t& bUseEyeHeight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_viewportwindow.bUseEyeHeight);
        }

        inline uint32_t& bWatchEyeHeight(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_viewportwindow.bWatchEyeHeight);
        }

        inline float& FOV(UObject* o)
        {
            return *reinterpret_cast<float*>(o->data + offsets_viewportwindow.FOV);
        }

        inline UObject*& DefaultTexture(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_viewportwindow.DefaultTexture);
        }

        inline Color& DefaultColor(UObject* o)
        {
            return *reinterpret_cast<Color*>(o->data + offsets_viewportwindow.DefaultColor);
        }

        inline UObject*& originActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_viewportwindow.originActor);
        }

        inline UObject*& watchActor(UObject* o)
        {
            return *reinterpret_cast<UObject**>(o->data + offsets_viewportwindow.watchActor);
        }

        inline Vector& Location(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_viewportwindow.Location);
        }

        inline Vector& relLocation(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_viewportwindow.relLocation);
        }

        inline Rotator& Rotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_viewportwindow.Rotation);
        }

        inline Rotator& relRotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_viewportwindow.relRotation);
        }

        inline uint32_t& bOriginActorDestroyed(UObject* o)
        {
            return *reinterpret_cast<uint32_t*>(o->data + offsets_viewportwindow.bOriginActorDestroyed);
        }

        inline Vector& lastLocation(UObject* o)
        {
            return *reinterpret_cast<Vector*>(o->data + offsets_viewportwindow.lastLocation);
        }

        inline Rotator& lastRotation(UObject* o)
        {
            return *reinterpret_cast<Rotator*>(o->data + offsets_viewportwindow.lastRotation);
        }

        inline int32_t& viewportFrame(UObject* o)
        {
            return *reinterpret_cast<int32_t*>(o->data + offsets_viewportwindow.viewportFrame);
        }

    }

    void bindProps_extension();
}
