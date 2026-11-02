#include "engine/fractal_paint.h"

#include "engine/mesh.h"
#include "engine/props_fire.h"

#include <algorithm>
#include <cstdlib>

namespace dx::fractalpaint {
    namespace {
        enum SparkKind : uint8_t {
            SparkBurn,
            SparkSparkle,
            SparkPulse,
            SparkSignal,
            SparkBlaze,
            SparkOzHasSpoken,
            SparkCone,
            SparkBlazeRight,
            SparkBlazeLeft,
            SparkCylinder,
            SparkCylinder3D,
            SparkLissajous,
            SparkJugglers,
            SparkEmit,
            SparkFountain,
            SparkFlocks,
            SparkEels,
            SparkOrganic,
            SparkWanderOrganic,
            SparkRandomCloud,
            SparkCustomCloud,
            SparkLocalCloud,
            SparkStars,
            SparkLineLightning,
            SparkRampLightning,
            SparkSphereLightning,
            SparkWheel,
            SparkGametes,
            SparkSprinkler,
            SparkLissajX,
            SparkLissajY,
        };

        enum DrawKind : uint8_t {
            DrawNormal,
            DrawLathe,
            DrawLathe2,
            DrawLathe3,
            DrawLathe4,
        };

        enum DropKind : uint8_t {
            DropFixedDepth,
            DropPhaseSpot,
            DropShallowSpot,
            DropHalfAmpl,
            DropRandomMover,
            DropFixedRandomSpot,
            DropWhirlyThing,
            DropBigWhirly,
            DropHorizontalLine,
            DropVerticalLine,
            DropDiagonalLine1,
            DropDiagonalLine2,
            DropHorizontalOsc,
            DropVerticalOsc,
            DropDiagonalOsc1,
            DropDiagonalOsc2,
            DropRainDrops,
            DropAreaClamp,
            DropLeakyTap,
            DropDrippyTap,
            DropWhirlyBack = 64,
            DropBigWhirlyBack,
        };

        uint8_t u8(int32_t value)
        {
            return static_cast<uint8_t>(value);
        }

        uint8_t speedRand()
        {
            static uint32_t seed = 0x2545F491u;
            seed = seed * 1103515245u + 12345u;
            return static_cast<uint8_t>(seed >> 16);
        }

        bool inside(UObject* texture, int32_t x, int32_t y)
        {
            UTexture* image = cast<UTexture>(texture);
            return image && x >= 0 && y >= 0 && x < static_cast<int32_t>(image->width()) && y < static_cast<int32_t>(image->height());
        }

        size_t pushSpark(UFireTexture* fire)
        {
            size_t index = static_cast<size_t>(firetexture::NumSparks(fire)++);
            if (fire->sparks.size() <= index) {
                fire->sparks.resize(index + 1);
            }
            fire->sparks[index] = FireSpark();
            return index;
        }

        FireSpark& lathed(UFireTexture* fire, const FireSpark& source, uint8_t phase, int32_t y)
        {
            FireSpark& spark = fire->sparks[pushSpark(fire)];
            spark.type = source.type;
            spark.x = source.x;
            spark.y = u8(y);
            spark.heat = firetexture::FX_Heat(fire);
            spark.a = phase;
            spark.b = source.b;
            spark.d = source.d;
            return spark;
        }

        void cylinder(UFireTexture* fire, size_t index, int32_t x, int32_t y)
        {
            int32_t size = firetexture::FX_Size(fire);
            int32_t speed = firetexture::FX_HorizSpeed(fire) - 128;
            int32_t penX = firetexture::PenDownX(fire);
            int32_t axis = penX * 2 + firetexture::FX_Size(fire);
            uint8_t mode = firetexture::DrawMode(fire);
            if (mode > DrawNormal && penX != 0) {
                size = axis - x * 2;
                if (size < 0) {
                    size = x * 2 - axis;
                    fire->sparks[index].x = u8(x - size);
                    speed = (256 - speed) & 255;
                }
            }
            FireSpark& spark = fire->sparks[index];
            spark.a = u8(fractaltexture::GlobalPhase(fire) * firetexture::FX_Frequency(fire) + firetexture::FX_Phase(fire));
            spark.b = u8(size);
            spark.d = u8(speed);
            FireSpark source = spark;
            int32_t limit = firetexture::SparksLimit(fire);
            if (mode == DrawLathe2 && firetexture::NumSparks(fire) < limit) {
                lathed(fire, source, u8(source.a + 128), y);
            }
            if (mode == DrawLathe3 && firetexture::NumSparks(fire) + 2 <= limit) {
                for (int32_t t = 1; t < 3; t++) {
                    lathed(fire, source, u8(source.a + t * (256 / 3)), y);
                }
            }
            if (mode == DrawLathe4 && firetexture::NumSparks(fire) + 3 <= limit) {
                for (int32_t t = 1; t < 4; t++) {
                    lathed(fire, source, u8(source.a + t * 64), y);
                }
            }
        }

