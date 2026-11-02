#pragma once

#include "render/gl_math.h"
#include "render/render_device.h"

#include <cstdint>
#include <vector>

namespace dx {
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
        void setOcclusion(float weight);
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
            int32_t occlusion = -1;
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
            float occlusion = 0;
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

        void begin(int width, int height, int samples, bool occlusion);
        void end(int width, int height, bool fxaa);
        bool occluding() const;
        uint32_t sceneFramebuffer() const;
        uint32_t sceneDepth() const;
        uint32_t sceneMask() const;
        void setMaskWrites(bool enabled);
        void clearMask();

    private:
        void releaseTextures();
        void releaseMask();

        uint32_t program = 0;
        uint32_t vao = 0;
        uint32_t texture = 0;
        uint32_t framebuffer = 0;
        uint32_t colorBuffer = 0;
        uint32_t depthBuffer = 0;
        uint32_t maskBuffer = 0;
        uint32_t depthTexture = 0;
        uint32_t maskTexture = 0;
        int32_t target = 0;
        int32_t maxSamples = 0;
        int32_t inverseSizeLocation = -1;
        int bufferWidth = 0;
        int bufferHeight = 0;
        int bufferSamples = 0;
        bool bufferOcclusion = false;
        int textureWidth = 0;
        int textureHeight = 0;
        bool offscreen = false;
        bool occlusionActive = false;
        bool maskWrites = false;
    };

    class OcclusionPass {
    public:
        OcclusionPass();
        ~OcclusionPass();
        OcclusionPass(const OcclusionPass&) = delete;
        OcclusionPass& operator=(const OcclusionPass&) = delete;

        void apply(uint32_t sceneFramebuffer, uint32_t sceneDepth, uint32_t sceneMask, int width, int height, OcclusionQuality quality, float nearPlane, float farPlane, float tanHalfWidth, float tanHalfHeight);
        void release();

    private:
        struct Program {
            uint32_t id = 0;
            int32_t linearize = -1;
            int32_t divisor = -1;
            int32_t unproject = -1;
            int32_t direction = -1;
            int32_t mapping = -1;
        };

        void resize(int width, int height, int divisor);
        void resolve(uint32_t sceneFramebuffer, int width, int height);
        void releaseResolve();
        void run(const Program& program, uint32_t target, int width, int height, uint32_t texture);

        Program prepare;
        Program occlusion[3];
        Program blur;
        Program blurResolve;
        Program composite;
        uint32_t vao = 0;
        uint32_t resolveFramebuffer = 0;
        uint32_t depthTexture = 0;
        uint32_t maskTexture = 0;
        uint32_t packedFramebuffers[3] = {};
        uint32_t packedTextures[3] = {};
        uint32_t resultFramebuffer = 0;
        uint32_t resultTexture = 0;
        int textureWidth = 0;
        int textureHeight = 0;
        int textureDivisor = 0;
        int packedWidth = 0;
        int packedHeight = 0;
        int resolveWidth = 0;
        int resolveHeight = 0;
    };
}
