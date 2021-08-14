#include "drawable.h"
#include <glm_includes.h>
#include <iostream>

Drawable::Drawable(OpenGLContext* context)
    : m_count(-1), m_bufIdxOpq(), m_bufIdxTra(), m_bufOpq(), m_bufTra(),
      m_idxOpqGenerated(false), m_idxTraGenerated(false),
      m_opqGenerated(false), m_traGenerated(false),
      m_hasBeenDestroyed(false), mp_context(context)
{}

Drawable::~Drawable()
{}


void Drawable::destroy()
{
    mp_context->glDeleteBuffers(1, &m_bufIdxOpq);
    mp_context->glDeleteBuffers(1, &m_bufOpq);
    mp_context->glDeleteBuffers(1, &m_bufTra);
    m_bufIdxOpq = m_bufOpq = m_bufTra = 0; // Prevent double-deletion
    m_idxOpqGenerated = m_idxTraGenerated = m_opqGenerated = m_traGenerated = false;
    m_count = 0;
    m_hasBeenDestroyed = true;
}

bool Drawable::hasBeenDestroyed() const {
    return m_hasBeenDestroyed;
}

GLenum Drawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int Drawable::elemCount() const
{
    return m_count;
}

void Drawable::generateIdxOpq()
{
    m_idxOpqGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdxOpq);
}


void Drawable::generateIdxTra()
{
    m_idxTraGenerated = true;
    // Create a VBO on our GPU and store its handle in bufIdx
    mp_context->glGenBuffers(1, &m_bufIdxTra);
}

void Drawable::generateOpq()
{
    m_opqGenerated = true;
    // Create a VBO on our GPU and store its handle in bufPos
    mp_context->glGenBuffers(1, &m_bufOpq);
}

void Drawable::generateTra()
{
    m_traGenerated = true;
    // Create a VBO on our GPU and store its handle in bufNor
    mp_context->glGenBuffers(1, &m_bufTra);
}

bool Drawable::bindIdx()
{
    if (m_idxOpqGenerated) {
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    }
    return m_idxOpqGenerated;
}

bool Drawable::bindOpq()
{
    if (m_opqGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufOpq);
    }
    return m_opqGenerated;
}

bool Drawable::bindTra()
{
    if (m_traGenerated){
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufTra);
    }
    return m_traGenerated;
}