        void lightning(UFireTexture* fire, PaintState& state, size_t index, int32_t x, int32_t y)
        {
            uint8_t type = fire->sparks[index].type;
            int32_t open = static_cast<int32_t>(index) - 1;
            while (open >= 0 && !(fire->sparks[static_cast<size_t>(open)].type == type && fire->sparks[static_cast<size_t>(open)].d == 0)) {
                open--;
            }
            if (open < 0) {
                fire->sparks[index].d = 0;
                fire->sparks[index].heat = 0;
                state.lightPinX = x;
                state.lightPinY = y;
                return;
            }
            firetexture::NumSparks(fire)--;
            fire->sparks.resize(static_cast<size_t>(firetexture::NumSparks(fire)));
            FireSpark& spark = fire->sparks[static_cast<size_t>(open)];
            spark.x = u8(state.lightPinX);
            spark.y = u8(state.lightPinY);
            spark.heat = u8(3 | firetexture::FX_Heat(fire));
            int32_t lengthX = x - spark.x;
            int32_t lengthY = y - spark.y;
            lengthX = lengthX < 0 ? (-lengthX) | 1 : lengthX & ~1;
            lengthY = lengthY < 0 ? (-lengthY) | 1 : lengthY & ~1;
            if (lengthX == 0 && lengthY == 0) {
                spark.heat = 0;
            }
            spark.a = u8(lengthX);
            spark.b = u8(lengthY);
        }

        uint8_t* drop(UObject* texture, int32_t index)
        {
            return watertexture::Drops(texture) + static_cast<size_t>(index) * 8;
        }

        void whirly(UObject* texture, uint8_t* target, uint8_t backwards)
        {
            target[1] = watertexture::FX_Depth(texture);
            target[4] = 0;
            int32_t frequency = watertexture::FX_Frequency(texture);
            if (frequency > 127) {
                target[0] = backwards;
                frequency = 255 - frequency;
            }
            uint32_t delta = 65535u / static_cast<uint32_t>(127 - frequency + 1);
            target[6] = static_cast<uint8_t>(delta & 0xff);
            target[7] = static_cast<uint8_t>((delta & 0xff00) >> 8);
            target[5] = static_cast<uint8_t>(((static_cast<uint32_t>(fractaltexture::GlobalPhase(texture)) * delta + (static_cast<uint32_t>(watertexture::FX_Phase(texture)) << 8)) >> 8) & 255);
        }
    }

    bool paintable(UObject* texture)
    {
        return texture && (cast<UFireTexture>(texture) || texture->isA(Name("WaterTexture")));
    }

