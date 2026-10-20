#pragma once

#include "render/gl_math.h"

#include <cstdint>
#include <vector>

namespace dx {
    struct GLVertex {
        float x = 0;
        float y = 0;
        float z = 0;
        float u = 0;
        float v = 0;
        float lu = 0;
        float lv = 0;
        float r = 1;
        float g = 1;
        float b = 1;
        float a = 1;
        float nx = 0;
        float ny = 0;
        float nz = 1;
        float fogR = 0;
        float fogG = 0;
        float fogB = 0;
    };

    struct DynamicLight {
        Vector location;
        float radius = 0;
        Vector color;
        Vector direction;
        float cone = 2;
        bool specialLit = false;
    };

    constexpr int32_t SHADER_Textured = 1;
    constexpr int32_t SHADER_Lightmap = 2;
    constexpr int32_t SHADER_AlphaTest = 4;
    constexpr int32_t SHADER_Dynamic = 16;
    constexpr int32_t SHADER_Detail = 32;
    constexpr int32_t SHADER_Fog = 64;
    constexpr size_t MaxDynamicLights = 8;

    enum class BlendMode : uint8_t {
        Opaque,
        Masked,
        Translucent,
        Modulated,
        Alpha,
        Highlight,
    };

    class UberShader {
    public:
        UberShader();
        ~UberShader();
        UberShader(const UberShader&) = delete;
        UberShader& operator=(const UberShader&) = delete;

        void use();
        void setViewProj(const Mat4& matrix);
        void setFlags(int32_t flags);
        void setQuality(bool smoothLightmap, float detailDistance);
        void setPan(float u, float v);
        void setScale(float u, float v);
        void setTint(float r, float g, float b, float a);
        void setEye(const Vector& eye, const Vector& forward);
        void setClipPlanes(const float* planes, int32_t count);
        void setBlend(BlendMode mode);
        void setDynamicLights(const std::vector<DynamicLight>& lights);
        void bindTexture(uint32_t texture);
        void bindLightmap(uint32_t texture);
        void draw(const std::vector<GLVertex>& vertices, uint32_t primitive);
        void upload(const std::vector<GLVertex>& vertices);
        void drawRange(int32_t first, int32_t count);
        void bindStatic(uint32_t vao);
        void drawStatic(uint32_t vao, int32_t first, int32_t count);
        void drawStaticRanges(uint32_t vao, const int32_t* firsts, const int32_t* counts, int32_t ranges);
        uint32_t createStatic(const std::vector<GLVertex>& vertices, uint32_t& buffer);

    private:
        struct Locations {
            int32_t viewProj = -1;
            int32_t flags = -1;
            int32_t pan = -1;
            int32_t scale = -1;
            int32_t tint = -1;
            int32_t lightCount = -1;
            int32_t lightPosition = -1;
            int32_t lightColor = -1;
            int32_t lightDirection = -1;
            int32_t eye = -1;
            int32_t forward = -1;
            int32_t clipPlanes = -1;
            int32_t clipCount = -1;
        };

        struct State {
            Mat4 viewProj {};
            int32_t flags = 0;
            float pan[2] = {};
            float scale[2] = { 1, 1 };
            float tint[4] = { 1, 1, 1, 1 };
            int32_t lightCount = 0;
            float lightPositions[MaxDynamicLights * 4] = {};
            float lightColors[MaxDynamicLights * 4] = {};
            float lightDirections[MaxDynamicLights * 4] = {};
            float eye[3] = {};
            float forward[3] = {};
            float clipPlanes[32] = {};
            int32_t clipCount = 0;
        };

        void setupAttributes();
        int32_t stream(const std::vector<GLVertex>& vertices);
        void select(int32_t index);
        void sendState();

        uint32_t programs[2] = {};
        Locations locations[2];
        bool stale[2] = {};
        int32_t current = 0;
        State state;
        uint32_t streamVao = 0;
        uint32_t streamVbo = 0;
        size_t streamCapacity = 0;
        size_t streamOffset = 0;
        int32_t uploadBase = 0;
    };

    class GammaPass {
    public:
        GammaPass();
        ~GammaPass();
        GammaPass(const GammaPass&) = delete;
        GammaPass& operator=(const GammaPass&) = delete;

        void apply(int width, int height, float exponent);
        void bindTarget(int width, int height);
        void resolve(int width, int height, float exponent);

    private:
        void resize(int width, int height);
        void draw(int width, int height, float exponent);

        uint32_t program = 0;
        uint32_t vao = 0;
        uint32_t texture = 0;
        uint32_t framebuffer = 0;
        uint32_t depthBuffer = 0;
        int textureWidth = 0;
        int textureHeight = 0;
        int32_t exponentLocation = -1;
    };

    class ScenePass {
    public:
        ScenePass();
        ~ScenePass();
        ScenePass(const ScenePass&) = delete;
        ScenePass& operator=(const ScenePass&) = delete;

        void begin(int width, int height, int samples);
        void end(int width, int height, bool fxaa);

    private:
        uint32_t program = 0;
        uint32_t vao = 0;
        uint32_t texture = 0;
        uint32_t framebuffer = 0;
        uint32_t colorBuffer = 0;
        uint32_t depthBuffer = 0;
        int32_t target = 0;
        int32_t maxSamples = 0;
        int32_t inverseSizeLocation = -1;
        int bufferWidth = 0;
        int bufferHeight = 0;
        int bufferSamples = 0;
        int textureWidth = 0;
        int textureHeight = 0;
        bool multisampled = false;
    };
}
