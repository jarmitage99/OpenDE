#include "render/lighting.h"

#include "core/objects.h"
#include "engine/props_engine.h"
#include "engine/resources.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <random>

namespace dx {
    namespace {
        const Name nameZoneInfo("ZoneInfo");

        constexpr double Pi64 = 3.1415926535897932;

        constexpr uint8_t LT_None = 0;
        constexpr uint8_t LT_Steady = 1;
        constexpr uint8_t LT_Pulse = 2;
        constexpr uint8_t LT_Blink = 3;
        constexpr uint8_t LT_Flicker = 4;
        constexpr uint8_t LT_Strobe = 5;
        constexpr uint8_t LT_SubtlePulse = 7;
        constexpr uint8_t LT_TexturePaletteOnce = 8;
        constexpr uint8_t LT_TexturePaletteLoop = 9;

        constexpr uint8_t LE_Searchlight = 4;
        constexpr uint8_t LE_SlowWave = 5;
        constexpr uint8_t LE_FastWave = 6;
        constexpr uint8_t LE_CloudCast = 7;
        constexpr uint8_t LE_StaticSpot = 8;
        constexpr uint8_t LE_Shock = 9;
        constexpr uint8_t LE_Disco = 10;
        constexpr uint8_t LE_Spotlight = 12;
        constexpr uint8_t LE_NonIncidence = 13;
        constexpr uint8_t LE_Shell = 14;
        constexpr uint8_t LE_OmniBumpMap = 15;
        constexpr uint8_t LE_Interference = 16;
        constexpr uint8_t LE_Cylinder = 17;
        constexpr uint8_t LE_Rotor = 18;
        constexpr uint8_t LE_Max = 20;

        constexpr int32_t RandomCount = 256;
        constexpr int32_t RandomMask = RandomCount - 1;
        constexpr int32_t RandomCycle = 16;

        struct LightRandoms {
            std::array<float, RandomCount> bases {};
            std::array<float, RandomCount> randoms {};
            std::array<float, RandomCount> deltas {};
            uint32_t lastTicks = 0;
            std::mt19937 generator { 12345 };
        };

        LightRandoms& lightRandoms()
        {
            static LightRandoms state;
            return state;
        }

        float frand(LightRandoms& state)
        {
            return std::uniform_real_distribution<float>(0.0f, 1.0f)(state.generator);
        }

        const std::array<float, 16384>& trigTable()
        {
            static const std::array<float, 16384> table = [] {
                std::array<float, 16384> values {};
                for (size_t i = 0; i < values.size(); i++) {
                    values[i] = static_cast<float>(std::sin(static_cast<double>(i) * 2.0 * Pi64 / 16384.0));
                }
                return values;
            }();
            return table;
        }

        const std::array<float, 2048>& sqrtTable()
        {
            static const std::array<float, 2048> table = [] {
                std::array<float, 2048> values {};
                for (uint32_t d = 0; d < 1024; d++) {
                    uint32_t bits = (std::bit_cast<uint32_t>(1.0f) & 0xff800000u) + (d << 13);
                    float root = static_cast<float>(std::sqrt(static_cast<double>(std::bit_cast<float>(bits))));
                    root = std::bit_cast<float>(std::bit_cast<uint32_t>(root) - (64u << 23));
                    values[d] = static_cast<float>(static_cast<double>(root) * std::sqrt(2.0));
                    values[d + 1024] = static_cast<float>(static_cast<double>(root) * 2.0);
                }
                return values;
            }();
            return table;
        }

        float cosFloat(double angle)
        {
            return cosTab(truncateToInt(angle * 65536.0 / (2.0 * Pi64)));
        }

        bool paletteLight(UObject* light, int32_t index, float& brightness, Vector& color)
        {
            UObject* skinObject = actor::Skin(light);
            if (!skinObject) {
                return false;
            }
            objects::ensureLoaded(skinObject);
            UTexture* skin = cast<UTexture>(skinObject);
            if (!skin) {
                return false;
            }
            UPalette* palette = skin->palette();
            if (!palette || palette->colors.size() < 256) {
                return false;
            }
            uint32_t entry = palette->colors[static_cast<size_t>(index)];
            float r = static_cast<float>(entry & 0xff);
            float g = static_cast<float>((entry >> 8) & 0xff);
            float b = static_cast<float>((entry >> 16) & 0xff);
            color = Vector { r, g, b }.normal();
            brightness = static_cast<float>(static_cast<double>(brightness) * ((2.0 * static_cast<double>(r) + 3.0 * static_cast<double>(g) + static_cast<double>(b)) * (1.0 / 1536.0)) * 2.8);
            return true;
        }