    void addSpark(UObject* texture, PaintState& state, int32_t x, int32_t y)
    {
        UFireTexture* fire = cast<UFireTexture>(texture);
        if (!fire || !inside(texture, x, y) || firetexture::NumSparks(fire) >= firetexture::SparksLimit(fire)) {
            return;
        }
        size_t index = pushSpark(fire);
        FireSpark& spark = fire->sparks[index];
        spark.type = firetexture::SparkType(fire);
        spark.x = u8(x);
        spark.y = u8(y);
        spark.heat = firetexture::FX_Heat(fire);
        switch (spark.type) {
        case SparkSparkle:
            spark.a = firetexture::FX_Size(fire);
            spark.b = firetexture::FX_AuxSize(fire);
            break;
        case SparkPulse:
            spark.heat = u8(fractaltexture::DrawPhase(fire) + fractaltexture::AuxPhase(fire));
            fractaltexture::DrawPhase(fire) = u8(fractaltexture::DrawPhase(fire) + firetexture::FX_Phase(fire));
            spark.d = firetexture::FX_Frequency(fire);
            break;
        case SparkSignal:
            spark.heat = speedRand();
            spark.d = firetexture::FX_Frequency(fire);
            spark.c = u8(255 - firetexture::FX_Heat(fire));
            break;
        case SparkStars:
            spark.a = firetexture::FX_Heat(fire);
            firetexture::StarStatus(fire) = 1;
            break;
        case SparkOrganic:
            spark.c = firetexture::FX_Area(fire);
            break;
        case SparkEels:
        case SparkBlazeLeft:
        case SparkBlazeRight:
            spark.c = firetexture::FX_Size(fire);
            break;
        case SparkEmit:
        case SparkFountain:
            spark.a = u8(firetexture::FX_HorizSpeed(fire) - 128);
            spark.b = u8(127 ^ firetexture::FX_VertSpeed(fire));
            spark.d = u8(255 / (firetexture::FX_Size(fire) + 1));
            break;
        case SparkCylinder:
        case SparkCylinder3D:
            cylinder(fire, index, x, y);
            break;
        case SparkJugglers:
            spark.a = u8(fractaltexture::GlobalPhase(fire) * firetexture::FX_Frequency(fire) + firetexture::FX_Phase(fire));
            spark.b = firetexture::FX_Size(fire);
            spark.d = u8(firetexture::FX_HorizSpeed(fire) - 128);
            break;
        case SparkLissajous:
            spark.heat = firetexture::FX_Size(fire);
            spark.a = firetexture::FX_Phase(fire);
            spark.b = firetexture::FX_Frequency(fire);
            spark.c = u8(firetexture::FX_HorizSpeed(fire) - 128);
            spark.d = u8(firetexture::FX_VertSpeed(fire) - 128);
            if (spark.c == 0) {
                spark.type = SparkLissajY;
            }
            if (spark.d == 0) {
                spark.type = SparkLissajX;
            }
            break;
        case SparkSphereLightning:
            spark.c = std::max<uint8_t>(firetexture::FX_Size(fire), 8);
            spark.d = 96;
            break;
        case SparkFlocks:
            spark.a = 128;
            spark.b = firetexture::FX_Size(fire);
            spark.c = 128;
            spark.d = u8(255 - firetexture::FX_Area(fire));
            break;
        case SparkWheel:
            spark.a = firetexture::FX_Phase(fire);
            spark.b = firetexture::FX_Size(fire);
            spark.c = firetexture::FX_Frequency(fire);
            spark.d = u8(255 - firetexture::FX_Area(fire));
            break;
        case SparkSprinkler:
            spark.a = firetexture::FX_Phase(fire);
            spark.b = 128;
            spark.c = 128;
            spark.d = firetexture::FX_Frequency(fire);
            break;
        case SparkGametes:
            spark.c = firetexture::FX_Size(fire);
            break;
        case SparkLocalCloud:
            spark.c = firetexture::FX_Area(fire);
            spark.a = u8(firetexture::FX_HorizSpeed(fire) - 128);
            spark.b = u8((255 ^ firetexture::FX_VertSpeed(fire)) - 128);
            spark.d = u8(255 - firetexture::FX_Size(fire));
            break;
        case SparkCustomCloud:
            spark.a = u8(firetexture::FX_HorizSpeed(fire) - 128);
            spark.b = u8((255 ^ firetexture::FX_VertSpeed(fire)) - 128);
            spark.d = u8(255 - firetexture::FX_Size(fire));
            break;
        case SparkLineLightning:
        case SparkRampLightning:
            lightning(fire, state, index, x, y);
            break;
        default:
            break;
        }
    }

    void closeSpark(UObject* texture)
    {
        UFireTexture* fire = cast<UFireTexture>(texture);
        if (!fire) {
            return;
        }
        for (int32_t s = firetexture::NumSparks(fire) - 1; s >= 0; s--) {
            FireSpark& spark = fire->sparks[static_cast<size_t>(s)];
            if ((spark.type == SparkLineLightning || spark.type == SparkRampLightning) && spark.d == 0) {
                spark.d = std::max<uint8_t>(firetexture::FX_Frequency(fire), 1);
                return;
            }
        }
    }

    void deleteSparks(UObject* texture, int32_t x, int32_t y, int32_t range)
    {
        UFireTexture* fire = cast<UFireTexture>(texture);
        if (!fire) {
            return;
        }
        int32_t& count = firetexture::NumSparks(fire);
        for (int32_t s = 0; s < count; s++) {
            const FireSpark& spark = fire->sparks[static_cast<size_t>(s)];
            if (range >= std::abs(x - spark.x) + std::abs(y - spark.y)) {
                fire->sparks[static_cast<size_t>(s)] = fire->sparks[static_cast<size_t>(--count)];
            }
        }
        fire->sparks.resize(static_cast<size_t>(std::max(count, 0)));
    }

    void drawSparkLine(UObject* texture, PaintState& state, int32_t startX, int32_t startY, int32_t destX, int32_t destY)
    {
        int32_t divX = destX - startX;
        int32_t divY = destY - startY;
        if (divX == 0 && divY == 0) {
            return;
        }
        int32_t incX = divX < 0 ? -1 : (divX > 0 ? 1 : 0);
        int32_t incY = divY < 0 ? -1 : (divY > 0 ? 1 : 0);
        divX = std::abs(divX);
        divY = std::abs(divY);
        int32_t x = startX;
        int32_t y = startY;
        bool alongX = divX >= divY;
        int32_t major = alongX ? divX : divY;
        int32_t minor2 = 2 * (alongX ? divY : divX);
        int32_t diff = minor2 - major;
        int32_t step = minor2 - 2 * major;
        for (int32_t count = 1; count <= major; count++) {
            addSpark(texture, state, x, y);
            if (diff < 0) {
                diff += minor2;
            } else {
                diff += step;
                if (alongX) {
                    y += incY;
                } else {
                    x += incX;
                }
            }
            if (alongX) {
                x += incX;
            } else {
                y += incY;
            }
        }
    }

