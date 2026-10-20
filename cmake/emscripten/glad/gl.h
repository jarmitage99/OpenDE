#pragma once

#include <GLES3/gl3.h>
#include <emscripten/html5_webgl.h>

#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FF
#define GL_CLIP_DISTANCE0 0x3000

typedef void* (*GLADloadfunc)(const char* name);

inline int GLAD_GL_ARB_texture_filter_anisotropic = 0;
inline int GLAD_GL_EXT_texture_filter_anisotropic = 0;

inline int gladLoadGL(GLADloadfunc)
{
    GLAD_GL_EXT_texture_filter_anisotropic = emscripten_webgl_enable_extension(emscripten_webgl_get_current_context(), "EXT_texture_filter_anisotropic") ? 1 : 0;
    return 1;
}

inline void glDepthRange(double nearValue, double farValue)
{
    glDepthRangef(static_cast<float>(nearValue), static_cast<float>(farValue));
}

inline void glClearDepth(double depth)
{
    glClearDepthf(static_cast<float>(depth));
}

inline void glMultiDrawArrays(GLenum mode, const GLint* firsts, const GLsizei* counts, GLsizei ranges)
{
    for (GLsizei i = 0; i < ranges; i++) {
        glDrawArrays(mode, firsts[i], counts[i]);
    }
}