        int32_t paletteOnceIndex(UObject* light)
        {
            float defaultLife = 0;
            if (light->cls && light->cls->defaultObject && light->cls->defaultObject->data) {
                defaultLife = actor::LifeSpan(light->cls->defaultObject);
            }
            double fraction = 1.0 - static_cast<double>(actor::LifeSpan(light)) / static_cast<double>(defaultLife);
            if (std::isnan(fraction)) {
                fraction = 0;
            }
            fraction = std::clamp(fraction, 0.0, 1.0);
            return std::clamp(appFloor(static_cast<float>(fraction * 255.0)), 0, 255);
        }

        int32_t paletteLoopIndex(UObject* light, float time)
        {
            int32_t period = std::max<int32_t>(actor::LightPeriod(light), 1);
            double cycle = static_cast<double>(time) * 35.0 / period + actor::LightPhase(light);
            return (truncateToInt(cycle * 256.0) & 255) % 255;
        }

        bool roundsIntensity(uint8_t effect)
        {
            switch (effect) {
            case LE_Searchlight:
            case LE_SlowWave:
            case LE_FastWave:
            case LE_StaticSpot:
            case LE_Shock:
            case LE_Disco:
            case LE_Spotlight:
            case LE_NonIncidence:
            case LE_Shell:
            case LE_Interference:
            case LE_Cylinder:
            case LE_Rotor:
                return false;
            default:
                return true;
            }
        }
    }

    int32_t truncateToInt(double value)
    {
        if (!(std::abs(value) < 9.2e18)) {
            return 0;
        }
        return static_cast<int32_t>(static_cast<uint32_t>(static_cast<uint64_t>(static_cast<int64_t>(value))));
    }

    int32_t appFloor(float value)
    {
        return static_cast<int32_t>(std::nearbyint(static_cast<double>(value) - 0.5));
    }

    float sinTab(int32_t angle)
    {
        return trigTable()[static_cast<size_t>((angle >> 2) & 0x3fff)];
    }

    float cosTab(int32_t angle)
    {
        return trigTable()[static_cast<size_t>(((angle + 16384) >> 2) & 0x3fff)];
    }

    float sqrtApprox(float value)
    {
        uint32_t bits = std::bit_cast<uint32_t>(value);
        uint32_t result = std::bit_cast<uint32_t>(sqrtTable()[(bits >> 13) & 0x7ff]) + ((bits & 0x7f000000u) >> 1);
        return std::bit_cast<float>(result);
    }

    void tickLightRandoms(float timeSeconds)
    {
        LightRandoms& state = lightRandoms();
        uint32_t serverTicks = static_cast<uint32_t>(truncateToInt(static_cast<double>(timeSeconds) * 35.0));
        for (float& base : state.bases) {
            base = frand(state);
        }
        if (static_cast<uint32_t>((serverTicks - state.lastTicks) * RandomCycle) < static_cast<uint32_t>(RandomCount) && state.lastTicks) {
            int32_t r = static_cast<int32_t>((serverTicks * RandomCycle) & RandomMask);
            int32_t m = static_cast<int32_t>((state.lastTicks * RandomCycle) & RandomMask);
            for (int32_t n = m; n != r; n = (n + 1) & RandomMask) {
                state.deltas[static_cast<size_t>(n)] = (state.bases[static_cast<size_t>(n)] - state.randoms[static_cast<size_t>(n)]) / static_cast<float>(RandomCount / RandomCycle);
            }
            float steps = static_cast<float>((serverTicks - state.lastTicks) & RandomMask);
            do {
                state.randoms[static_cast<size_t>(r)] += state.deltas[static_cast<size_t>(r)] * steps;
                r = (r + 1) & RandomMask;
            } while (r != m);
        } else {
            for (size_t i = 0; i < state.randoms.size(); i++) {
                state.randoms[i] = frand(state);
                state.deltas[i] = (state.bases[i] - state.randoms[i]) / static_cast<float>(RandomCount / RandomCycle);
            }
        }
        for (float& value : state.randoms) {
            value = std::clamp(value, 0.0f, 1.0f);
        }
        state.lastTicks = serverTicks;
    }

