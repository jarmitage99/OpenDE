#include "render/procedural.h"

#include "core/objects.h"
#include "engine/mesh.h"
#include "engine/props_engine.h"
#include "engine/props_fire.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace dx {
    namespace {
        const Name nameFireTexture("FireTexture");
        const Name nameWaveTexture("WaveTexture");
        const Name nameWetTexture("WetTexture");
        const Name nameWaterTexture("WaterTexture");
        const Name nameIceTexture("IceTexture");

        constexpr double UnrealPi = 3.1415926535897932;
        constexpr int32_t MaxSparksLimit = 8192;
        constexpr int32_t MinSparksLimit = 4;
        constexpr int32_t StarCutoff = 38;
        constexpr int32_t LowestGlow = 50;
        constexpr int32_t LightPhaseBias = 32;
        constexpr int32_t MaxDrops = 256;

        enum class Kind : uint8_t {
            None,
            Fire,
            Water,
            Wave,
            Wet,
            Ice,
        };

        enum SparkType : uint8_t {
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
            SparkTransients,
            SparkDrifter,
            SparkDriftSlow,
            SparkFaller,
            SparkVShooter,
            SparkDrop,
            SparkMove,
            SparkSpawnedEel,
            SparkSpawnedTwirl,
            SparkSprinklerTwirl,
            SparkCustom,
            SparkGraviton,
            SparkSpawnedSperm,
        };

        enum DropType : uint8_t {
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

        struct WaterDrop {
            uint8_t type = 0;
            uint8_t depth = 0;
            uint8_t x = 0;
            uint8_t y = 0;
            uint8_t a = 0;
            uint8_t b = 0;
            uint8_t c = 0;
            uint8_t d = 0;
        };

        uint8_t u8(int32_t value)
        {
            return static_cast<uint8_t>(value);
        }

        int32_t s8(uint8_t value)
        {
            return static_cast<int8_t>(value);
        }

        int32_t appRound(float value)
        {
            return static_cast<int32_t>(std::lrint(value));
        }

        int32_t logTwo(int32_t value)
        {
            int32_t bits = 0;
            while ((1 << (bits + 1)) <= value) {
                bits++;
            }
            return bits;
        }

        bool powerOfTwo(int32_t value)
        {
            return value > 0 && (value & (value - 1)) == 0;
        }

        struct FractalTables {
            uint8_t phase[256] = {};
            uint8_t signedPhase[256] = {};
            uint8_t lightPhase[256] = {};
            uint8_t wave[1536] = {};
            uint8_t randoms[512] = {};
            uint32_t randomIndex = 0;

            FractalTables()
            {
                for (int32_t t = 0; t < 256; t++) {
                    double angle = static_cast<double>(t) * static_cast<double>(1.0f / 256.0f) * static_cast<double>(6.2831853f);
                    double value = std::sin(angle) * 127.5 + static_cast<double>(127.45f);
                    phase[t] = u8(appRound(static_cast<float>(value)));
                }
                for (int32_t t = 0; t < 256; t++) {
                    lightPhase[t] = u8(std::clamp(phase[t] + LightPhaseBias, 0, 255));
                    signedPhase[t] = u8(-128 + s8(phase[t]));
                }
                uint32_t seed = 1;
                for (int32_t t = 0; t < 512; t++) {
                    seed = seed * 214013u + 2531011u;
                    randoms[t] = static_cast<uint8_t>((seed >> 16) & 0x7fffu & 255u);
                }
                for (int32_t t = 0; t < 1536; t++) {
                    wave[t] = u8(std::clamp((t / 2 - 256) + ((t - 512) < 256 ? 1 : 0), 0, 255));
                }
            }

            uint32_t word(uint32_t offset) const
            {
                return static_cast<uint32_t>(randoms[offset]) | (static_cast<uint32_t>(randoms[offset + 1]) << 8) | (static_cast<uint32_t>(randoms[offset + 2]) << 16) | (static_cast<uint32_t>(randoms[offset + 3]) << 24);
            }

            uint8_t speedRand()
            {
                uint32_t source = (randomIndex + 128) & 0xfc;
                uint32_t dest = (randomIndex + 4) & 0xfc;
                uint32_t value = word(source);
                randomIndex = dest;
                uint32_t mixed = word(dest) ^ value;
                for (uint32_t i = 0; i < 4; i++) {
                    randoms[dest + i] = static_cast<uint8_t>(mixed >> (i * 8));
                }
                return static_cast<uint8_t>(value);
            }
        };

        FractalTables fractal;

        uint8_t speedRand()
        {
            return fractal.speedRand();
        }

        Kind kindOf(UTexture* texture)
        {
            if (!texture || !texture->cls) {
                return Kind::None;
            }
            if (texture->isA(nameFireTexture)) {
                return Kind::Fire;
            }
            if (texture->isA(nameWaveTexture)) {
                return Kind::Wave;
            }
            if (texture->isA(nameWetTexture)) {
                return Kind::Wet;
            }
            if (texture->isA(nameWaterTexture)) {
                return Kind::Water;
            }
            if (texture->isA(nameIceTexture)) {
                return Kind::Ice;
            }
            return Kind::None;
        }

        const TextureMip* bitmapOf(UObject* object)
        {
            objects::ensureLoaded(object);
            UTexture* texture = cast<UTexture>(object);
            if (!texture || texture->mips.empty() || texture->format() != 0) {
                return nullptr;
            }
            TextureMip& mip = texture->mips[0];
            int32_t w = static_cast<int32_t>(mip.width);
            int32_t h = static_cast<int32_t>(mip.height);
            if (!powerOfTwo(w) || !powerOfTwo(h)) {
                return nullptr;
            }
            size_t count = static_cast<size_t>(w) * static_cast<size_t>(h);
            if (mip.data.size() < count && kindOf(texture) != Kind::None) {
                mip.data.resize(count, 0);
            }
            if (mip.data.size() < count) {
                return nullptr;
            }
            return &mip;
        }

        std::vector<uint8_t> upsample(const TextureMip& source, int32_t usize, int32_t vsize, int32_t uscaler, int32_t vscaler)
        {
            int32_t sourceBits = logTwo(static_cast<int32_t>(source.width));
            std::vector<uint8_t> result(static_cast<size_t>(usize) * static_cast<size_t>(vsize));
            for (int32_t v = 0; v < vsize; v++) {
                for (int32_t u = 0; u < usize; u++) {
                    size_t index = static_cast<size_t>((u >> uscaler) + ((v >> vscaler) << sourceBits));
                    result[static_cast<size_t>(u + v * usize)] = source.data[index];
                }
            }
            return result;
        }
    }

    struct ProceduralState {
        UTexture* texture = nullptr;
        Kind kind = Kind::None;
        int32_t usize = 0;
        int32_t vsize = 0;
        int32_t ubits = 0;
        int32_t vbits = 0;
        int32_t umask = 0;
        int32_t vmask = 0;
        bool valid = false;
        std::vector<uint8_t> pixels;
        UPalette* palette = nullptr;
        bool started = false;
        double lastTime = 0;
        float accumulator = 0;
        int32_t primeCurrent = 0;
        int32_t globalPhase = 0;
        uint8_t auxPhase = 0;
        uint8_t renderTable[1028] = {};
        std::vector<FireSpark> sparks;
        int32_t activeSparks = 0;
        int32_t sparksLimit = 0;
        uint8_t starStatus = 1;
        std::vector<uint8_t> previous;
        std::vector<uint8_t> fields;
        std::array<WaterDrop, MaxDrops> drops = {};
        int32_t numDrops = 0;
        uint8_t waveParity = 0;
        std::vector<uint8_t> sourceBitmap;
        std::vector<uint8_t> glassBitmap;
        float masterCount = 0;
        float uDisplace = 0;
        float vDisplace = 0;
        float uPosition = 0;
        float vPosition = 0;

        void init(UTexture* target)
        {
            texture = target;
            kind = kindOf(target);
            palette = target->palette();
            usize = std::max(bitmap::UClamp(target), 1);
            vsize = std::max(bitmap::VClamp(target), 1);
            if (!target->mips.empty() && target->mips[0].width > 0) {
                usize = static_cast<int32_t>(target->mips[0].width);
                vsize = static_cast<int32_t>(target->mips[0].height);
            }
            pixels.assign(static_cast<size_t>(usize) * static_cast<size_t>(vsize), 0);
            valid = powerOfTwo(usize) && powerOfTwo(vsize) && usize <= 256 && vsize <= 256;
            if (!valid) {
                return;
            }
            ubits = logTwo(usize);
            vbits = logTwo(vsize);
            umask = usize - 1;
            vmask = vsize - 1;
            switch (kind) {
            case Kind::Fire:
                initFire();
                break;
            case Kind::Wave:
                initWater();
                setWaveLight();
                break;
            case Kind::Wet:
                initWater();
                initWet();
                break;
            case Kind::Ice:
                initIce();
                break;
            case Kind::Water:
            case Kind::None:
                break;
            }
        }

        void initFire()
        {
            int32_t heat = firetexture::RenderHeat(texture);
            for (int32_t t = 0; t < 1024; t++) {
                double value = static_cast<double>(t) / 4.0 + 1.0 - static_cast<double>(255 - heat) / 16.0;
                renderTable[t] = static_cast<uint8_t>(std::clamp(value, 0.0, 255.0));
            }
            if (UFireTexture* fire = cast<UFireTexture>(texture)) {
                sparks = fire->sparks;
            }
            sparksLimit = firetexture::SparksLimit(texture);
            activeSparks = firetexture::NumSparks(texture);
            if (static_cast<int32_t>(sparks.size()) < sparksLimit) {
                sparks.resize(static_cast<size_t>(sparksLimit));
            }
            activeSparks = std::clamp(activeSparks, 0, static_cast<int32_t>(sparks.size()));
            if (static_cast<int32_t>(sparks.size()) != sparksLimit) {
                sparksLimit = std::clamp(sparksLimit, MinSparksLimit, MaxSparksLimit);
                if (static_cast<int32_t>(sparks.size()) > sparksLimit) {
                    if (activeSparks > sparksLimit) {
                        for (int32_t t = activeSparks - 1; t >= 0; t--) {
                            if (sparks[static_cast<size_t>(t)].type >= SparkTransients) {
                                sparks[static_cast<size_t>(t)] = sparks[static_cast<size_t>(--activeSparks)];
                                if (activeSparks <= sparksLimit) {
                                    break;
                                }
                            }
                        }
                    }
                    activeSparks = std::min(activeSparks, sparksLimit);
                }
                sparks.resize(static_cast<size_t>(sparksLimit));
            }
        }

        void initWater()
        {
            fields.assign(static_cast<size_t>(usize) * static_cast<size_t>(vsize) / 2, 128);
            numDrops = std::clamp(watertexture::NumDrops(texture), 0, MaxDrops);
            const uint8_t* source = watertexture::Drops(texture);
            for (int32_t i = 0; i < MaxDrops; i++) {
                const uint8_t* drop = source + i * 8;
                drops[static_cast<size_t>(i)] = { drop[0], drop[1], drop[2], drop[3], drop[4], drop[5], drop[6], drop[7] };
            }
        }

        void setWaveLight()
        {
            waveLight(watertexture::WaveAmp(texture), wavetexture::BumpMapLight(texture), wavetexture::BumpMapAngle(texture), wavetexture::PhongRange(texture), wavetexture::PhongSize(texture));
        }

        void waveLight(uint8_t amplitude, uint8_t bumpMapLight, uint8_t bumpMapAngle, uint8_t range, uint8_t size)
        {
            double waveAmp = amplitude;
            int32_t phongRange = range;
            double phongRadius = size * static_cast<double>(1.0f / 512.0f);
            double lamp = static_cast<double>(static_cast<float>(bumpMapLight * (UnrealPi / 255.0)));
            double viewer = static_cast<double>(static_cast<float>(bumpMapAngle * (UnrealPi / 255.0)));
            for (int32_t i = 0; i < 1024; i++) {
                double slope = waveAmp * static_cast<double>(1.0f / 255.0f) * (512.0 - i) * static_cast<double>(1.0f / 196.0f);
                double normal = slope * static_cast<double>(3.1415f * 0.5f) / (std::abs(slope) + 1.0) + UnrealPi * 0.5;
                int32_t light = static_cast<int32_t>((256 - phongRange / 2) * std::cos(normal - lamp));
                double reflected = static_cast<double>(static_cast<float>(normal)) * 2.0 - lamp - viewer;
                if (reflected * reflected < phongRadius * phongRadius) {
                    light += static_cast<int32_t>((phongRadius - std::abs(reflected)) * (phongRange * 2) / phongRadius);
                }
                renderTable[i] = u8(std::clamp(light, 0, 255));
            }
        }

        void refreshBitmap(std::vector<uint8_t>& target, const std::vector<uint8_t>& data, int32_t width, int32_t height, bool allowUpsample)
        {
            if (target.empty() || width <= 0 || height <= 0) {
                return;
            }
            int32_t sourceUBits = logTwo(width);
            int32_t sourceVBits = logTwo(height);
            if (sourceUBits == ubits && sourceVBits == vbits && data.size() >= pixels.size()) {
                target.assign(data.begin(), data.begin() + static_cast<std::ptrdiff_t>(pixels.size()));
            } else if (allowUpsample && ubits - sourceUBits >= 0 && vbits - sourceVBits >= 0) {
                TextureMip mip;
                mip.width = static_cast<uint32_t>(width);
                mip.height = static_cast<uint32_t>(height);
                mip.data = data;
                target = upsample(mip, usize, vsize, ubits - sourceUBits, vbits - sourceVBits);
            }
        }

        void initWet()
        {
            UObject* sourceObject = wettexture::SourceTexture(texture);
            const TextureMip* source = bitmapOf(sourceObject);
            if (!source) {
                return;
            }
            int32_t sourceUBits = logTwo(static_cast<int32_t>(source->width));
            int32_t sourceVBits = logTwo(static_cast<int32_t>(source->height));
            if (sourceUBits == ubits && sourceVBits == vbits) {
                sourceBitmap.assign(source->data.begin(), source->data.begin() + static_cast<std::ptrdiff_t>(pixels.size()));
            } else if (ubits - sourceUBits >= 0 && vbits - sourceVBits >= 0) {
                sourceBitmap = upsample(*source, usize, vsize, ubits - sourceUBits, vbits - sourceVBits);
            } else {
                return;
            }
            UPalette* sourcePalette = cast<UTexture>(sourceObject)->palette();
            if (sourcePalette) {
                palette = sourcePalette;
            }
            int32_t waveAmp = watertexture::WaveAmp(texture);
            for (int32_t i = 0; i < 1024; i++) {
                int32_t light = static_cast<int32_t>(static_cast<float>(i - 511) * (static_cast<float>(waveAmp) / 512.0f));
                renderTable[i] = u8(std::clamp(light, -128, 127));
            }
        }

        void initIce()
        {
            masterCount = icetexture::MasterCount(texture);
            uDisplace = icetexture::UDisplace(texture);
            vDisplace = icetexture::VDisplace(texture);
            uPosition = icetexture::UPosition(texture);
            vPosition = icetexture::VPosition(texture);
            UObject* sourceObject = icetexture::SourceTexture(texture);
            UObject* glassObject = icetexture::GlassTexture(texture);
            const TextureMip* source = bitmapOf(sourceObject);
            if (!source || usize < 8 || vsize < 8) {
                return;
            }
            const TextureMip* glass = glassObject ? bitmapOf(glassObject) : nullptr;
            bool noSource = static_cast<int32_t>(source->width) != usize || static_cast<int32_t>(source->height) != vsize;
            bool noGlass = !glass || static_cast<int32_t>(glass->width) != usize || static_cast<int32_t>(glass->height) != vsize;
            UPalette* sourcePalette = cast<UTexture>(sourceObject)->palette();
            if (noSource || noGlass) {
                int32_t uscaler = ubits - logTwo(static_cast<int32_t>(source->width));
                int32_t vscaler = vbits - logTwo(static_cast<int32_t>(source->height));
                if (uscaler < 0 || vscaler < 0) {
                    return;
                }
                pixels = upsample(*source, usize, vsize, uscaler, vscaler);
            } else {
                sourceBitmap.assign(source->data.begin(), source->data.begin() + static_cast<std::ptrdiff_t>(pixels.size()));
                glassBitmap.assign(glass->data.begin(), glass->data.begin() + static_cast<std::ptrdiff_t>(pixels.size()));
            }
            if (sourcePalette) {
                palette = sourcePalette;
            }
        }

        bool tick(double time)
        {
            if (started && time == lastTime) {
                return false;
            }
            float delta = started ? static_cast<float>(time - lastTime) : 0.0f;
            started = true;
            lastTime = time;
            if (!valid) {
                return false;
            }
            if (kind == Kind::Ice && icetexture::TimeMethod(texture) != 0) {
                renderIce(delta);
                return true;
            }
            bool ticked = false;
            while (primeCurrent < texture::PrimeCount(texture)) {
                primeCurrent++;
                constantTimeTick();
                ticked = true;
            }
            float maxFrameRate = texture::MaxFrameRate(texture);
            if (maxFrameRate == 0.0f) {
                constantTimeTick();
                return true;
            }
            float minTime = 1.0f / std::clamp(maxFrameRate, 0.01f, 100.0f);
            float maxTime = 1.0f / std::clamp(texture::MinFrameRate(texture), 0.01f, 100.0f);
            accumulator += delta;
            if (accumulator < minTime) {
                return ticked;
            }
            constantTimeTick();
            if (accumulator < maxTime) {
                accumulator = 0;
            } else {
                accumulator -= maxTime;
                if (accumulator > maxTime) {
                    accumulator = maxTime;
                }
            }
            return true;
        }

        void constantTimeTick()
        {
            switch (kind) {
            case Kind::Fire:
                if (usize >= 8 && vsize >= 8) {
                    redrawSparks();
                    calculateFire(firetexture::bRising(texture) ? 1 : 0);
                    postDrawSparks();
                }
                break;
            case Kind::Wave:
                if (usize >= 8 && vsize >= 8) {
                    waterRedrawDrops();
                    calculateWater();
                }
                break;
            case Kind::Wet:
                if (!sourceBitmap.empty() && usize >= 8 && vsize >= 8) {
                    waterRedrawDrops();
                    calculateWater();
                    applyWetTexture();
                }
                break;
            case Kind::Ice:
                renderIce(1.0f / 120.0f);
                break;
            case Kind::Water:
            case Kind::None:
                break;
            }
        }

        int32_t spot(int32_t x, int32_t y) const
        {
            return x + (y << ubits);
        }

        void plot(int32_t index, uint8_t value)
        {
            pixels[static_cast<size_t>(static_cast<uint32_t>(index)) & (pixels.size() - 1)] = value;
        }

        uint8_t& pixel(int32_t row, int32_t column)
        {
            return pixels[static_cast<size_t>((row & vmask) * usize + (column & umask))];
        }

        void moveSparkXY(FireSpark& spark, int32_t xspeed, int32_t yspeed)
        {
            if (xspeed < 0) {
                if ((speedRand() & 127) < -xspeed) {
                    spark.x = u8(umask & (spark.x - 1));
                }
            } else {
                if ((speedRand() & 127) < xspeed) {
                    spark.x = u8(umask & (spark.x + 1));
                }
            }
            if (yspeed < 0) {
                if ((speedRand() & 127) < -yspeed) {
                    spark.y = u8(vmask & (spark.y - 1));
                }
            } else {
                if ((speedRand() & 127) < yspeed) {
                    spark.y = u8(vmask & (spark.y + 1));
                }
            }
        }

        void moveSpark(FireSpark& spark)
        {
            moveSparkXY(spark, s8(spark.a), s8(spark.b));
        }

        void moveSparkAngle(FireSpark& spark, uint8_t angle)
        {
            int32_t xdir = s8(u8(-127 + s8(fractal.phase[angle])));
            int32_t ydir = s8(u8(-127 + s8(fractal.phase[u8(angle + 64)])));
            moveSparkXY(spark, xdir, ydir);
        }

        void moveSparkTwo(FireSpark& spark)
        {
            int32_t xspeed = s8(spark.a);
            if (xspeed < 0) {
                if ((speedRand() & 127) < -xspeed) {
                    spark.x = u8(umask & (spark.x - 1));
                }
            } else {
                if ((speedRand() & 127) < xspeed) {
                    spark.x = u8(umask & (spark.x + 1));
                }
            }
            spark.y = u8(vmask & (spark.y - 2));
        }

        void wander(FireSpark& spark)
        {
            if (speedRand() & 1) {
                spark.x = u8(umask & (spark.x + (speedRand() & 15) - 7));
            }
            if (speedRand() & 1) {
                spark.y = u8(vmask & (spark.y + (speedRand() & 15) - 7));
            }
        }

        void jitter(FireSpark& spark)
        {
            int32_t dx = speedRand() & 7;
            dx -= speedRand() & 7;
            spark.x = u8(umask & (spark.x + dx));
            int32_t dy = speedRand() & 7;
            dy -= speedRand() & 7;
            spark.y = u8(vmask & (spark.y + dy));
        }

        void drawFlashRamp(uint8_t xpos, uint8_t ypos, uint8_t xlen, uint8_t ylen, uint8_t color1, uint8_t color2)
        {
            if (((ylen & 1) && ylen * 2 >= xlen) || ((xlen & 1) && ylen * 2 < xlen)) {
                xpos = u8(xpos + ((xlen & 1) ? -xlen : xlen));
                ypos = u8(ypos + ((ylen & 1) ? -ylen : ylen));
                xlen = u8(xlen ^ 1);
                ylen = u8(ylen ^ 1);
                std::swap(color1, color2);
            }
            int32_t majorLen = 1 | std::max(xlen, ylen);
            uint8_t flashArray[256];
            int32_t flashPos = 0;
            for (int32_t flash = 0; flash < majorLen; flash++) {
                flashArray[flash] = speedRand();
                flashPos += flashArray[flash];
            }
            int32_t ystep = (ylen & 1) ? -1 : 1;
            int32_t realYlen = (ylen & 1) ? -ylen : ylen;
            int32_t xstep = (xlen & 1) ? -1 : 1;
            int32_t realXlen = (xlen & 1) ? -xlen : xlen;
            int32_t rampColor = color1 * (1 << 23);
            int32_t colorSlope = ((color2 - color1) * (1 << 23)) / majorLen;
            if (xlen >= ylen) {
                int32_t ypoz = ypos * 64;
                int32_t flashBias = (realYlen * 64 - flashPos) / majorLen;
                for (int32_t flash = 0; flash < xlen; flash++) {
                    ypoz += flashArray[flash] + flashBias;
                    int32_t dest = (((ypoz >> 6) & vmask) << ubits) + (xpos & umask);
                    xpos = u8(xpos + xstep);
                    rampColor += colorSlope;
                    plot(dest, u8(rampColor >> 23));
                }
            } else {
                int32_t xpoz = xpos * 64;
                int32_t flashBias = (realXlen * 64 - flashPos) / majorLen;
                for (int32_t flash = 0; flash < ylen; flash++) {
                    xpoz += flashArray[flash] + flashBias;
                    int32_t dest = ((ypos & vmask) << ubits) + ((xpoz >> 6) & umask);
                    ypos = u8(ypos + ystep);
                    rampColor += colorSlope;
                    plot(dest, u8(rampColor >> 23));
                }
            }
        }

        FireSpark& spawn()
        {
            return sparks[static_cast<size_t>(activeSparks++)];
        }

        bool canSpawn() const
        {
            return activeSparks < sparksLimit;
        }

        void removeSpark(FireSpark& spark)
        {
            spark = sparks[static_cast<size_t>(--activeSparks)];
        }

        void redrawSparks()
        {
            auxPhase = u8(auxPhase + firetexture::FX_Frequency(texture));
            globalPhase++;
            const uint8_t* phase = fractal.phase;
            for (int32_t s = 0; s < activeSparks; s++) {
                FireSpark& spark = sparks[static_cast<size_t>(s)];
                switch (spark.type) {
                case SparkBurn:
                    plot(spot(spark.x, spark.y), speedRand());
                    break;
                case SparkSparkle: {
                    int32_t x = spark.x + ((speedRand() * spark.a) >> 8);
                    int32_t y = spark.y + ((speedRand() * spark.b) >> 8);
                    plot((umask & x) + ((vmask & y) << ubits), spark.heat);
                    break;
                }
                case SparkPulse:
                    plot(spot(spark.x, spark.y), spark.heat);
                    spark.heat = u8(spark.heat + spark.d);
                    break;
                case SparkSignal:
                    if (spark.heat > spark.c) {
                        plot(spot(spark.x, spark.y), spark.heat);
                    }
                    spark.heat = u8(spark.heat + spark.d);
                    if (spark.heat < spark.d) {
                        spark.heat = speedRand();
                    }
                    break;
                case SparkCylinder: {
                    uint8_t heat = u8(std::min(phase[(spark.a + 64) & 255] + spark.heat, 255));
                    uint8_t x = u8(spark.x + ((phase[spark.a] * spark.b) >> 8));
                    plot((x & umask) + (spark.y << ubits), heat);
                    spark.a = u8(spark.a + spark.d);
                    break;
                }
                case SparkCylinder3D:
                    if (((spark.a + 64) & 255) < 128) {
                        uint8_t heat = u8(std::min(phase[(spark.a + 64) & 255] + spark.heat, 255));
                        uint8_t x = u8(spark.x + ((phase[spark.a] * spark.b) >> 8));
                        plot((x & umask) + (spark.y << ubits), heat);
                    }
                    spark.a = u8(spark.a + spark.d);
                    break;
                case SparkJugglers: {
                    uint8_t heat = u8(std::min(phase[(spark.a + 64) & 255] + spark.heat, 255));
                    uint8_t y = u8(spark.y + ((phase[spark.a] * spark.b) >> 8));
                    plot(spark.x + ((y & vmask) << ubits), heat);
                    spark.a = u8(spark.a + spark.d);
                    break;
                }
                case SparkLissajous: {
                    uint8_t heat = fractal.lightPhase[u8(spark.a + 64)];
                    uint8_t x = u8(spark.x + ((phase[spark.a] * spark.heat) >> 8));
                    uint8_t y = u8(spark.y + ((phase[spark.b] * spark.heat) >> 8));
                    plot((x & umask) + ((y & vmask) << ubits), heat);
                    spark.a = u8(spark.a + spark.c);
                    spark.b = u8(spark.b + spark.d);
                    break;
                }
                case SparkLissajX: {
                    uint8_t x = u8(spark.x + ((phase[spark.a] * spark.heat) >> 8));
                    uint8_t heat = fractal.lightPhase[u8(spark.a + 64)];
                    plot((x & umask) + (spark.y << ubits), heat);
                    spark.a = u8(spark.a + spark.c);
                    break;
                }
                case SparkLissajY: {
                    uint8_t y = u8(spark.y + ((phase[spark.b] * spark.heat) >> 8));
                    uint8_t heat = fractal.lightPhase[u8(spark.b + 64)];
                    plot(spark.x + ((y & vmask) << ubits), heat);
                    spark.b = u8(spark.b + spark.d);
                    break;
                }
                case SparkBlaze:
                    if (canSpawn() && 128 > speedRand()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkDrifter;
                        spawned.heat = spark.heat;
                        spawned.x = spark.x;
                        spawned.y = spark.y;
                        spawned.a = speedRand();
                        spawned.b = speedRand();
                        spawned.c = spark.c;
                        spawned.d = spark.d;
                    }
                    break;
                case SparkOzHasSpoken:
                    if (canSpawn() && 128 > speedRand()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkDriftSlow;
                        spawned.heat = spark.heat;
                        spawned.x = spark.x;
                        spawned.y = spark.y;
                        spawned.a = u8((speedRand() & 127) - 63);
                        spawned.b = u8(-127);
                        spawned.d = 2;
                    }
                    break;
                case SparkCone:
                    if (canSpawn() && 64 > speedRand()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkFaller;
                        spawned.heat = spark.heat;
                        spawned.x = spark.x;
                        spawned.y = spark.y;
                        spawned.a = u8((speedRand() & 127) - 63);
                        spawned.b = 0;
                        spawned.c = 50;
                    }
                    break;
                case SparkBlazeRight:
                    if (canSpawn() && 64 > speedRand()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkFaller;
                        spawned.heat = spark.heat;
                        spawned.x = spark.x;
                        spawned.y = spark.y;
                        spawned.a = u8((speedRand() & 63) + 63);
                        spawned.b = u8(-29);
                        spawned.c = spark.c;
                    }
                    break;
                case SparkBlazeLeft:
                    if (canSpawn() && 64 > speedRand()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkFaller;
                        spawned.heat = spark.heat;
                        spawned.x = spark.x;
                        spawned.y = spark.y;
                        spawned.a = u8((speedRand() & 63) - 128);
                        spawned.b = u8(-29);
                        spawned.c = spark.c;
                    }
                    break;
                case SparkEmit:
                case SparkFountain:
                    if (canSpawn() && 64 > speedRand()) {
                        FireSpark& spawned = spawn();
                        spawned.type = spark.type == SparkEmit ? SparkDriftSlow : SparkGraviton;
                        spawned.heat = spark.heat;
                        spawned.x = spark.x;
                        spawned.y = spark.y;
                        spawned.a = spark.a;
                        spawned.b = spark.b;
                        spawned.d = spark.d;
                    }
                    break;
                case SparkOrganic:
                    if (canSpawn() && 128 > speedRand()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkVShooter;
                        spawned.x = u8(umask & (spark.x + ((speedRand() * spark.c) >> 8)));
                        spawned.y = u8(vmask & (spark.y + ((speedRand() * spark.c) >> 8)));
                        spawned.a = u8(speedRand() - 127);
                        spawned.b = u8(256 - 127);
                        spawned.c = 255;
                    }
                    break;
                case SparkWanderOrganic:
                case SparkRandomCloud:
                    if (canSpawn()) {
                        FireSpark& spawned = spawn();
                        spawned.type = spark.type == SparkWanderOrganic ? SparkVShooter : SparkDrop;
                        spawned.x = u8(umask & (spark.x + (speedRand() & 31)));
                        spawned.y = u8(vmask & (spark.y + (speedRand() & 31)));
                        if (spark.type == SparkWanderOrganic) {
                            spawned.a = u8(speedRand() - 127);
                            spawned.b = u8(256 - 127);
                            spawned.c = 255;
                        } else {
                            spawned.a = u8((speedRand() & 31) - 15);
                            spawned.b = u8(256 - 127);
                            spawned.c = 0;
                        }
                    }
                    wander(spark);
                    break;
                case SparkEels:
                    if (speedRand() < 20 && canSpawn()) {
                        FireSpark& spawned = spawn();
                        spawned.heat = spark.heat;
                        spawned.type = SparkSpawnedEel;
                        spawned.x = u8(umask & (spark.x + (speedRand() & 31)));
                        spawned.y = u8(vmask & (spark.y + (speedRand() & 31)));
                        spawned.a = speedRand();
                        spawned.b = speedRand();
                        spawned.c = spark.c;
                    }
                    wander(spark);
                    break;
                case SparkGametes:
                    if (speedRand() < 20 && canSpawn()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkSpawnedSperm;
                        spawned.heat = spark.heat;
                        spawned.x = u8(umask & (spark.x + (speedRand() & 31)));
                        spawned.y = u8(vmask & (spark.y + (speedRand() & 31)));
                        spawned.a = speedRand();
                        spawned.c = spark.c;
                        spawned.d = speedRand();
                    }
                    wander(spark);
                    break;
                case SparkCustomCloud:
                    if (canSpawn()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkMove;
                        spawned.x = u8(umask & (spark.x + (speedRand() & 31)));
                        spawned.y = u8(vmask & (spark.y + (speedRand() & 31)));
                        spawned.a = spark.a;
                        spawned.b = spark.b;
                        spawned.c = spark.d;
                    }
                    jitter(spark);
                    break;
                case SparkLocalCloud:
                    if (canSpawn()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkMove;
                        spawned.x = u8(umask & (spark.x + ((speedRand() * spark.c) >> 8)));
                        spawned.y = u8(vmask & (spark.y + ((speedRand() * spark.c) >> 8)));
                        spawned.a = spark.a;
                        spawned.b = spark.b;
                        spawned.c = spark.d;
                    }
                    break;
                case SparkFlocks:
                    if (canSpawn()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkSpawnedTwirl;
                        spawned.x = u8(umask & (spark.x + (speedRand() & 31)));
                        spawned.y = u8(vmask & (spark.y + (speedRand() & 31)));
                        spawned.b = spark.a;
                        spawned.a = 0;
                        spawned.c = spark.b;
                        spawned.d = spark.d;
                        spawned.heat = spark.heat;
                        spark.a = u8(spark.a + spark.c);
                    }
                    jitter(spark);
                    break;
                case SparkWheel:
                    if (canSpawn()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkSpawnedTwirl;
                        spawned.x = spark.x;
                        spawned.y = spark.y;
                        spawned.b = spark.a;
                        spawned.a = 0;
                        spawned.c = spark.b;
                        spawned.d = spark.d;
                        spawned.heat = spark.heat;
                    }
                    spark.a = u8(spark.a + spark.c);
                    break;
                case SparkSprinkler:
                    if (canSpawn()) {
                        FireSpark& spawned = spawn();
                        spawned.type = SparkSprinklerTwirl;
                        spawned.x = spark.x;
                        spawned.y = spark.y;
                        spawned.heat = spark.heat;
                        spawned.a = spark.a;
                        spawned.b = spark.b;
                        spawned.c = spark.c;
                        spawned.d = 2;
                    }
                    spark.a = u8(spark.a + spark.d);
                    break;
                case SparkStars:
                    plot(spot(spark.x, spark.y), spark.b);
                    break;
                case SparkLineLightning:
                case SparkRampLightning:
                    if (spark.heat == 0) {
                        break;
                    }
                    if (spark.c > 0) {
                        spark.c--;
                        uint8_t heat = spark.heat;
                        uint8_t tail = spark.type == SparkLineLightning ? heat : u8(heat >> 3);
                        drawFlashRamp(spark.x, spark.y, spark.a, spark.b, heat, tail);
                    } else if (speedRand() >= spark.d) {
                        spark.c = u8((1 + speedRand()) & 5);
                    }
                    break;
                case SparkSphereLightning:
                    if (speedRand() >= spark.d) {
                        int32_t angle = speedRand();
                        int32_t radius = spark.c;
                        uint8_t color1 = spark.heat;
                        uint8_t color2 = u8(spark.heat >> 2);
                        int32_t xdisp = (radius * phase[angle]) >> 8;
                        int32_t ydisp = (radius * phase[(angle + 64) & 255]) >> 8;
                        int32_t xlen = xdisp - radius / 2;
                        int32_t ylen = ydisp - radius / 2;
                        if (xlen <= 0) {
                            xlen = (-xlen) | 1;
                        } else {
                            xlen &= ~1;
                        }
                        if (ylen <= 0) {
                            ylen = (-ylen) | 1;
                        } else {
                            ylen &= ~1;
                        }
                        drawFlashRamp(spark.x, spark.y, u8(xlen), u8(ylen), color1, color2);
                    }
                    break;
                case SparkDrifter:
                    spark.heat = u8(spark.heat - 5);
                    if (spark.heat < 251) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        moveSpark(spark);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkDriftSlow:
                    spark.heat = u8(spark.heat - spark.d);
                    if (spark.heat > spark.d) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        moveSpark(spark);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkCustom:
                    spark.heat = u8(spark.heat - spark.c);
                    if (spark.heat < 250) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        moveSpark(spark);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkFaller:
                    spark.c = u8(spark.c - 1);
                    if (spark.c > 0) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        moveSpark(spark);
                        if (s8(spark.b) < 122) {
                            spark.b = u8(spark.b + 3);
                        }
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkGraviton:
                    spark.heat = u8(spark.heat - spark.d);
                    if (spark.heat > LowestGlow) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        moveSpark(spark);
                        if (globalPhase & 1) {
                            if (s8(spark.b) < 124) {
                                spark.b = u8(spark.b + 3);
                            }
                        }
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkVShooter:
                    spark.c = u8(spark.c - 3);
                    if (spark.c > 190) {
                        plot(spot(spark.x, spark.y), spark.c);
                        moveSparkTwo(spark);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkDrop:
                    spark.c = u8(spark.c + 4);
                    if (spark.c < 250) {
                        plot(spot(spark.x, spark.y), spark.c);
                        moveSparkTwo(spark);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkMove:
                    spark.c = u8(spark.c + 4);
                    if (spark.c < 250) {
                        plot(spot(spark.x, spark.y), spark.c);
                        moveSpark(spark);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkSpawnedEel:
                    spark.c = u8(spark.c - 1);
                    if (spark.c < 255) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        moveSpark(spark);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkSpawnedSperm:
                    spark.c = u8(spark.c - 1);
                    if (spark.c < 255) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        spark.a = u8(spark.a + 7);
                        int32_t sawTooth = 127 & spark.a;
                        if (sawTooth > 63) {
                            sawTooth = 127 - sawTooth;
                        }
                        moveSparkAngle(spark, u8(255 & (sawTooth + spark.d)));
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkSpawnedTwirl:
                    spark.c = u8(spark.c - 1);
                    if (spark.c < 255) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        int32_t xspeed = s8(fractal.signedPhase[(spark.b + 64) & 255]);
                        int32_t yspeed = s8(fractal.signedPhase[spark.b]);
                        int32_t angle = (spark.a | (spark.b << 8)) + (spark.d << 4);
                        spark.a = u8(angle & 255);
                        spark.b = u8((angle >> 8) & 255);
                        moveSparkXY(spark, xspeed, yspeed);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                case SparkSprinklerTwirl:
                    spark.c = u8(spark.c - 1);
                    if (spark.c < 255) {
                        plot(spot(spark.x, spark.y), spark.heat);
                        int32_t xspeed = s8(u8(-128 + s8(phase[(spark.a + 64) & 255])));
                        int32_t yspeed = s8(spark.b);
                        spark.a = u8(spark.a + spark.d);
                        moveSparkXY(spark, xspeed, yspeed);
                    } else {
                        removeSpark(spark);
                    }
                    break;
                default:
                    break;
                }
            }
        }

        void postDrawSparks()
        {
            if (starStatus == 0) {
                return;
            }
            bool foundStar = false;
            for (int32_t s = 0; s < activeSparks; s++) {
                FireSpark& spark = sparks[static_cast<size_t>(s)];
                if (spark.type == SparkStars) {
                    foundStar = true;
                    size_t index = static_cast<size_t>(spot(spark.x, spark.y)) & (pixels.size() - 1);
                    uint8_t dimStar = pixels[index];
                    spark.b = dimStar;
                    if (dimStar < StarCutoff) {
                        pixels[index] = spark.a;
                    }
                }
            }
            if (!foundStar) {
                starStatus = 0;
            }
        }

        void calculateFire(int32_t spread)
        {
            previous = pixels;
            for (int32_t y = 0; y < vsize; y++) {
                uint8_t* line = &pixels[static_cast<size_t>(y * usize)];
                const uint8_t* row = &previous[static_cast<size_t>(((y + spread) & vmask) * usize)];
                const uint8_t* lower = &previous[static_cast<size_t>(((y + spread + 1) & vmask) * usize)];
                for (int32_t x = 0; x < usize; x++) {
                    line[x] = renderTable[row[x] + row[(x - 1) & umask] + row[(x + 1) & umask] + lower[x]];
                }
            }
        }

        void setField(int32_t index, uint8_t depthA, uint8_t depthB)
        {
            size_t mask = fields.size() - 1;
            fields[static_cast<size_t>(static_cast<uint32_t>(index)) & mask] = depthA;
            fields[static_cast<size_t>(static_cast<uint32_t>(index + usize / 2)) & mask] = depthB;
        }

        void waterRedrawDrops()
        {
            int32_t u2mask = u8(umask >> 1);
            int32_t v2mask = u8(vmask >> 1);
            const uint8_t* phase = fractal.phase;
            globalPhase++;
            for (int32_t s = 0; s < numDrops; s++) {
                WaterDrop& drop = drops[static_cast<size_t>(s)];
                int32_t xnow = drop.x;
                int32_t ynow = drop.y;
                int32_t dropDest = (ynow << ubits) + xnow;
                switch (drop.type) {
                case DropFixedDepth:
                    setField(dropDest, drop.d, drop.d);
                    break;
                case DropPhaseSpot:
                    drop.depth = u8(drop.depth + drop.d);
                    setField(dropDest, phase[drop.depth], phase[drop.depth]);
                    break;
                case DropHalfAmpl: {
                    drop.depth = u8(drop.depth + drop.d);
                    uint8_t depth = std::max(phase[drop.depth], static_cast<uint8_t>(128));
                    setField(dropDest, depth, depth);
                    break;
                }
                case DropShallowSpot: {
                    drop.depth = u8(drop.depth + drop.d);
                    uint8_t depth = u8(64 + (phase[drop.depth] >> 1));
                    setField(dropDest, depth, depth);
                    break;
                }
                case DropRandomMover: {
                    int32_t x = xnow - (speedRand() & 3);
                    x += speedRand() & 3;
                    drop.x = u8(u2mask & x);
                    int32_t y = ynow - (speedRand() & 3);
                    y += speedRand() & 3;
                    drop.y = u8(v2mask & y);
                    setField(dropDest, 128 + 57, 128 - 57);
                    break;
                }
                case DropFixedRandomSpot: {
                    uint8_t depthA = speedRand();
                    uint8_t depthB = speedRand();
                    setField(dropDest, depthA, depthB);
                    break;
                }
                case DropWhirlyThing:
                case DropWhirlyBack:
                case DropBigWhirly:
                case DropBigWhirlyBack: {
                    int32_t phase16 = (drop.b << 8) + drop.a;
                    int32_t delta = (drop.d << 8) + drop.c;
                    bool back = drop.type == DropWhirlyBack || drop.type == DropBigWhirlyBack;
                    phase16 = 0xffff & (back ? phase16 - delta : phase16 + delta);
                    drop.a = u8(phase16 & 0xff);
                    drop.b = u8((phase16 >> 8) & 0xff);
                    int32_t shift = drop.type == DropWhirlyThing || drop.type == DropWhirlyBack ? 4 : 3;
                    int32_t x = u2mask & (xnow + (phase[drop.b] >> shift));
                    int32_t y = (v2mask & (ynow + (phase[(drop.b + 64) & 255] >> shift))) << ubits;
                    setField(x + y, drop.depth, drop.depth);
                    break;
                }
                case DropHorizontalLine:
                case DropVerticalLine:
                case DropDiagonalLine1:
                case DropDiagonalLine2:
                case DropHorizontalOsc:
                case DropVerticalOsc:
                case DropDiagonalOsc1:
                case DropDiagonalOsc2: {
                    uint8_t depth = drop.depth;
                    int32_t lineType = drop.type - DropHorizontalLine;
                    if (drop.type >= DropHorizontalOsc) {
                        drop.depth = u8(drop.depth + drop.c);
                        depth = phase[drop.depth];
                        lineType = drop.type - DropHorizontalOsc;
                    }
                    int32_t segSize = drop.d >> 1;
                    for (int32_t t = 0; t <= segSize; t++) {
                        int32_t dest = 0;
                        if (lineType == 0) {
                            dest = (ynow << ubits) + ((t + xnow) & u2mask);
                        } else if (lineType == 1) {
                            dest = xnow + (((ynow + t) & v2mask) << ubits);
                        } else if (lineType == 2) {
                            dest = (((ynow + t) & v2mask) << ubits) + ((xnow - t) & u2mask);
                        } else {
                            dest = (((ynow + t) & v2mask) << ubits) + ((xnow + t) & u2mask);
                        }
                        setField(dest, depth, depth);
                    }
                    break;
                }
                case DropAreaClamp: {
                    int32_t segSize = drop.d >> 1;
                    for (int32_t v = 0; v < segSize; v++) {
                        int32_t destY = ((ynow + v) & v2mask) << ubits;
                        int32_t dest = destY + (xnow & u2mask);
                        for (int32_t u = 1; u <= segSize; u++) {
                            setField(dest, drop.depth, drop.depth);
                            dest = destY + ((u + xnow) & u2mask);
                        }
                    }
                    break;
                }
                case DropRainDrops:
                    if ((speedRand() & 15) == 0) {
                        int32_t spray = drop.d;
                        int32_t y = ((ynow + ((speedRand() * spray) >> 8)) & v2mask) << ubits;
                        int32_t x = (xnow + ((speedRand() * spray) >> 8)) & u2mask;
                        setField(x + y, drop.depth, u8(255 ^ drop.depth));
                    }
                    break;
                case DropLeakyTap:
                case DropDrippyTap:
                    drop.a = u8(drop.a + drop.d);
                    if (drop.a <= drop.d) {
                        if (drop.type == DropDrippyTap) {
                            drop.a = speedRand();
                        }
                        setField(dropDest, drop.depth, u8(255 ^ drop.depth));
                    }
                    break;
                default:
                    break;
                }
            }
        }

        void calculateWater()
        {
            int32_t xdim = usize / 2;
            int32_t ydim = vsize / 2;
            int32_t lines = ydim * 2;
            waveParity++;
            auto cell = [&](int32_t line, int32_t x) -> uint8_t& {
                return fields[static_cast<size_t>(((line + lines) % lines) * xdim + (x + xdim) % xdim)];
            };
            const uint8_t* wave = fractal.wave + 512;
            const uint8_t* table = renderTable + 512;
            bool even = (waveParity & 1) == 0;
            for (int32_t y = 0; y < ydim; y++) {
                int32_t line = even ? y * 2 : y * 2 + 1;
                int32_t base = even ? -3 : -2;
                for (int32_t x = 0; x < xdim; x++) {
                    int32_t a = cell(line - 1, x + base);
                    int32_t c = cell(line - 1, x + base + 1);
                    int32_t e = cell(line - 1, x + base + 2);
                    int32_t g = cell(line - 1, x + base + 3);
                    int32_t b = cell(line + 1, x + base);
                    int32_t d = cell(line + 1, x + base + 1);
                    int32_t f = cell(line + 1, x + base + 2);
                    int32_t h = cell(line + 1, x + base + 3);
                    uint8_t& dest = cell(line, x);
                    dest = wave[(e + g + f + h) - (dest << 1)];
                    int32_t ea = e - a;
                    int32_t fb = f - b;
                    int32_t gc = g - c;
                    int32_t hd = h - d;
                    int32_t row = even ? y * 2 - 1 : y * 2;
                    int32_t column = even ? x * 2 - 1 : x * 2;
                    int32_t right = (gc + hd) >> 1;
                    pixel(row, column + 1) = table[right * 2];
                    pixel(row + 1, column) = table[fb + hd];
                    pixel(row, column) = table[right + ((fb + ea) >> 1)];
                    pixel(row + 1, column + 1) = table[hd + hd];
                }
            }
        }

        void applyWetTexture()
        {
            for (int32_t v = 0; v < vsize; v++) {
                uint8_t* line = &pixels[static_cast<size_t>(v << ubits)];
                const uint8_t* source = &sourceBitmap[static_cast<size_t>(v << ubits)];
                for (int32_t u = 0; u < usize; u++) {
                    line[u] = source[(u + line[u]) & umask];
                }
            }
        }

        void moveIcePosition(float deltaTime)
        {
            double delta = static_cast<double>(deltaTime);
            double amplitude = icetexture::Amplitude(texture) + 1;
            double frequency = icetexture::Frequency(texture) + 1;
            double uspeed = s8(u8(icetexture::HorizPanSpeed(texture) + 128));
            double vspeed = s8(u8(icetexture::VertPanSpeed(texture) + 128));
            masterCount = static_cast<float>(delta * 120.0 + static_cast<double>(masterCount));
            uDisplace = static_cast<float>(static_cast<double>(uDisplace) - uspeed * delta * 2.0);
            vDisplace = static_cast<float>(vspeed * delta * 2.0 + static_cast<double>(vDisplace));
            double phase = frequency * static_cast<double>(masterCount);
            double fast = phase * static_cast<double>(0.0012f);
            double slow = phase * static_cast<double>(0.0011f);
            switch (icetexture::PanningStyle(texture)) {
            case 0:
                uPosition = uDisplace;
                vPosition = vDisplace;
                break;
            case 1:
                uPosition = static_cast<float>(appRound(static_cast<float>(amplitude * std::sin(fast)))) + uDisplace;
                vPosition = static_cast<float>(appRound(static_cast<float>(amplitude * std::cos(fast)))) + vDisplace;
                break;
            case 2:
                uPosition = static_cast<float>(appRound(static_cast<float>(amplitude * std::sin(fast)))) + uDisplace;
                vPosition = static_cast<float>(appRound(static_cast<float>(amplitude * std::cos(slow)))) + vDisplace;
                break;
            case 3:
                uPosition = static_cast<float>(appRound(static_cast<float>(amplitude * std::sin(fast) * 0.5))) + uDisplace;
                vPosition = vDisplace;
                break;
            case 4:
                uPosition = uDisplace;
                vPosition = static_cast<float>(appRound(static_cast<float>(amplitude * std::cos(fast) * 0.5))) + vDisplace;
                break;
            default:
                break;
            }
        }

        void renderIce(float deltaTime)
        {
            if (glassBitmap.empty() || sourceBitmap.empty()) {
                return;
            }
            moveIcePosition(deltaTime);
            blitIce(icetexture::MoveIce(texture) != 0);
        }

        void blitIce(bool moveIce)
        {
            int32_t udisp = appRound(uPosition) & umask;
            int32_t vdisp = appRound(vPosition) & vmask;
            for (int32_t v = 0; v < vsize; v++) {
                uint8_t* line = &pixels[static_cast<size_t>(v << ubits)];
                if (moveIce) {
                    const uint8_t* tex = &sourceBitmap[static_cast<size_t>(v << ubits)];
                    const uint8_t* glass = &glassBitmap[static_cast<size_t>(((v + vdisp) & vmask) << ubits)];
                    for (int32_t u = 0; u < usize; u++) {
                        line[u] = tex[(u + glass[(u + udisp) & umask]) & umask];
                    }
                } else {
                    const uint8_t* tex = &sourceBitmap[static_cast<size_t>(((v + vdisp) & vmask) << ubits)];
                    const uint8_t* glass = &glassBitmap[static_cast<size_t>(v << ubits)];
                    for (int32_t u = 0; u < usize; u++) {
                        line[u] = tex[(u + udisp + glass[u]) & umask];
                    }
                }
            }
        }
    };

    ProceduralTextures::ProceduralTextures() = default;

    ProceduralTextures::~ProceduralTextures() = default;

    bool ProceduralTextures::isProcedural(UTexture* texture)
    {
        return kindOf(texture) != Kind::None;
    }

    void ProceduralTextures::clear()
    {
        states.clear();
    }

    bool ProceduralTextures::update(UTexture* texture, double time, std::vector<uint8_t>& rgba, uint32_t& width, uint32_t& height)
    {
        auto& slot = states[texture];
        bool created = false;
        if (!slot) {
            slot = std::make_unique<ProceduralState>();
            slot->init(texture);
            created = true;
        }
        ProceduralState& state = *slot;
        if (state.kind == Kind::Wet || state.kind == Kind::Ice) {
            UObject* sources[2] = { state.kind == Kind::Wet ? wettexture::SourceTexture(texture) : icetexture::SourceTexture(texture), state.kind == Kind::Ice ? icetexture::GlassTexture(texture) : nullptr };
            for (int i = 0; i < 2; i++) {
                UTexture* source = cast<UTexture>(sources[i]);
                if (!source || source == texture || !isProcedural(source)) {
                    continue;
                }
                std::vector<uint8_t> scratch;
                uint32_t sourceWidth = 0;
                uint32_t sourceHeight = 0;
                update(source, time, scratch, sourceWidth, sourceHeight);
                const ProceduralState& sourceState = *states[source];
                state.refreshBitmap(i == 0 ? state.sourceBitmap : state.glassBitmap, sourceState.pixels, sourceState.usize, sourceState.vsize, state.kind == Kind::Wet);
            }
        }
        bool changed = state.tick(time);
        if (!changed && !created) {
            return false;
        }

        width = static_cast<uint32_t>(state.usize);
        height = static_cast<uint32_t>(state.vsize);
        rgba.resize(state.pixels.size() * 4);
        UPalette* palette = state.palette;
        const uint32_t* colors = palette && palette->colors.size() >= 256 ? palette->colors.data() : nullptr;
        for (size_t i = 0; i < state.pixels.size(); i++) {
            uint8_t index = state.pixels[i];
            uint32_t color = colors ? colors[index] : (0xff000000u | index * 0x010101u);
            rgba[i * 4 + 0] = static_cast<uint8_t>(color & 0xff);
            rgba[i * 4 + 1] = static_cast<uint8_t>((color >> 8) & 0xff);
            rgba[i * 4 + 2] = static_cast<uint8_t>((color >> 16) & 0xff);
            rgba[i * 4 + 3] = index == 0 ? 0 : 255;
        }
        return true;
    }
}
