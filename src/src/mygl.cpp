#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QDateTime>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
      m_progLambert(this), m_progFlat(this), m_progSlot(this),
      m_terrain(this), m_player(this, glm::vec3(32.f, 164.f, 32.f), m_terrain),
      m_inputs(this->mapToGlobal(QPoint(width() / 2, height() / 2)).x(), this->mapToGlobal(QPoint(width() / 2, height() / 2)).y()),
      m_skyFrameBuffer(this, this->width(), this->height(), this->devicePixelRatio()),
      m_geomQuad(this), m_progSky(this),
      m_timer(), m_currFrameTime(QDateTime::currentMSecsSinceEpoch()), summed_dTs(0.f),
      m_initialTerrainLoaded(false)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL() {
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.create();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    // Create and set up the slot shader
    m_progSlot.create(":/glsl/slot.vert.glsl", ":/glsl/slot.frag.glsl");

    m_terrain.initTexture();

    m_skyFrameBuffer.create();
    m_geomQuad.create();
    m_progSky.create(":/glsl/passthrough.vert.glsl", ":/glsl/sky.frag.glsl");

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

//    m_terrain.CreateTestScene();
    std::cout << "Generating terrain" << std::endl;
    m_terrain.generateTerrain(-64 * TERRAIN_CREATE_RADIUS, 64 * (TERRAIN_CREATE_RADIUS + 1),
                              -64 * TERRAIN_CREATE_RADIUS, 64 * (TERRAIN_CREATE_RADIUS + 1));
    std::cout << "Done initializing terrain" << std::endl;
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
}

void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setViewProjMatrix(viewproj);
    m_progFlat.setViewProjMatrix(viewproj);

    m_progLambert.setScreenDimensions(w * this->devicePixelRatio(), h * this->devicePixelRatio());
    m_progSky.setScreenDimensions(w * this->devicePixelRatio(), h * this->devicePixelRatio());

    m_skyFrameBuffer.resize(w, h, this->devicePixelRatio());
    m_skyFrameBuffer.destroy();
    m_skyFrameBuffer.create();

    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    // Calculate the change in time since the previous tick
    long long prevTemp = m_currFrameTime;
    m_currFrameTime = QDateTime::currentMSecsSinceEpoch();
    float dT = (m_currFrameTime - prevTemp) * 0.001f; // Seconds elapsed
    summed_dTs += dT;

    m_progLambert.setTime(summed_dTs);
    m_progSky.setTime(summed_dTs);

    if (m_initialTerrainLoaded) {
        // Have the player update their position and physics
        m_player.tick(dT, m_inputs);
        m_progLambert.setPlayerPos(m_player.mcr_position);
    }

    // Check if the terrain should expand
    // This both checks to see if the player is near the border of existing
    // terrain AND checks the status of any FBMWorkers that are generating
    // Chunks
    m_terrain.tryExpansion(m_player.mcr_position, m_player.mcr_posPrev);
    m_terrain.checkThreadResults();

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI();

    if (!m_initialTerrainLoaded) {
        m_initialTerrainLoaded = m_terrain.initialTerrainDoneLoading();
    }
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {

    m_progFlat.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progLambert.setViewProjMatrix(m_player.mcr_camera.getViewProj());
    m_progSky.setCamAttribs(m_player.mcr_camera);

    // Render a sky texture to the frame buffer
    m_skyFrameBuffer.bindFrameBuffer();
    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    glClearColor(1.f, 0.f, 0.f, 1);
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_progSky.drawScreenSpace(m_geomQuad);

    // Re-bind the default frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0,0,this->width(),this->height());
    // Clear the screen so that we only see newly drawn images
    glClearColor(0.37f, 0.74f, 1.0f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Associate the sky texture we just rendered with slot 3
    m_skyFrameBuffer.bindToTextureSlot(SKY_TEXTURE_SLOT);
    m_progLambert.setSkyTextureSampler(SKY_TEXTURE_SLOT);

    // Only draw things once the initial terrain set is completely
    // loaded by the worker threads, to avoid out-of-bounds errors
    // from the maps
    if (m_initialTerrainLoaded) {
        renderTerrain();
        glDisable(GL_DEPTH_TEST);
        m_player.highlightBlock(m_terrain, &m_progFlat); // Outline the block the player would break if they left-clicked
        glEnable(GL_DEPTH_TEST);
    }

    m_progSky.drawScreenSpace(m_geomQuad);
}

// TODO: Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrain() {
    int zoneX = 64 * static_cast<int>(glm::floor(m_player.mcr_position.x / 64.f));
    int zoneZ = 64 * static_cast<int>(glm::floor(m_player.mcr_position.z / 64.f));
    // Draw the 3x3 terrain zones immediately surrounding the player
    // Render opaque first
    m_terrain.draw(zoneX - 64 * TERRAIN_DRAW_RADIUS, zoneX + 64 * TERRAIN_DRAW_RADIUS,
                   zoneZ - 64 * TERRAIN_DRAW_RADIUS, zoneZ + 64 * TERRAIN_DRAW_RADIUS, &m_progLambert,
                   true);
    // Then render transparent
    m_terrain.draw(zoneX - 64 * TERRAIN_DRAW_RADIUS, zoneX + 64 * TERRAIN_DRAW_RADIUS,
                   zoneZ - 64 * TERRAIN_DRAW_RADIUS, zoneZ + 64 * TERRAIN_DRAW_RADIUS, &m_progLambert,
                   false);
}


void MyGL::keyPressEvent(QKeyEvent *e) {
    float amount = 1.0f;
    if (e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }

    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = true;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = true;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = true;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = true;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = true;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = true;
    } else if (e->key() == Qt::Key_F) {
        m_player.toggleFlyMode();
    } else if (e->key() == Qt::Key_Shift) {
        m_inputs.shiftPressed = true;
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = false;
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = false;
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = false;
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = false;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = false;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = false;
    } else if (e->key() == Qt::Key_Shift) {
        m_inputs.shiftPressed = false;
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    m_inputs.mouseXprev = this->width() / 2;
    m_inputs.mouseYprev = this->height() / 2;
    m_inputs.mouseX = e->x();
    m_inputs.mouseY = e->y();
    moveMouseToCenter();
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton) {
        // Place block
        glm::ivec3 toPlace;
        if (m_player.placeBlockCheck(m_terrain, &toPlace)) {
            m_terrain.changeBlockAt(toPlace, STONE);
        }
    }
    else if (e->button() == Qt::LeftButton) {
        // Remove block
        glm::ivec3 toBreak;
        if (m_player.breakBlockCheck(m_terrain, &toBreak)) {
            m_terrain.changeBlockAt(toBreak, EMPTY);
        }
    }
}
