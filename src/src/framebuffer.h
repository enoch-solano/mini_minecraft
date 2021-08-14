#pragma once
#include "openglcontext.h"
#include "glm_includes.h"

#define SKY_TEXTURE_SLOT 3

class FrameBuffer {
private:
    OpenGLContext *mp_context;
    GLuint m_frameBuffer;
    GLuint m_outputTexture;
    GLuint m_depthRenderBuffer;

    unsigned int m_width, m_height, m_devicePixelRatio;
    bool m_created;

    unsigned int m_textureSlot;

public:
    FrameBuffer(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio);
    void resize(unsigned int width, unsigned int height, unsigned int devicePixelRatio);
    // Initialize all GPU-side data required
    void create();
    // Deallocate all GPU-side data
    void destroy();
    void bindFrameBuffer();
    // Associate our output texture with the indicated texture slot
    void bindToTextureSlot(unsigned int slot);
    unsigned int getTextureSlot() const;
};
