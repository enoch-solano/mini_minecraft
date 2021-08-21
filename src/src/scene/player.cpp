#include "player.h"
#include <QString>
#include "gridmarch.h"

#define GRAVITY -35.f
#define MOVE_ACCEL 20.f
#define JUMP_VELOCITY 17.f
#define MOUSE_ROTATION_RADIANS glm::radians(3.f)
#define PLAYER_ARM_LENGTH 4.f

Player::Player(OpenGLContext *context, glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_posPrev(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      m_flyMode(true), m_isGrounded(false), m_blockOutline(context),
      mcr_posPrev(m_posPrev), mcr_camera(m_camera)
{}

Player::~Player()
{}


void Player::toggleFlyMode() {
    if (!m_flyMode) {
        m_acceleration.y = glm::max(0.f, m_acceleration.y);
        m_velocity.y = glm::max(0.f, m_velocity.y);
    }
    m_flyMode = !m_flyMode;
}

void Player::tick(float dT, InputBundle &input) {
    // Player doesn't care about its previous
    // position, so it's safe to update it at the beginning
    // of tick(). Only matters to Terrain when trying to expand.
    m_posPrev = m_position;
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    // Zero out accel for this frame
    m_acceleration = glm::vec3(0.f);
    float speedMod = 1.f;
    if (inputs.shiftPressed) {
        speedMod = 4.f;
    }
    // Handle clamping player movement to XZ plane
    // if not in fly mode
    glm::vec3 forward = m_forward;
    glm::vec3 right = m_right;
    if (!m_flyMode) {
        forward.y = right.y = 0.f;
        forward = glm::normalize(forward);
        right = glm::normalize(right);
    }
    // Apply gravity if we're not grounded and not flying
    if (!m_flyMode && !m_isGrounded) {
        m_acceleration.y += GRAVITY;
    }
    // Handle WASD
    if (inputs.wPressed) {
        m_acceleration += forward * MOVE_ACCEL * speedMod;
    }
    if (inputs.sPressed) {
        m_acceleration -= forward * MOVE_ACCEL * speedMod;
    }
    if (inputs.dPressed) {
        m_acceleration += right * MOVE_ACCEL * speedMod;
    }
    if (inputs.aPressed) {
        m_acceleration -= right * MOVE_ACCEL * speedMod;
    }
    // Handle QE if in fly mode
    if (m_flyMode) {
        if (inputs.qPressed) {
            m_acceleration -= glm::vec3(0,1,0) * MOVE_ACCEL * speedMod;
        }
        if (inputs.ePressed) {
            m_acceleration += glm::vec3(0,1,0) * MOVE_ACCEL * speedMod;
        }
    }
    // Let the player jump if they're on the ground
    if (inputs.spacePressed && !m_flyMode && m_isGrounded) {
        m_velocity.y += JUMP_VELOCITY;
    }
    // Handle look rotation from mouse
    float phi = (inputs.mouseX - inputs.mouseXprev) * MOUSE_ROTATION_RADIANS;
    float theta = (inputs.mouseY - inputs.mouseYprev) * MOUSE_ROTATION_RADIANS;
    rotateOnUpGlobal(-phi);
    rotateOnRightLocal(-theta);
}

const static vector<glm::vec3> corners {
    // Bottom
    glm::vec3(-0.45, 0.0, -0.45),
    glm::vec3(0.45, 0.0, -0.45),
    glm::vec3(0.45, 0.0, 0.45),
    glm::vec3(-0.45, 0.0, 0.45),
    // Mid
    glm::vec3(-0.45, 0.95, -0.45),
    glm::vec3(0.45, 0.95, -0.45),
    glm::vec3(0.45, 0.95, 0.45),
    glm::vec3(-0.45, 0.95, 0.45),
    // Top
    glm::vec3(-0.45, 1.9, -0.45),
    glm::vec3(0.45, 1.9, -0.45),
    glm::vec3(0.45, 1.9, 0.45),
    glm::vec3(-0.45, 1.9, 0.45),
};

void Player::computePhysics(float dT, const Terrain &terrain) {
    // 1. Decay velocity
    m_velocity *= 0.85f;
    // 2. Update velocity based on acceleration
    m_velocity += m_acceleration * dT;
    // 3. Update position based on velocity, while also
    // grid marching to make sure we collide properly
    if (m_flyMode) {
        moveAlongVector(m_velocity * dT);
    }
    else {
        // 1. Project our 12 corners along our velocity
        // across each axis independently
        for (int i = 0; i < 3; ++i) {
            float distToMove = glm::abs(m_velocity[i] * dT);
            for (glm::vec3 corner : corners) {
                glm::vec3 projectedCorner = corner + m_position + m_velocity * dT;
                float collisionDist = distToMove;
                gridMarchAxis(corner + m_position, projectedCorner - (corner + m_position), i,
                              terrain, &collisionDist);
                collisionDist = collisionDist - 0.0001f;
                distToMove = glm::min(distToMove, collisionDist);
            }
            // 2. Only move along the ith axis of our velocity
            glm::vec3 dir = glm::normalize(m_velocity);
            for (int k = 0; k < 3; ++k) {
                if (k != i) {
                    dir[k] = 0.f;
                }
            }
            moveAlongVector(dir * distToMove);
        }
#if 0
        // 1. Project our 12 corners along our velocity
        float distToMove = glm::length(m_velocity * dT);
        for (glm::vec3 corner : corners) {
            glm::vec3 projectedCorner = corner + m_position + m_velocity * dT;
            float collisionDist = distToMove;
            glm::ivec3 blockHit; // Unused in this usage of gridMarch
            gridMarch(corner + m_position, projectedCorner - (corner + m_position), terrain,
                      &collisionDist, &blockHit);
            collisionDist = collisionDist - 0.0001f;
            distToMove = glm::min(distToMove, collisionDist);
        }
        // 2. Move the distance computed
        moveAlongVector(glm::normalize(m_velocity) * distToMove);
#endif
        // 3. Check if grounded
        // Assume we're not, then check if anything is beneath us
        m_isGrounded = false;
        // Iterate over the bottom four corners and check just below them
        for (int i = 0; i < 4; ++i) {
            glm::vec3 belowCorner = corners[i] + m_position - glm::vec3(0, 0.0001f, 0);
            BlockType belowMe = terrain.getBlockAt(belowCorner);
            if (belowMe != EMPTY) {
                m_isGrounded = true;
                m_velocity.y = 0.f;
                break;
            }
        }
    }
}


void Player::highlightBlock(const Terrain &terrain, ShaderProgram *sp) {
    // Lazy coding...
    if (m_blockOutline.elemCount() == -1) {
        m_blockOutline.create();
    }

    float dist = PLAYER_ARM_LENGTH;
    glm::ivec3 blockHit;
    if (gridMarch(m_camera.mcr_position, m_forward * PLAYER_ARM_LENGTH, terrain, &dist, &blockHit)) {
        sp->setModelMatrix(glm::translate(glm::mat4(), glm::vec3(blockHit)));
        sp->drawWireframe(m_blockOutline);
    }
}

bool Player::breakBlockCheck(const Terrain &terrain, glm::ivec3 *out_block) const {
    float dist = PLAYER_ARM_LENGTH;
    glm::ivec3 blockHit;
    if (gridMarch(m_camera.mcr_position, m_forward * PLAYER_ARM_LENGTH, terrain, &dist, &blockHit)) {
        *out_block = blockHit;
        return true;
    }
    return false;
}
bool Player::placeBlockCheck(const Terrain &terrain, glm::ivec3 *out_block) const {
    float dist = PLAYER_ARM_LENGTH;
    glm::ivec3 blockHit;
    if (gridMarch(m_camera.mcr_position, m_forward * PLAYER_ARM_LENGTH, terrain, &dist, &blockHit)) {
        // Do math to determine where to place the block
        glm::vec3 blockCenter = glm::vec3(blockHit) + glm::vec3(0.5f);
        glm::vec3 toSpotHit = (m_camera.mcr_position + m_forward * dist) - blockCenter;
        glm::vec3 toSpotHitAbs = glm::abs(toSpotHit);
        // Find the max cell of the vector from block center to spot hit
        int maxIdx = 0;
        if (toSpotHitAbs.y > toSpotHitAbs.x && toSpotHitAbs.y > toSpotHitAbs.z) {
            maxIdx = 1;
        }
        else if (toSpotHitAbs.z > toSpotHitAbs.x && toSpotHitAbs.z > toSpotHitAbs.y) {
            maxIdx = 2;
        }
        glm::ivec3 blockOffset(0,0,0);
        blockOffset[maxIdx] = glm::sign(toSpotHit[maxIdx]);
        *out_block = blockHit + blockOffset;
        return true;
    }
    return false;
}

void Player::moveForwardGrounded(float amount) {
    // TODO: Move the player along its forward vector
    // as though it were walking along the ground.
    // Also update the camera accordingly.
}

void Player::moveRightGrounded(float amount) {
    // TODO: Move the player along its right vector
    // as though it were walking along the ground.
    // Also update the camera accordingly.
}


void Player::moveAlongVector(glm::vec3 v) {
    Entity::moveAlongVector(v);
    m_camera.moveAlongVector(v);
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}
