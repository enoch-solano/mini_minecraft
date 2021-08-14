#pragma once
#include "drawable.h"

class BlockOutline : public Drawable {
public:
    BlockOutline(OpenGLContext* context) : Drawable(context){}
    virtual ~BlockOutline() override;
    void create() override;
    GLenum drawMode() override;
};