    void addDrop(UObject* texture, int32_t x, int32_t y)
    {
        if (!texture || !texture->isA(Name("WaterTexture")) || !inside(texture, x, y) || watertexture::NumDrops(texture) >= MaxDrops) {
            return;
        }
        int32_t index = watertexture::NumDrops(texture)++;
        uint8_t* target = drop(texture, index);
        std::fill_n(target, 8, uint8_t { 0 });
        target[0] = watertexture::DropType(texture);
        target[2] = u8(x >> 1);
        target[3] = u8(y >> 1);
        fractaltexture::DrawPhase(texture)++;
        uint8_t phase = u8(fractaltexture::GlobalPhase(texture) * watertexture::FX_Frequency(texture) + watertexture::FX_Phase(texture));
        switch (target[0]) {
        case DropPhaseSpot:
        case DropHalfAmpl:
        case DropShallowSpot:
            target[1] = phase;
            target[7] = watertexture::FX_Frequency(texture);
            break;
        case DropHorizontalLine:
        case DropVerticalLine:
        case DropDiagonalLine1:
        case DropDiagonalLine2:
        case DropRainDrops:
        case DropAreaClamp:
            target[1] = watertexture::FX_Depth(texture);
            target[7] = watertexture::FX_Size(texture);
            break;
        case DropHorizontalOsc:
        case DropVerticalOsc:
        case DropDiagonalOsc1:
        case DropDiagonalOsc2:
            target[1] = phase;
            target[6] = watertexture::FX_Frequency(texture);
            target[7] = watertexture::FX_Size(texture);
            break;
        case DropWhirlyThing:
            whirly(texture, target, DropWhirlyBack);
            break;
        case DropBigWhirly:
            whirly(texture, target, DropBigWhirlyBack);
            break;
        case DropLeakyTap:
            target[1] = watertexture::FX_Depth(texture);
            target[4] = phase;
            target[7] = u8(watertexture::FX_Frequency(texture) >> 2);
            break;
        case DropDrippyTap:
            target[1] = watertexture::FX_Depth(texture);
            target[4] = speedRand();
            target[7] = u8(watertexture::FX_Frequency(texture) >> 2);
            break;
        default:
            break;
        }
    }

    void deleteDrops(UObject* texture, int32_t x, int32_t y, int32_t range)
    {
        if (!texture || !texture->isA(Name("WaterTexture"))) {
            return;
        }
        int32_t& count = watertexture::NumDrops(texture);
        for (int32_t s = 0; s < count; s++) {
            uint8_t* target = drop(texture, s);
            if (range >= std::abs(x - (target[2] << 1)) + std::abs(y - (target[3] << 1))) {
                std::copy_n(drop(texture, --count), 8, target);
            }
        }
    }

    void paint(UObject* texture, PaintState& state, int32_t x, int32_t y, bool left, bool right)
    {
        bool moved = state.lastX != x || state.lastY != y;
        bool leftChanged = state.lastLeft != left;
        bool rightChanged = state.lastRight != right;
        if (UFireTexture* fire = cast<UFireTexture>(texture)) {
            if (leftChanged) {
                firetexture::PenDownX(fire) = left ? u8(x) : 0;
                firetexture::PenDownY(fire) = left ? u8(y) : 0;
            }
            uint8_t type = firetexture::SparkType(fire);
            if (left) {
                if (!leftChanged && moved && type < SparkBlaze) {
                    drawSparkLine(fire, state, state.lastX, state.lastY, x, y);
                }
                if (leftChanged || (moved && type >= SparkBlaze)) {
                    addSpark(fire, state, x, y);
                }
            } else if (leftChanged || moved) {
                closeSpark(fire);
            }
            if (!leftChanged && !moved && left) {
                closeSpark(fire);
            }
            if (right && (moved || rightChanged)) {
                deleteSparks(fire, x, y, DeleteRange);
            }
        } else if (texture && texture->isA(Name("WaterTexture"))) {
            uint8_t type = watertexture::DropType(texture);
            bool continuous = type < DropHorizontalLine || type == DropRainDrops || type > DropAreaClamp;
            if (left && (leftChanged || (continuous && moved))) {
                addDrop(texture, x, y);
            }
            if (right) {
                deleteDrops(texture, x, y, DeleteRange);
            }
        }
        state.lastX = x;
        state.lastY = y;
        state.lastLeft = left;
        state.lastRight = right;
    }
}
