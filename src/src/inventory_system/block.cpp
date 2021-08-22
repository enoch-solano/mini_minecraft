#include "block.h"

Block::Block(OpenGLContext *context, BlockType type)
    : Drawable(context),
      m_type(type),
      m_len(0.06f),
      m_time(0.f),
      m_is_spinning(false) {}

void Block::create() {
    std::vector<vertexAttribute> vboData;
    std::vector<GLuint> idxData;

    FaceDirection dir[6] = { TOP, RIGHT, FRONT, BOTTOM, LEFT, BACK };

    for (int i = 0; i < 6; i++) {
        addFace(vboData, idxData, dir[i]);
    }

    m_count = static_cast<int>(idxData.size());

    // send index data to the GPU
    generateIdxOpq();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxData.size() * sizeof(GLuint), idxData.data(), GL_STATIC_DRAW);

    // send VBO data to the GPU
    generateOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufOpq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(vertexAttribute), vboData.data(), GL_STATIC_DRAW);
}

void Block::addFace(std::vector<vertexAttribute> &vboData,
                    std::vector<GLuint> &idxData, FaceDirection dir)
{
    if (m_type == EMPTY) {
        return;
    }

    // adds the appropriate vertex indices for new vertices
    GLuint prev_num_verts = vboData.size();
    for (GLuint i = 0; i < 2; i++) {
        idxData.push_back(prev_num_verts);
        idxData.push_back(prev_num_verts + i+1);
        idxData.push_back(prev_num_verts + i+2);
    }

    // holds vertex VBO data of THIS face
    std::array<vertexAttribute, 4> faceVBOData;

    //////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////// DETERMINES ATTRIBUTES BASED on TYPE of BLOCK //////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////

    glm::vec2 offset;       // determines offset in texture map for each block
    float face_cos_pow;     // the cosine power for the Blinn-Phong reflection model
    float face_is_animateable = -1.f;    // the animateable flag

    switch(m_type) {
    case GRASS:
        offset = dir == TOP ? glm::vec2(8, 13) : (dir == BOTTOM ? glm::vec2(2, 15) : glm::vec2(3, 15));
        face_cos_pow = 2.f;
        break;
    case DIRT:
        offset = glm::vec2(2, 15);
        face_cos_pow = 2.f;
        break;
    case STONE:
        offset = glm::vec2(1, 15);
        face_cos_pow = 50.f;
        break;
    case SNOW:
        offset = glm::vec2(2, 11);
        face_cos_pow = 10.f;
        break;
    default:
        offset = dir == TOP ? glm::vec2(8, 13) : (dir == BOTTOM ? glm::vec2(2, 15) : glm::vec2(3, 15));
        face_cos_pow = 2.f;
        break;
    }

    // adds the appropriate uv coordinates for this face
    faceVBOData[0].uv = (glm::vec2(1.f, 1.f) + offset) / 16.f;
    faceVBOData[1].uv = (glm::vec2(0.f, 1.f) + offset) / 16.f;
    faceVBOData[2].uv = (glm::vec2(0.f, 0.f) + offset) / 16.f;
    faceVBOData[3].uv = (glm::vec2(1.f, 0.f) + offset) / 16.f;

    //////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////// DETERMINES ATTRIBUTES BASED on DIR. of BLOCK //////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////

    // determines appropriate face normal to attach to face, along with positions
    glm::vec4 face_nor;
    switch(dir) {
    case RIGHT:
        face_nor = glm::vec4(1.f, 0.f, 0.f, 0.f);

        faceVBOData[0].pos = glm::vec4(m_len,  m_len,  m_len, 1.f);
        faceVBOData[1].pos = glm::vec4(m_len,  m_len, -m_len, 1.f);
        faceVBOData[2].pos = glm::vec4(m_len, -m_len, -m_len, 1.f);
        faceVBOData[3].pos = glm::vec4(m_len, -m_len,  m_len, 1.f);

        break;
    case LEFT:
        face_nor = glm::vec4(-1.f, 0.f, 0.f, 0.f);

        faceVBOData[0].pos = glm::vec4(-m_len,  m_len, -m_len, 1.f);
        faceVBOData[1].pos = glm::vec4(-m_len,  m_len,  m_len, 1.f);
        faceVBOData[2].pos = glm::vec4(-m_len, -m_len,  m_len, 1.f);
        faceVBOData[3].pos = glm::vec4(-m_len, -m_len, -m_len, 1.f);

        break;
    case TOP:
        face_nor = glm::vec4(0.f, 1.f, 0.f, 0.f);

        faceVBOData[0].pos = glm::vec4(-m_len, m_len, -m_len, 1.f);
        faceVBOData[1].pos = glm::vec4( m_len, m_len, -m_len, 1.f);
        faceVBOData[2].pos = glm::vec4( m_len, m_len,  m_len, 1.f);
        faceVBOData[3].pos = glm::vec4(-m_len, m_len,  m_len, 1.f);

        break;
    case BOTTOM:
        face_nor = glm::vec4(0.f, -1.f, 0.f, 0.f);

        faceVBOData[0].pos = glm::vec4(-m_len, -m_len, -m_len, 1.f);
        faceVBOData[1].pos = glm::vec4(-m_len, -m_len,  m_len, 1.f);
        faceVBOData[2].pos = glm::vec4( m_len, -m_len,  m_len, 1.f);
        faceVBOData[3].pos = glm::vec4( m_len, -m_len, -m_len, 1.f);

        break;
    case FRONT:
        face_nor = glm::vec4(0.f, 0.f, 1.f, 0.f);

        faceVBOData[0].pos = glm::vec4(-m_len,  m_len, m_len, 1.f);
        faceVBOData[1].pos = glm::vec4( m_len,  m_len, m_len, 1.f);
        faceVBOData[2].pos = glm::vec4( m_len, -m_len, m_len, 1.f);
        faceVBOData[3].pos = glm::vec4(-m_len, -m_len, m_len, 1.f);

        break;
    case BACK:
        face_nor = glm::vec4(0.f, 0.f, -1.f, 0.f);

        faceVBOData[0].pos = glm::vec4( m_len,  m_len, -m_len, 1.f);
        faceVBOData[1].pos = glm::vec4(-m_len,  m_len, -m_len, 1.f);
        faceVBOData[2].pos = glm::vec4(-m_len, -m_len, -m_len, 1.f);
        faceVBOData[3].pos = glm::vec4( m_len, -m_len, -m_len, 1.f);

        break;
    }


    //////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////

    float yaw = yaw_angle;
    if (m_is_spinning) {
        yaw += m_time;
        m_time += 1 / spin_rate;
    }

    glm::mat4 rot = glm::rotate(glm::mat4(), roll_angle, glm::vec3(1,0,0)) * glm::rotate(glm::mat4(), yaw, glm::vec3(0,1,0));

    for (int i = 0; i < 4; i++) {
        faceVBOData[i].pos = rot * faceVBOData[i].pos;
    }


    // ensures all vertices have the same attribute
    for (int i = 0; i < 4; i++) {
        faceVBOData[i].nor = face_nor;
        faceVBOData[i].cosPos = face_cos_pow;
        faceVBOData[i].animFlag = face_is_animateable;

        faceVBOData[i].pos.x *= 3.f / 4.f;
        faceVBOData[i].pos.z *= 3.f / 4.f;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////

    // appends NEW face-vertex information
    vboData.insert(vboData.end(), faceVBOData.data(), faceVBOData.data() + 4);
}


void Block::change_type(BlockType type) {
    m_type = type;
}

BlockType Block::get_type() {
    return m_type;
}

void Block::give_it_a_spin() {
    m_is_spinning = true;
}

void Block::remove_spin() {
    m_is_spinning = false;
    m_time = 0.f;
}
