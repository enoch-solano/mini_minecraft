#include "quad.h"

Quad::Quad(OpenGLContext *context) : Drawable(context)
{}

void Quad::create()
{
    GLuint idx[6]{0, 1, 2, 0, 2, 3};

    float pos_uv_data[] {
        -1.f, -1.f, 0.9999f, 1.f,
        0.f, 0.f,
        1.f, -1.f, 0.9999f, 1.f,
        1.f, 0.f,
        1.f, 1.f, 0.9999f, 1.f,
        1.f, 1.f,
        -1.f, 1.f, 0.9999f, 1.f,
        0.f, 1.f,
    };

    m_count = 6;

    // Create a VBO on our GPU and store its handle in bufIdx
    generateIdxOpq();
    // Tell OpenGL that we want to perform subsequent operations on the VBO referred to by bufIdx
    // and that it will be treated as an element array buffer (since it will contain triangle indices)
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    // Pass the data stored in cyl_idx into the bound buffer, reading a number of bytes equal to
    // CYL_IDX_COUNT multiplied by the size of a GLuint. This data is sent to the GPU to be read by shader programs.
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);

    // The next few sets of function calls are basically the same as above, except bufPos and bufNor are
    // array buffers rather than element array buffers, as they store vertex attributes like position.
    generateOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufOpq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), pos_uv_data, GL_STATIC_DRAW);
}
