#include "blockoutline.h"
#include "vector"

BlockOutline::~BlockOutline()
{}

void BlockOutline::create() {
    std::vector<float> vboData;
    std::vector<GLuint> idxData {
        // Bottom
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        // Top
        4, 5,
        5, 6,
        6, 7,
        7, 4,
        // Bottom to Top
        0, 4,
        1, 5,
        2, 6,
        3, 7
    };

    std::vector<glm::vec3> positions {
        glm::vec3(0,0,0),
        glm::vec3(1,0,0),
        glm::vec3(1,0,1),
        glm::vec3(0,0,1),

        glm::vec3(0,1,0),
        glm::vec3(1,1,0),
        glm::vec3(1,1,1),
        glm::vec3(0,1,1),
    };

    for (glm::vec3 p : positions) {
        // Pos
        vboData.push_back(p.x);
        vboData.push_back(p.y);
        vboData.push_back(p.z);
        vboData.push_back(1.f);
        // Col
        vboData.push_back(1.f);
        vboData.push_back(1.f);
        vboData.push_back(1.f);
        vboData.push_back(1.f);
    }

    m_count = static_cast<int>(idxData.size());
    generateIdxOpq();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxData.size() * sizeof(GLuint), idxData.data(), GL_STATIC_DRAW);
    generateOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufOpq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(float), vboData.data(), GL_STATIC_DRAW);
}

GLenum BlockOutline::drawMode() {
    return GL_LINES;
}