    float lightRandomBase(int32_t index)
    {
        return lightRandoms().bases[static_cast<size_t>(index & RandomMask)];
    }

    float lightRandom(int32_t index)
    {
        return lightRandoms().randoms[static_cast<size_t>(index & RandomMask)];
    }

    bool lightEffectSpatialDynamic(uint8_t effect)
    {
        switch (effect) {
        case LE_Searchlight:
        case LE_SlowWave:
        case LE_FastWave:
        case LE_CloudCast:
        case LE_Shock:
        case LE_Disco:
        case LE_Interference:
        case LE_Rotor:
            return true;
        default:
            return false;
        }
    }

    bool lightEffectMergeDynamic(uint8_t effect)
    {
        return effect >= 1 && effect <= 3;
    }

    bool lightOnSurfaces(UObject* light)
    {
        return isLightSource(light) && actor::LightEffect(light) != LE_OmniBumpMap;
    }

    bool lightMovesOnSurfaces(UObject* light)
    {
        return actor::bDynamicLight(light) || !(actor::bStatic(light) || actor::bNoDelete(light));
    }

    bool lightBakedOnSurfaces(UObject* light)
    {
        if (lightMovesOnSurfaces(light) || !actor::bStatic(light) || actor::LightType(light) != LT_Steady) {
            return false;
        }
        uint8_t effect = actor::LightEffect(light) < LE_Max ? actor::LightEffect(light) : 0;
        return !lightEffectSpatialDynamic(effect) && !lightEffectMergeDynamic(effect);
    }

    float autoPanTexels(float timeSeconds, float speed)
    {
        double scaled = static_cast<double>(timeSeconds) * 35.0 * static_cast<double>(speed) * 256.0;
        return static_cast<float>(static_cast<double>(truncateToInt(scaled) & 0x3ffff) / 256.0);
    }

    float smallWavyPanU(float timeSeconds)
    {
        double t = timeSeconds;
        return static_cast<float>(8.0 * std::sin(t) + 4.0 * std::cos(2.3 * t));
    }

    float smallWavyPanV(float timeSeconds)
    {
        double t = timeSeconds;
        return static_cast<float>(8.0 * std::cos(t) + 4.0 * std::sin(2.3 * t));
    }

    Vector fGetHSV(uint8_t hue, uint8_t saturation, uint8_t value)
    {
        float brightness = static_cast<float>(value) * 1.4f / 255.0f;
        brightness *= 0.7f / (0.01f + std::sqrt(brightness));
        brightness = std::clamp(brightness, 0.0f, 1.0f);
        float h = static_cast<float>(hue);
        Vector base;
        if (hue < 86) {
            base = { (85.0f - h) / 85.0f, h / 85.0f, 0.0f };
        } else if (hue < 171) {
            base = { 0.0f, (170.0f - h) / 85.0f, (h - 85.0f) / 85.0f };
        } else {
            base = { (h - 170.0f) / 85.0f, 0.0f, (255.0f - h) / 84.0f };
        }
        float s = static_cast<float>(saturation) / 255.0f;
        return (base + (Vector { 1, 1, 1 } - base) * s) * brightness;
    }

    float lightDistanceFalloff(float distanceSquaredNormalized)
    {
        float index = std::round(distanceSquaredNormalized * 4093.0f);
        if (index >= 4096.0f) {
            return 0;
        }
        float s = std::sqrt((index + 1.0f) / 4096.0f);
        return 1.0f - 3.0f * s * s + 2.0f * s * s * s;
    }

    float lightRadius(UObject* light)
    {
        return (static_cast<float>(actor::LightRadius(light)) + 1.0f) * 25.0f;
    }

    bool isLightSource(UObject* actor)
    {
        return actor && actor->data && !actor->destroyed && actor::LightType(actor) != LT_None && actor::LightBrightness(actor) > 0;
    }

