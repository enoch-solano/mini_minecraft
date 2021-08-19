#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <openglcontext.h>
#include <glm_includes.h>
#include <glm/glm.hpp>

#include "drawable.h"
#include "scene/camera.h"


class ShaderProgram
{
public:
    GLuint vertShader; // A handle for the vertex shader stored in this shader program
    GLuint fragShader; // A handle for the fragment shader stored in this shader program
    GLuint prog;       // A handle for the linked shader program stored in this class

    int attrPos; // A handle for the "in" vec4 representing vertex position in the vertex shader
    int attrNor; // A handle for the "in" vec4 representing vertex normal in the vertex shader
    int attrUV; // A handle for the "in" vec4 representing vertex UV in the vertex shader
    int attrCosPow; // A handle for the "in" vec4 representing vertex color in the vertex shader
    int attrAnim; // A handle for the "in" vec4 representing vertex color in the vertex shader
    int attrCol; // Vertex color

    int unifModel; // A handle for the "uniform" mat4 representing model matrix in the vertex shader
    int unifModelInvTr; // A handle for the "uniform" mat4 representing inverse transpose of the model matrix in the vertex shader
    int unifViewProj; // A handle for the "uniform" mat4 representing combined projection and view matrices in the vertex shader
    int unifBlockTexture; // A handle for the "uniform" vec4 representing color of geometry in the vertex shader
    int unifInventorySlotTexture;
    int unifSkyTexture;
    int unifScreenDimensions;
    int unifPlayerPos;
    int unifCamAttribs;
    int unifTime;

public:
    ShaderProgram(OpenGLContext* context);
    // Sets up the requisite GL data and shaders from the given .glsl files
    void create(const char *vertfile, const char *fragfile);

    // Tells our OpenGL context to use this shader to draw things
    void useMe();

    // Pass the given model matrix to this shader on the GPU
    void setModelMatrix(const glm::mat4 &model);
    // Pass the given Projection * View matrix to this shader on the GPU
    void setViewProjMatrix(const glm::mat4 &vp);
    // Pass the player's position to the GPU
    void setPlayerPos(const glm::vec3 &pp);
    // Pass the current time counter to the GPU
    void setTime(float t);
    // Pass the inventory slot texture
    void setInventorySlotTextureSampler(int slot);
    // Pass the given color to this shader on the GPU
    void setBlockTextureSampler(int slot);
    // Set out sky texture sampler to the correct slot
    void setSkyTextureSampler(int textureSlot);
    void setScreenDimensions(int w, int h);
    void setCamAttribs(const Camera &cam);

    // Interleaved VBO is used to draw all opaque data
    void draw(Drawable &d, bool opaque);

    // Interleaved VBO is used to draw onto a screen-space
    // object. Only needs position and UV data.
    void drawScreenSpace(Drawable &d);

    // Only uses position and color in interleaved VBO
    void drawWireframe(Drawable &d);

    // Utility function used in create()
    char* textFileRead(const char*);
    // Utility function that prints any shader compilation errors to the console
    void printShaderInfoLog(GLuint shader);
    // Utility function that prints any shader linking errors to the console
    void printLinkInfoLog(int prog);

    QString qTextFileRead(const char*);

private:
    OpenGLContext* context;   // Since Qt's OpenGL support is done through classes like QOpenGLFunctions_3_2_Core,
                            // we need to pass our OpenGL context to the Drawable in order to call GL functions
                            // from within this class.
};


#endif // SHADERPROGRAM_H
