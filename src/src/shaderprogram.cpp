#include "shaderprogram.h"
#include <QFile>
#include <QStringBuilder>
#include <QTextStream>
#include <QDebug>
#include <iostream>


ShaderProgram::ShaderProgram(OpenGLContext *context)
    : vertShader(), fragShader(), prog(),
      attrPos(-1), attrNor(-1), attrUV(-1), attrCosPow(-1), attrAnim(-1),
      unifModel(-1), unifModelInvTr(-1), unifViewProj(-1), unifBlockTexture(-1),
      unifSkyTexture(-1), unifScreenDimensions(-1),
      unifPlayerPos(-1), unifCamAttribs(-1), unifTime(-1),
      context(context)
{}

void ShaderProgram::create(const char *vertfile, const char *fragfile)
{
    // Allocate space on our GPU for a vertex shader and a fragment shader and a shader program to manage the two
    vertShader = context->glCreateShader(GL_VERTEX_SHADER);
    fragShader = context->glCreateShader(GL_FRAGMENT_SHADER);
    prog = context->glCreateProgram();
    // Get the body of text stored in our two .glsl files
    QString qVertSource = qTextFileRead(vertfile);
    QString qFragSource = qTextFileRead(fragfile);

    char* vertSource = new char[qVertSource.size()+1];
    strcpy(vertSource, qVertSource.toStdString().c_str());
    char* fragSource = new char[qFragSource.size()+1];
    strcpy(fragSource, qFragSource.toStdString().c_str());


    // Send the shader text to OpenGL and store it in the shaders specified by the handles vertShader and fragShader
    context->glShaderSource(vertShader, 1, &vertSource, 0);
    context->glShaderSource(fragShader, 1, &fragSource, 0);
    // Tell OpenGL to compile the shader text stored above
    context->glCompileShader(vertShader);
    context->glCompileShader(fragShader);
    // Check if everything compiled OK
    GLint compiled;
    context->glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(vertShader);
    }
    context->glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        printShaderInfoLog(fragShader);
    }

    // Tell prog that it manages these particular vertex and fragment shaders
    context->glAttachShader(prog, vertShader);
    context->glAttachShader(prog, fragShader);
    context->glLinkProgram(prog);

    // Check for linking success
    GLint linked;
    context->glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        printLinkInfoLog(prog);
    }

    // Get the handles to the variables stored in our shaders
    // See shaderprogram.h for more information about these variables
    attrPos = context->glGetAttribLocation(prog, "vs_Pos");
    attrNor = context->glGetAttribLocation(prog, "vs_Nor");
    attrUV = context->glGetAttribLocation(prog, "vs_UV");
    attrCosPow = context->glGetAttribLocation(prog, "vs_CosPow");
    attrAnim = context->glGetAttribLocation(prog, "vs_Anim");
    attrCol = context->glGetAttribLocation(prog, "vs_Col");

    unifModel      = context->glGetUniformLocation(prog, "u_Model");
    unifModelInvTr = context->glGetUniformLocation(prog, "u_ModelInvTr");
    unifViewProj   = context->glGetUniformLocation(prog, "u_ViewProj");
    unifBlockTexture    = context->glGetUniformLocation(prog, "u_TerrainTexture");
    unifSkyTexture    = context->glGetUniformLocation(prog, "u_SkyTexture");
    unifScreenDimensions    = context->glGetUniformLocation(prog, "u_ScreenDimensions");
    unifPlayerPos  = context->glGetUniformLocation(prog, "u_PlayerPos");
    unifTime       = context->glGetUniformLocation(prog, "u_Time");
    unifCamAttribs       = context->glGetUniformLocation(prog, "u_CameraAttribs");
}

void ShaderProgram::useMe()
{
    context->glUseProgram(prog);
}

