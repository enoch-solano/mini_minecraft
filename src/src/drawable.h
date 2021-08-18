#pragma once

#include <openglcontext.h>
#include <glm_includes.h>

// holds the vertex attributes in the correct order
struct vertexAttribute {
    glm::vec4 pos;  // position
    glm::vec4 nor;  // normal
    glm::vec2 uv;   // uv coords
    float cosPos;   // cosine power
    float animFlag; // animation flag
};


//This defines a class which can be rendered by our shader program.
//Make any geometry a subclass of ShaderProgram::Drawable in order to render it with the ShaderProgram class.
class Drawable {
protected:
    int m_count;     // The number of indices stored in bufIdx.
    GLuint m_bufIdxOpq; // A Vertex Buffer Object that we will use to store triangle indices (GLuints)
    GLuint m_bufIdxTra; // A Vertex Buffer Object that we will use to store triangle indices (GLuints)
    GLuint m_bufOpq; // A Vertex Buffer Object that we will use to store opaque mesh data (vec4s)
    GLuint m_bufTra; // A Vertex Buffer Object that we will use to store transparent mesh data (vec4s)

    bool m_idxOpqGenerated; // Set to TRUE by generateIdx(), returned by bindIdx().
    bool m_idxTraGenerated;
    bool m_opqGenerated;
    bool m_traGenerated;
    bool m_hasBeenDestroyed;

    OpenGLContext* mp_context; // Since Qt's OpenGL support is done through classes like QOpenGLFunctions_3_2_Core,
                          // we need to pass our OpenGL context to the Drawable in order to call GL functions
                          // from within this class.


public:
    Drawable(OpenGLContext* context);
    virtual ~Drawable();

    virtual void create() = 0; // To be implemented by subclasses. Populates the VBOs of the Drawable.
    void destroy(); // Frees the VBOs of the Drawable.
    bool hasBeenDestroyed() const;

    // Getter functions for various GL data
    virtual GLenum drawMode();
    int elemCount() const;

    // Call these functions when you want to call glGenBuffers on the buffers stored in the Drawable
    // These will properly set the values of idxBound etc. which need to be checked in ShaderProgram::draw()
    void generateIdxOpq();
    void generateIdxTra();
    void generateOpq();
    void generateTra();

    bool bindIdx();
    bool bindOpq();
    bool bindTra();
};
