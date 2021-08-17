#ifndef BLOCK_H
#define BLOCK_H

#include "drawable.h"
#include "scene/terrain.h"
#include "glm_includes.h"
#include "utils.h"

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Block : public Drawable {
protected:

public:
    Block(OpenGLContext *context, BlockType type);

    virtual ~Block() {}
    void create() override {}
};

#endif // BLOCK_H
