#include "render/gl_device.h"

#include <glad/gl.h>

namespace dx {
    GLRenderDevice::GLRenderDevice()
    {
        shader = std::make_unique<UberShader>();
        gammaPass = std::make_unique<GammaPass>();
        scenePass = std::make_unique<ScenePass>();
        textures = std::make_unique<TextureCache>();
        canvasImpl = std::make_unique<GLCanvas>(*shader, *textures);
    }

    GLRenderDevice::~GLRenderDevice() = default;

    bool GLRenderDevice::drawingToWindow()
    {
        GLint boundFramebuffer = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &boundFramebuffer);
        return boundFramebuffer == 0;
    }

    void GLRenderDevice::bindGammaTarget(int width, int height)
    {
        gammaPass->bindTarget(width, height);
    }

    void GLRenderDevice::resolveGamma(int width, int height, float exponent)
    {
        gammaPass->resolve(width, height, exponent);
    }

    void GLRenderDevice::applyGamma(int width, int height, float exponent)
    {
        gammaPass->apply(width, height, exponent);
    }

    void GLRenderDevice::beginScene(int width, int height, int samples, OcclusionQuality ambientOcclusion)
    {
        bool occluding = ambientOcclusion != OcclusionQuality::Off;
        if (occluding && !occlusionPass) {
            occlusionPass = std::make_unique<OcclusionPass>();
        }
        if (!occluding && occlusionPass) {
            occlusionPass->release();
        }
        sceneWidth = width;
        sceneHeight = height;
        occlusionQuality = ambientOcclusion;
        scenePass->begin(width, height, samples, occluding);
    }

    void GLRenderDevice::endScene(int width, int height, bool fxaa)
    {
        scenePass->end(width, height, fxaa);
    }

    void GLRenderDevice::beginOcclusion()
    {
        scenePass->clearMask();
    }

    void GLRenderDevice::setOcclusionSource(OcclusionSource source)
    {
        switch (source) {
        case OcclusionSource::None:
            shader->setOcclusion(0.0f);
            break;
        case OcclusionSource::World:
            shader->setOcclusion(0.25f);
            break;
        case OcclusionSource::Mesh:
            shader->setOcclusion(1.0f);
            break;
        }
    }

    void GLRenderDevice::applyOcclusion(float nearPlane, float farPlane, float tanHalfWidth, float tanHalfHeight)
    {
        if (!scenePass->occluding()) {
            return;
        }
        occlusionPass->apply(scenePass->sceneFramebuffer(), scenePass->sceneDepth(), scenePass->sceneMask(), sceneWidth, sceneHeight, occlusionQuality, nearPlane, farPlane, tanHalfWidth, tanHalfHeight);
    }

    void GLRenderDevice::readPixels(int width, int height, uint8_t* rgba)
    {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }

    void GLRenderDevice::setViewport(int x, int y, int width, int height)
    {
        glViewport(x, y, width, height);
    }

    void GLRenderDevice::enableScissor(int x, int y, int width, int height)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, width, height);
    }

    void GLRenderDevice::disableScissor()
    {
        glDisable(GL_SCISSOR_TEST);
    }

    void GLRenderDevice::clear(bool color, bool depth, float depthValue)
    {
        if (depthValue != 1.0f) {
            glClearDepth(static_cast<double>(depthValue));
        }
        GLbitfield mask = 0;
        if (color) {
            glClearColor(0, 0, 0, 1);
            mask |= GL_COLOR_BUFFER_BIT;
        }
        if (depth) {
            mask |= GL_DEPTH_BUFFER_BIT;
        }
        glClear(mask);
        if (depthValue != 1.0f) {
            glClearDepth(1.0);
        }
    }

    void GLRenderDevice::setDepthTest(bool enabled)
    {
        if (enabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }

    void GLRenderDevice::setDepthWrite(bool enabled)
    {
        glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    }

    void GLRenderDevice::setDepthFunc(DepthFunc func)
    {
        switch (func) {
        case DepthFunc::LessEqual:
            glDepthFunc(GL_LEQUAL);
            break;
        case DepthFunc::Equal:
            glDepthFunc(GL_EQUAL);
            break;
        case DepthFunc::Always:
            glDepthFunc(GL_ALWAYS);
            break;
        }
    }

    void GLRenderDevice::setDepthRange(float nearValue, float farValue)
    {
        glDepthRange(static_cast<double>(nearValue), static_cast<double>(farValue));
    }

    void GLRenderDevice::setColorWrite(bool enabled)
    {
        GLboolean value = enabled ? GL_TRUE : GL_FALSE;
        glColorMask(value, value, value, value);
    }

    void GLRenderDevice::setCulling(bool enabled)
    {
        if (enabled) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }
    }

    void GLRenderDevice::setFrontFace(bool clockwise)
    {
        glFrontFace(clockwise ? GL_CW : GL_CCW);
    }

    void GLRenderDevice::setWireframe(bool enabled)
    {
#ifndef __EMSCRIPTEN__
        glPolygonMode(GL_FRONT_AND_BACK, enabled ? GL_LINE : GL_FILL);
#else
        static_cast<void>(enabled);
#endif
    }

    void GLRenderDevice::setPolygonOffset(bool enabled, float factor, float units)
    {
        if (enabled) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(factor, units);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    void GLRenderDevice::setBlend(BlendMode mode)
    {
        shader->setBlend(mode);
        scenePass->setMaskWrites(mode == BlendMode::Opaque || mode == BlendMode::Masked);
    }

    void GLRenderDevice::setClipPlanes(const float* planes, int32_t count)
    {
#ifndef __EMSCRIPTEN__
        for (int32_t i = 0; i < 8; i++) {
            if (i < count) {
                glEnable(static_cast<GLenum>(GL_CLIP_DISTANCE0 + i));
            } else {
                glDisable(static_cast<GLenum>(GL_CLIP_DISTANCE0 + i));
            }
        }
#endif
        shader->setClipPlanes(planes, count);
    }

    void GLRenderDevice::useShader()
    {
        shader->use();
    }

    void GLRenderDevice::setViewProj(const Mat4& matrix)
    {
        shader->setViewProj(matrix);
    }

    void GLRenderDevice::setFlags(int32_t flags)
    {
        shader->setFlags(flags);
    }

    void GLRenderDevice::setQuality(bool smoothLightmap, float detailDistance)
    {
        shader->setQuality(smoothLightmap, detailDistance);
    }

    void GLRenderDevice::setPan(float u, float v)
    {
        shader->setPan(u, v);
    }

    void GLRenderDevice::setScale(float u, float v)
    {
        shader->setScale(u, v);
    }

    void GLRenderDevice::setTint(float r, float g, float b, float a)
    {
        shader->setTint(r, g, b, a);
    }

    void GLRenderDevice::setEye(const Vector& eye, const Vector& forward)
    {
        shader->setEye(eye, forward);
    }

    void GLRenderDevice::setDynamicLights(const std::vector<DynamicLight>& lights)
    {
        shader->setDynamicLights(lights);
    }

    void GLRenderDevice::bindTexture(uint32_t texture)
    {
        shader->bindTexture(texture);
    }

    void GLRenderDevice::bindLightmap(uint32_t texture)
    {
        shader->bindLightmap(texture);
    }

    void GLRenderDevice::draw(const std::vector<GLVertex>& vertices, Primitive primitive)
    {
        shader->draw(vertices, primitive == Primitive::Lines ? GL_LINES : GL_TRIANGLES);
    }

    void GLRenderDevice::upload(const std::vector<GLVertex>& vertices)
    {
        shader->upload(vertices);
    }

    void GLRenderDevice::drawRange(int32_t first, int32_t count)
    {
        shader->drawRange(first, count);
    }

    uint32_t GLRenderDevice::createStatic(const std::vector<GLVertex>& vertices)
    {
        uint32_t buffer = 0;
        uint32_t vao = shader->createStatic(vertices, buffer);
        staticBuffers[vao] = buffer;
        return vao;
    }

    void GLRenderDevice::deleteStatic(uint32_t geometry)
    {
        auto found = staticBuffers.find(geometry);
        if (found == staticBuffers.end()) {
            return;
        }
        glDeleteVertexArrays(1, &found->first);
        glDeleteBuffers(1, &found->second);
        staticBuffers.erase(found);
    }

    void GLRenderDevice::bindStatic(uint32_t geometry)
    {
        shader->bindStatic(geometry);
    }

    void GLRenderDevice::drawStatic(uint32_t geometry, int32_t first, int32_t count)
    {
        shader->drawStatic(geometry, first, count);
    }

    void GLRenderDevice::drawStaticRanges(uint32_t geometry, const int32_t* firsts, const int32_t* counts, int32_t ranges)
    {
        shader->drawStaticRanges(geometry, firsts, counts, ranges);
    }

    uint32_t GLRenderDevice::createQuery()
    {
        GLuint query = 0;
        glGenQueries(1, &query);
        return query;
    }

    void GLRenderDevice::deleteQuery(uint32_t query)
    {
        glDeleteQueries(1, &query);
    }

    void GLRenderDevice::beginQuery(uint32_t query)
    {
        glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
    }

    void GLRenderDevice::endQuery()
    {
        glEndQuery(GL_ANY_SAMPLES_PASSED);
    }

    bool GLRenderDevice::queryResult(uint32_t query, bool& visible)
    {
        GLuint available = 0;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available) {
            return false;
        }
        GLuint result = 0;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &result);
        visible = result != 0;
        return true;
    }

    void GLRenderDevice::beginTextureFrame(double time)
    {
        textures->beginFrame(time);
    }

    UTexture* GLRenderDevice::animated(UTexture* texture)
    {
        return textures->animated(texture);
    }

    const GLTextureInfo* GLRenderDevice::texture(UTexture* texture)
    {
        return textures->get(texture);
    }

    uint32_t GLRenderDevice::whiteTexture()
    {
        return textures->white();
    }

    void GLRenderDevice::setTextureOptions(float anisotropy, bool trilinear, float rate)
    {
        textures->setFiltering(anisotropy, trilinear);
        textures->setConstantRate(rate);
    }

    void GLRenderDevice::clearTextures()
    {
        textures->clear();
    }

    uint32_t GLRenderDevice::createLightmap(int32_t size, const uint8_t* rgba)
    {
        GLuint id = 0;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return id;
    }

    void GLRenderDevice::updateLightmap(uint32_t texture, int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t* rgba)
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    }

    void GLRenderDevice::deleteLightmaps(const std::vector<uint32_t>& ids)
    {
        if (!ids.empty()) {
            glDeleteTextures(static_cast<GLsizei>(ids.size()), ids.data());
        }
    }

    Canvas2D& GLRenderDevice::canvas()
    {
        return *canvasImpl;
    }

    void GLRenderDevice::beginCanvas(int width, int height)
    {
        canvasImpl->begin(width, height);
    }

    void GLRenderDevice::flushCanvas()
    {
        canvasImpl->flush();
    }
}