    Vector lightColor(UObject* light, float time)
    {
        return lightColor(light, time, nullptr);
    }

    Vector lightColor(UObject* light, float time, float* brightnessOut)
    {
        float brightness = static_cast<float>(actor::LightBrightness(light)) / 255.0f;
        Vector color = fGetHSV(actor::LightHue(light), actor::LightSaturation(light), 255);
        int32_t phase = actor::LightPhase(light);
        switch (actor::LightType(light)) {
        case LT_None:
            brightness = 0;
            break;
        case LT_Pulse: {
            int32_t period = std::max<int32_t>(actor::LightPeriod(light), 1);
            int32_t angle = truncateToInt(static_cast<double>(time) * 2293760.0 / period + (phase << 8));
            brightness = static_cast<float>((static_cast<double>(sinTab(angle)) * 0.39 + 0.6) * static_cast<double>(brightness));
            break;
        }
        case LT_SubtlePulse: {
            int32_t period = std::max<int32_t>(actor::LightPeriod(light), 1);
            int32_t angle = truncateToInt(static_cast<double>(time) * 2293760.0 / period + (phase << 8));
            brightness = static_cast<float>((static_cast<double>(sinTab(angle)) * 0.09 + 0.9) * static_cast<double>(brightness));
            break;
        }
        case LT_Blink: {
            int32_t period = actor::LightPeriod(light) + 1;
            if (truncateToInt(static_cast<double>(time) * 2293760.0 / period + (phase << 8)) & 1) {
                brightness = 0;
            }
            break;
        }
        case LT_Flicker: {
            float r = lightRandomBase(static_cast<int32_t>(light->handle & 0xff));
            brightness = r < 0.5f ? 0.0f : brightness * r;
            break;
        }
        case LT_Strobe: {
            static float lastUpdateTime = 0;
            static int32_t toggle = 0;
            if (lastUpdateTime != time) {
                lastUpdateTime = time;
                toggle ^= 1;
            }
            if (toggle) {
                brightness = 0;
            }
            break;
        }
        case LT_TexturePaletteOnce:
            paletteLight(light, paletteOnceIndex(light), brightness, color);
            break;
        case LT_TexturePaletteLoop:
            paletteLight(light, paletteLoopIndex(light, time), brightness, color);
            break;
        default:
            break;
        }
        brightness = std::clamp(brightness, 0.0f, 1.0f);
        float levelBrightness = 1.0f;
        if (UObject* level = actor::Level(light)) {
            if (level->data) {
                levelBrightness = levelinfo::Brightness(level);
            }
        }
        if (brightnessOut) {
            *brightnessOut = brightness;
        }
        return color * (brightness * levelBrightness);
    }

    PreparedLight prepareLight(UObject* light, const Vector& lightLocation)
    {
        PreparedLight prepared;
        prepared.location = lightLocation;
        prepared.radius = lightRadius(light);
        prepared.radiusSquared = prepared.radius * prepared.radius;
        prepared.effect = actor::LightEffect(light);
        prepared.period = actor::LightPeriod(light);
        prepared.phase = actor::LightPhase(light);
        if (prepared.effect == LE_Spotlight || prepared.effect == LE_StaticSpot) {
            prepared.spotDirection = -rotationVector(actor::Rotation(light));
            prepared.outer = 1.0f - actor::LightCone(light) / 256.0f;
        }
        return prepared;
    }

