#pragma once

#include "openglcontext.h"
#include "glm_includes.h"
#include "smartpointerhelp.h"

#define MINECRAFT_BLOCK_TEXTURE_SLOT 0
#define POST_PROCESS_TEXTURE_SLOT 1

class Texture
{
public:
    Texture(OpenGLContext* context);
    ~Texture();

    void create(const char *texturePath);
    void load(GLuint texSlot);
    void bind(GLuint texSlot);

private:
    OpenGLContext* context;
    GLuint m_textureHandle;
    uPtr<QImage> m_textureImage;
};
