#pragma once

#include "render/gl_canvas.h"
#include "render/gl_shader.h"
#include "render/render_device.h"
#include "render/texture_cache.h"

#include <memory>
#include <unordered_map>

namespace dx {
    class GLRenderDevice : public RenderDevice {
    public:
        GLRenderDevice();
        ~GLRenderDevice() override;
        GLRenderDevice(const GLRenderDevice&) = delete;
        GLRenderDevice& operator=(const GLRenderDevice&) = delete;

        bool drawingToWindow() override;
        void bindGammaTarget(int width, int height) override;
        void resolveGamma(int width, int height, float exponent) override;
        void applyGamma(int width, int height, float exponent) override;
        void beginScene(int width, int height, int samples, OcclusionQuality ambientOcclusion) override;
        void endScene(int width, int height, bool fxaa) override;
        void beginOcclusion() override;
        void setOcclusionSource(OcclusionSource source) override;
        void applyOcclusion(float nearPlane, float farPlane, float tanHalfWidth, float tanHalfHeight) override;
        void readPixels(int width, int height, uint8_t* rgba) override;

        void setViewport(int x, int y, int width, int height) override;
        void enableScissor(int x, int y, int width, int height) override;
        void disableScissor() override;
        void clear(bool color, bool depth, float depthValue) override;
        void setDepthTest(bool enabled) override;
        void setDepthWrite(bool enabled) override;
        void setDepthFunc(DepthFunc func) override;
        void setDepthRange(float nearValue, float farValue) override;
        void setColorWrite(bool enabled) override;
        void setCulling(bool enabled) override;
        void setFrontFace(bool clockwise) override;
        void setWireframe(bool enabled) override;
        void setPolygonOffset(bool enabled, float factor, float units) override;
        void setBlend(BlendMode mode) override;
        void setClipPlanes(const float* planes, int32_t count) override;

        void useShader() override;
        void setViewProj(const Mat4& matrix) override;
        void setFlags(int32_t flags) override;
        void setQuality(bool smoothLightmap, float detailDistance) override;
        void setPan(float u, float v) override;
        void setScale(float u, float v) override;
        void setTint(float r, float g, float b, float a) override;
        void setEye(const Vector& eye, const Vector& forward) override;
        void setDynamicLights(const std::vector<DynamicLight>& lights) override;
        void bindTexture(uint32_t texture) override;
        void bindLightmap(uint32_t texture) override;

        void draw(const std::vector<GLVertex>& vertices, Primitive primitive) override;
        void upload(const std::vector<GLVertex>& vertices) override;
        void drawRange(int32_t first, int32_t count) override;
        uint32_t createStatic(const std::vector<GLVertex>& vertices) override;
        void deleteStatic(uint32_t geometry) override;
        void bindStatic(uint32_t geometry) override;
        void drawStatic(uint32_t geometry, int32_t first, int32_t count) override;
        void drawStaticRanges(uint32_t geometry, const int32_t* firsts, const int32_t* counts, int32_t ranges) override;

        uint32_t createQuery() override;
        void deleteQuery(uint32_t query) override;
        void beginQuery(uint32_t query) override;
        void endQuery() override;
        bool queryResult(uint32_t query, bool& visible) override;

        void beginTextureFrame(double time) override;
        UTexture* animated(UTexture* texture) override;
        const GLTextureInfo* texture(UTexture* texture) override;
        uint32_t whiteTexture() override;
        void setTextureOptions(float anisotropy, bool trilinear, float rate) override;
        void clearTextures() override;

        uint32_t createLightmap(int32_t size, const uint8_t* rgba) override;
        void updateLightmap(uint32_t texture, int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t* rgba) override;
        void deleteLightmaps(const std::vector<uint32_t>& ids) override;

        Canvas2D& canvas() override;
        void beginCanvas(int width, int height) override;
        void flushCanvas() override;

    private:
        std::unique_ptr<UberShader> shader;
        std::unique_ptr<GammaPass> gammaPass;
        std::unique_ptr<ScenePass> scenePass;
        std::unique_ptr<OcclusionPass> occlusionPass;
        std::unique_ptr<TextureCache> textures;
        std::unique_ptr<GLCanvas> canvasImpl;
        std::unordered_map<uint32_t, uint32_t> staticBuffers;
        int sceneWidth = 0;
        OcclusionQuality occlusionQuality = OcclusionQuality::Off;
        int sceneHeight = 0;
    };
}