    float lightIllumination(const PreparedLight& light, const Vector& point, const Vector& normal, float time)
    {
        float radius = light.radius;
        Vector delta = light.location - point;
        float lengthSquared = delta.sizeSquared();
        float distanceSquared = lengthSquared / light.radiusSquared;
        switch (light.effect) {
        case LE_NonIncidence: {
            float distance = sqrtApprox(lengthSquared) / radius;
            return distance < 1.0f ? 1.02f - distance : 0.0f;
        }
        case LE_Cylinder:
            return std::max(1.0f - (delta.x * delta.x + delta.y * delta.y) / light.radiusSquared, 0.0f);
        case LE_Shell: {
            float distance = sqrtApprox(lengthSquared) / radius;
            return (distance > 0.8f && distance < 1.0f) ? 1.0f - 10.0f * std::abs(distance - 0.9f) : 0.0f;
        }
        default:
            break;
        }
        float falloff = lightDistanceFalloff(distanceSquared);
        if (falloff <= 0) {
            return 0;
        }
        float length = std::sqrt(lengthSquared);
        Vector direction = length > 0 ? delta / length : Vector { 0, 0, 1 };
        float attenuation = falloff * std::abs(direction.dot(normal));
        Vector vertex = -delta;
        double x = vertex.x;
        double y = vertex.y;
        double z = vertex.z;
        double planar = x * x + y * y;
        double t = time;
        switch (light.effect) {
        case LE_Spotlight:
        case LE_StaticSpot: {
            if (light.outer >= 1.0f) {
                return 0;
            }
            float cosine = direction.dot(light.spotDirection);
            if (cosine <= light.outer) {
                return 0;
            }
            float spot = (cosine - light.outer) / (1.0f - light.outer);
            attenuation *= spot * spot;
            break;
        }
        case LE_SlowWave: {
            int32_t distance = truncateToInt(static_cast<double>(sqrtApprox(lengthSquared)));
            attenuation *= static_cast<float>(0.7 + 0.3 * static_cast<double>(sinTab(truncateToInt((distance - t * 35.0) * 1024.0))));
            break;
        }
        case LE_FastWave: {
            int32_t distance = truncateToInt(static_cast<double>(sqrtApprox(lengthSquared)));
            attenuation *= static_cast<float>(0.7 + 0.3 * static_cast<double>(sinTab(truncateToInt(((distance >> 2) - t * 35.0) * 2048.0))));
            break;
        }
        case LE_Searchlight: {
            double offset = 2.0 * Pi64 + light.phase * (8.0 * Pi64 / 256.0) + (light.period ? 35.0 * t / light.period : 0.0);
            double angle = static_cast<float>(std::fmod(offset + 4.0 * std::atan2(x, y), 8.0 * Pi64));
            if (angle < Pi64 || angle > Pi64 * 3.0) {
                return 0;
            }
            double scale = static_cast<float>(0.5 + 0.5 * static_cast<double>(cosFloat(angle)));
            double spread = static_cast<float>(0.00006 * planar);
            if (spread < 1.0) {
                scale *= spread;
            }
            attenuation *= static_cast<float>(scale);
            break;
        }
        case LE_Rotor: {
            double angle = static_cast<float>(6.0 * std::atan2(x, y));
            double scale = static_cast<float>(0.5 + 0.5 * static_cast<double>(cosFloat(angle + t * 3.5)));
            double spread = static_cast<float>(0.0001 * planar);
            if (spread < 1.0) {
                scale = 1.0 - spread + scale * spread;
            }
            attenuation *= static_cast<float>(scale);
            break;
        }
        case LE_Shock: {
            int32_t distance = truncateToInt(8.0 * static_cast<double>(sqrtApprox(lengthSquared)));
            double wave = t * 4000.0;
            double brightness = 0.9 + 0.1 * static_cast<double>(sinTab(truncateToInt(((distance << 1) - wave) * 16.0)));
            brightness *= 0.9 + 0.1 * static_cast<double>(cosTab(truncateToInt((distance + wave) * 16.0)));
            brightness *= 0.9 + 0.1 * static_cast<double>(sinTab(truncateToInt(((distance >> 1) - wave) * 16.0)));
            attenuation *= static_cast<float>(brightness);
            break;
        }
        case LE_Disco: {
            double yaw = static_cast<float>(11.0 * std::atan2(x, y));
            double pitch = static_cast<float>(11.0 * std::atan2(static_cast<double>(sqrtApprox(static_cast<float>(planar))), z));
            double first = static_cast<float>(0.5 + 0.5 * static_cast<double>(cosFloat(yaw + t * 5.0)));
            double second = static_cast<float>(0.5 + 0.5 * static_cast<double>(cosFloat(pitch + t * 5.0)));
            double scale = static_cast<float>(first + second - first * second);
            double spread = static_cast<float>(0.00005 * planar);
            if (spread < 1.0) {
                scale *= spread;
            }
            attenuation *= static_cast<float>(1.0 - scale);
            break;
        }
        case LE_Interference: {
            double pitch = static_cast<float>(11.0 * std::atan2(static_cast<double>(sqrtApprox(static_cast<float>(planar))), z));
            attenuation *= static_cast<float>(0.5 + 0.5 * static_cast<double>(cosFloat(pitch + t * 5.0)));
            break;
        }
        default:
            break;
        }
        return attenuation;
    }