void ShaderProgram::setModelMatrix(const glm::mat4 &model)
{
    useMe();

    if (unifModel != -1) {
        // Pass a 4x4 matrix into a uniform variable in our shader
                        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifModel,
                        // How many matrices to pass
                           1,
                        // Transpose the matrix? OpenGL uses column-major, so no.
                           GL_FALSE,
                        // Pointer to the first element of the matrix
                           &model[0][0]);
    }

    if (unifModelInvTr != -1) {
        glm::mat4 modelinvtr = glm::inverse(glm::transpose(model));
        // Pass a 4x4 matrix into a uniform variable in our shader
                        // Handle to the matrix variable on the GPU
        context->glUniformMatrix4fv(unifModelInvTr,
                        // How many matrices to pass
                           1,
                        // Transpose the matrix? OpenGL uses column-major, so no.
                           GL_FALSE,
                        // Pointer to the first element of the matrix
                           &modelinvtr[0][0]);
    }
}

void ShaderProgram::setViewProjMatrix(const glm::mat4 &vp)
{
    // Tell OpenGL to use this shader program for subsequent function calls
    useMe();

    if (unifViewProj != -1) {
    // Pass a 4x4 matrix into a uniform variable in our shader
                    // Handle to the matrix variable on the GPU
    context->glUniformMatrix4fv(unifViewProj,
                    // How many matrices to pass
                       1,
                    // Transpose the matrix? OpenGL uses column-major, so no.
                       GL_FALSE,
                    // Pointer to the first element of the matrix
                       &vp[0][0]);
    }
}

void ShaderProgram::setPlayerPos(const glm::vec3 &pp) {
    useMe();
    if (unifPlayerPos != -1) {
        context->glUniform3f(unifPlayerPos, pp.x, pp.y, pp.z);
    }
}

void ShaderProgram::setTime(float t) {
    useMe();
    if (unifTime != -1) {
        context->glUniform1f(unifTime, t);
    }
}

void ShaderProgram::setBlockTextureSampler(int slot)
{
    useMe();

    if (unifBlockTexture != -1)
    {
        context->glUniform1i(unifBlockTexture, slot);
    }
}


void ShaderProgram::setSkyTextureSampler(int textureSlot)
{
    useMe();

    if (unifSkyTexture != -1)
    {
        context->glUniform1i(unifSkyTexture, textureSlot);
    }
}

void ShaderProgram::setScreenDimensions(int w, int h) {
    useMe();
    if (unifScreenDimensions != -1) {
        context->glUniform2i(unifScreenDimensions, w, h);
    }
}

void ShaderProgram::setCamAttribs(const Camera &cam) {
    useMe();
    if (unifCamAttribs != -1) {
        glm::mat4 attribs {glm::vec4(cam.m_right, 0.f),
                           glm::vec4(cam.m_up, 0.f),
                           glm::vec4(cam.m_forward, 0.f),
                           glm::vec4(cam.m_position, 0.f)};
        context->glUniformMatrix4fv(unifCamAttribs, 1, GL_FALSE, &attribs[0][0]);
    }
}

//This function, as its name implies, uses the passed in GL widget
void ShaderProgram::draw(Drawable &d, bool opaque) {
    useMe();

    if (d.elemCount() < 0) {
        throw std::out_of_range("Attempting to draw a drawable with m_count of " + std::to_string(d.elemCount()) + "!");
    }

    bool (Drawable::*bindAppropriateVBO)(void) = &Drawable::bindOpq;
    if (!opaque) {
        bindAppropriateVBO = &Drawable::bindTra;
    }

    if ((d.*bindAppropriateVBO)()) {
        if (attrPos != -1) {
            context->glEnableVertexAttribArray(attrPos);
            context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 12 * sizeof(float), static_cast<void*>(0));
        }
        if (attrNor != -1) {
            context->glEnableVertexAttribArray(attrNor);
            context->glVertexAttribPointer(attrNor, 4, GL_FLOAT, false, 12 * sizeof(float), (void*)(4 * sizeof(float)));
        }
        if (attrUV != -1) {
            context->glEnableVertexAttribArray(attrUV);
            context->glVertexAttribPointer(attrUV, 2, GL_FLOAT, false, 12 * sizeof(float), (void*)(8 * sizeof(float)));
        }
        if (attrCosPow != -1) {
            context->glEnableVertexAttribArray(attrCosPow);
            context->glVertexAttribPointer(attrCosPow, 1, GL_FLOAT, false, 12 * sizeof(float), (void*)(10 * sizeof(float)));
        }
        if (attrAnim != -1) {
            context->glEnableVertexAttribArray(attrAnim);
            context->glVertexAttribPointer(attrAnim, 1, GL_FLOAT, false, 12 * sizeof(float), (void*)(11 * sizeof(float)));
        }
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdx();
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, nullptr);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
    if (attrUV != -1) context->glDisableVertexAttribArray(attrUV);
    if (attrCosPow != -1) context->glDisableVertexAttribArray(attrCosPow);
    if (attrAnim != -1) context->glDisableVertexAttribArray(attrAnim);

    context->printGLErrorLog();
}

