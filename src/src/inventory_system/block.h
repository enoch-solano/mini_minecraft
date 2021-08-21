#ifndef BLOCK_H
#define BLOCK_H

#include "drawable.h"
#include "scene/terrain.h"
#include "glm_includes.h"
#include "utils.h"

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

// Represents directionality w.r.t. to which face on a Block
enum FaceDirection : unsigned char {
    RIGHT, LEFT, TOP, BOTTOM, FRONT, BACK
};

class Block : public Drawable {
protected:
    float yaw_angle = -3*PI/4;
    float roll_angle = -PI/6;
    float spin_rate = -100.f;

    float m_time;
    float m_len;

    bool m_is_spinning;

    BlockType m_type;

public:
    Block(OpenGLContext *context, BlockType type);

    virtual ~Block() {}
    void create() override;

    void addFace(std::vector<vertexAttribute> &vboData,
                 std::vector<GLuint> &idxData, FaceDirection dir);

    void change_type(BlockType type);
    BlockType get_type();

    void give_it_a_spin();
    void remove_spin();
};

#endif // BLOCK_H