    float lightIllumination(UObject* light, const Vector& lightLocation, const Vector& point, const Vector& normal, float time)
    {
        return lightIllumination(prepareLight(light, lightLocation), point, normal, time);
    }

    int32_t lightIntensity(const PreparedLight& light, const Vector& point, const Vector& normal, float time, int32_t source)
    {
        float factor = lightIllumination(light, point, normal, time);
        if (factor <= 0) {
            return 0;
        }
        float scaled = static_cast<float>(source) * factor;
        if (roundsIntensity(light.effect)) {
            return std::clamp(static_cast<int32_t>(std::lround(scaled)), 0, 255);
        }
        return appFloor(scaled) & 0xff;
    }

    VolumetricLight prepareVolumetric(const Vector& relativeLocation, uint8_t volumeRadius, float brightness, uint8_t volumeBrightness)
    {
        VolumetricLight light;
        light.location = relativeLocation;
        light.radius = 25.0f * (static_cast<float>(volumeRadius) + 1.0f);
        light.radiusSquared = light.radius * light.radius;
        light.reciprocalRadius = 1.0f / light.radius;
        light.reciprocalRadiusSquared = light.reciprocalRadius * light.reciprocalRadius;
        light.locationSizeSquared = relativeLocation.sizeSquared();
        light.brightness = brightness * static_cast<float>(volumeBrightness) / 64.0f;
        light.inside = light.locationSizeSquared < light.radiusSquared;
        return light;
    }

    float volumetricFog(const VolumetricLight& light, const Vector& vertex)
    {
        static int32_t rejection = 0;
        float s = light.location.dot(vertex);
        if (!light.inside && std::signbit(s)) {
            return 0;
        }
        float vertexSizeSquared = vertex.sizeSquared();
        float s2 = s * s;
        if (rejection && vertexSizeSquared * light.radiusSquared < vertexSizeSquared * light.locationSizeSquared - s2) {
            return 0;
        }
        float d = light.locationSizeSquared - s2 / vertexSizeSquared;
        if (light.radiusSquared < d) {
            rejection = 1;
            return 0;
        }
        rejection = 0;
        float vertexSize = sqrtApprox(vertexSizeSquared);
        float c0 = s / vertexSize;
        float h = sqrtApprox(light.radiusSquared - d);
        bool fullSphere = false;
        float c1 = c0 - vertexSize;
        float c2 = c0;
        if (c0 > h) {
            c2 = h;
            if (c1 < -h) {
                c1 = -h;
                fullSphere = true;
            }
        } else if (c1 < -h) {
            c1 = -h;
        }
        if (c1 >= c2) {
            return 0;
        }
        float d2 = d * light.reciprocalRadiusSquared;
        c2 = c2 * light.reciprocalRadius;
        float inner = 3.0f - 3.0f * d2;
        float fog = 0;
        if (fullSphere) {
            fog = 2.0f * light.brightness * (c2 * (inner - c2 * c2));
        } else {
            c1 = c1 * light.reciprocalRadius;
            fog = light.brightness * ((c2 * (inner - c2 * c2)) - (c1 * (inner - c1 * c1)));
        }
        if (fog < 0) {
            return 0;
        }
        return std::min(fog, 1.0f);
    }

    uint8_t fogMerge(uint8_t dest, uint8_t add)
    {
        return static_cast<uint8_t>(add + dest - (add * dest) / 127);
    }

    Vector zoneAmbient(UObject* zone)
    {
        if (!zone || !zone->data || !zone->isA(nameZoneInfo)) {
            return {};
        }
        return fGetHSV(zoneinfo::AmbientHue(zone), zoneinfo::AmbientSaturation(zone), zoneinfo::AmbientBrightness(zone));
    }
}