void ShaderProgram::drawScreenSpace(Drawable &d) {
    useMe();
    if (d.elemCount() < 0) {
        throw std::out_of_range("Attempting to draw a drawable with m_count of " + std::to_string(d.elemCount()) + "!");
    }

    if (d.bindOpq()) {
        if (attrPos != -1) {
            context->glEnableVertexAttribArray(attrPos);
            context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 6 * sizeof(float), static_cast<void*>(0));
        }
        if (attrUV != -1) {
            context->glEnableVertexAttribArray(attrUV);
            context->glVertexAttribPointer(attrUV, 2, GL_FLOAT, false, 6 * sizeof(float), (void*)(4 * sizeof(float)));
        }
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdx();
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, nullptr);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrUV != -1) context->glDisableVertexAttribArray(attrUV);

    context->printGLErrorLog();
}


void ShaderProgram::drawWireframe(Drawable &d) {
    useMe();

    if (d.elemCount() < 0) {
        throw std::out_of_range("Attempting to draw a drawable with m_count of " + std::to_string(d.elemCount()) + "!");
    }

    if (d.bindOpq()) {
        if (attrPos != -1) {
            context->glEnableVertexAttribArray(attrPos);
            context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 8 * sizeof(float), static_cast<void*>(0));
        }
        if (attrCol != -1) {
            context->glEnableVertexAttribArray(attrCol);
            context->glVertexAttribPointer(attrCol, 4, GL_FLOAT, false, 8 * sizeof(float), (void*)(4 * sizeof(float)));
        }
    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    d.bindIdx();
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, nullptr);

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    if (attrCol != -1) context->glDisableVertexAttribArray(attrCol);

    context->printGLErrorLog();
}

char* ShaderProgram::textFileRead(const char* fileName) {
    char* text = nullptr;

    if (fileName != nullptr) {
        FILE *file = fopen(fileName, "rt");

        if (file != nullptr) {
            fseek(file, 0, SEEK_END);
            int count = ftell(file);
            rewind(file);

            if (count > 0) {
                text = (char*)malloc(sizeof(char) * (count + 1));
                count = fread(text, sizeof(char), count, file);
                text[count] = '\0';	//cap off the string with a terminal symbol, fixed by Cory
            }
            fclose(file);
        }
    }
    return text;
}

QString ShaderProgram::qTextFileRead(const char *fileName)
{
    QString text;
    QFile file(fileName);
    if (file.open(QFile::ReadOnly))
    {
        QTextStream in(&file);
        text = in.readAll();
        text.append('\0');
    }
    return text;
}

void ShaderProgram::printShaderInfoLog(GLuint shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0)
    {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
        qDebug() << "ShaderInfoLog:" << endl << infoLog << endl;
        delete [] infoLog;
    }

    // should additionally check for OpenGL errors here
}

void ShaderProgram::printLinkInfoLog(int prog)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    context->glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

    // should additionally check for OpenGL errors here

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        context->glGetProgramInfoLog(prog, infoLogLen, &charsWritten, infoLog);
        qDebug() << "LinkInfoLog:" << endl << infoLog << endl;
        delete [] infoLog;
    }
}
