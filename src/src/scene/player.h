#pragma once
#include "entity.h"
#include "camera.h"
#include "terrain.h"
#include "blockoutline.h"

class Player : public Entity {
private:
    glm::vec3 m_posPrev;
    glm::vec3 m_velocity, m_acceleration;
    Camera m_camera;
    const Terrain &mcr_terrain;
    bool m_flyMode, m_isGrounded;
    BlockOutline m_blockOutline;

    void processInputs(InputBundle &inputs);
    void computePhysics(float dT, const Terrain &terrain);

public:
    // Readonly public reference to our camera
    // for easy access from MyGL
    const glm::vec3 &mcr_posPrev;
    const Camera& mcr_camera;

    Player(OpenGLContext *context, glm::vec3 pos, const Terrain &terrain);
    virtual ~Player() override;

    void setCameraWidthHeight(unsigned int w, unsigned int h);

    void tick(float dT, InputBundle &input) override;
    void highlightBlock(const Terrain &terrain, ShaderProgram *sp);
    // Return the location of the block broken or placed
    bool breakBlockCheck(const Terrain &terrain, glm::ivec3 *out_block) const;
    bool placeBlockCheck(const Terrain &terrain, glm::ivec3 *out_block) const;

    void moveForwardGrounded(float amount);
    void moveRightGrounded(float amount);

    void moveAlongVector(glm::vec3 v) override;

    void toggleFlyMode();

    // Player overrides all of Entity's movement
    // functions so that it transforms its camera
    // by the same amount as it transforms itself.
    void moveForwardLocal(float amount) override;
    void moveRightLocal(float amount) override;
    void moveUpLocal(float amount) override;
    void moveForwardGlobal(float amount) override;
    void moveRightGlobal(float amount) override;
    void moveUpGlobal(float amount) override;
    void rotateOnForwardLocal(float degrees) override;
    void rotateOnRightLocal(float degrees) override;
    void rotateOnUpLocal(float degrees) override;
    void rotateOnForwardGlobal(float degrees) override;
    void rotateOnRightGlobal(float degrees) override;
    void rotateOnUpGlobal(float degrees) override;

    QString posAsQString() const;
    QString velAsQString() const;
    QString accAsQString() const;
    QString lookAsQString() const;
};
