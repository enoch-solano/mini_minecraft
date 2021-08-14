#include "scene/terrain.h"
#include <stdexcept>
#include <iostream>
#include "noise_functions.h"
#include "chunkworkers.h"
#include <QThreadPool>

Chunk::Chunk(OpenGLContext *context, int x, int z)
    : Drawable(context), m_blocks(),
      m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr},
                  {ZNEG, nullptr}, {YPOS, nullptr}, {YNEG, nullptr}},
      m_minX(x), m_minZ(z)
{
    fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    try {
        return m_blocks.at(x + 16 * y + 16 * 256 * z);
    }
    catch(std::out_of_range &e) {
        std::cout << e.what() << std::endl;
        std::cout << x << ", " << y << ", " << z << std::endl;
    }
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {
    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}
void Chunk::setBlockAt(int x, int y, int z, BlockType t) {
    setBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z), t);
}

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), mp_context(context), m_blocksTexture(context),
      m_chunksThatHaveBlockData(), m_chunksThatHaveBlockDataLock(), m_chunksThatHaveVBOs(), m_chunksThatHaveVBOsLock()
{}

Terrain::~Terrain() {
    // Destroy all the Chunks
    for (auto &x : this->m_chunks) {
        x.second->destroy();
    }
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if (z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if (hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.d
        if (y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        int xMapped = x % 16;
        if (xMapped < 0) xMapped += 16;
        int zMapped = z % 16;
        if (zMapped < 0) zMapped += 16;
        return c->getBlockAt(static_cast<unsigned int>(xMapped),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(zMapped));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = glm::floor(x / 16.f);
    int zFloor = glm::floor(z / 16.f);
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t) {
    if (hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if (y < 0 || y >= 256) {
            return;
        }
        uPtr<Chunk> &c = getChunkAt(x, z);
        vec2 chunkOrigin = vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

void Terrain::setBlockAt(ivec3 p, BlockType t) {
    setBlockAt(p.x, p.y, p.z, t);
}


void Terrain::changeBlockAt(glm::ivec3 toChange, BlockType t) {
    setBlockAt(toChange, t);
    uPtr<Chunk>& chunk = getChunkAt(toChange.x, toChange.z);
    chunk->destroy();
    chunk->create();
    // Also check if we need to re-create any neighboring Chunks
    int localChunkX = toChange.x - 16 * glm::floor(toChange.x / 16.f);
    int localChunkZ = toChange.z - 16 * glm::floor(toChange.x / 16.f);

    // +X
    if (crossesBorder(glm::ivec3(localChunkX, toChange.y, localChunkZ), glm::ivec3(1,0,0))) {
        if (chunk->m_neighbors[XPOS] != nullptr) {
            chunk->m_neighbors[XPOS]->destroy();
            chunk->m_neighbors[XPOS]->create();
        }
    }
    // -X
    if (crossesBorder(glm::ivec3(localChunkX, toChange.y, localChunkZ), glm::ivec3(-1,0,0))) {
        if (chunk->m_neighbors[XNEG] != nullptr) {
            chunk->m_neighbors[XNEG]->destroy();
            chunk->m_neighbors[XNEG]->create();
        }
    }
    // +Z
    if (crossesBorder(glm::ivec3(localChunkX, toChange.y, localChunkZ), glm::ivec3(0,0,1))) {
        if (chunk->m_neighbors[ZPOS] != nullptr) {
            chunk->m_neighbors[ZPOS]->destroy();
            chunk->m_neighbors[ZPOS]->create();
        }
    }
    // -Z
    if (crossesBorder(glm::ivec3(localChunkX, toChange.y, localChunkZ), glm::ivec3(0,0,-1))) {
        if (chunk->m_neighbors[ZNEG] != nullptr) {
            chunk->m_neighbors[ZNEG]->destroy();
            chunk->m_neighbors[ZNEG]->create();
        }
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    // Instantiate the chunk and put it into the map
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context, x, z);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if (hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if (hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if (hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if (hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    return cPtr;
}

const static unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if (neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

bool crossesBorder(glm::ivec3 pos, glm::ivec3 dir) {
    int dx = (pos.x % 16 + dir.x);
    int dy = (pos.y % 256 + dir.y);
    int dz = (pos.z % 16 + dir.z);
    return dx < 0 || dy < 0 || dz < 0 || dx >= 16 || dy >= 256 || dz >= 16;
}

void Chunk::create() {
    std::vector<float> vboData;
    std::vector<GLuint> idxData;
    GLuint maxIdx = 0;
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 256; ++y) {
            for (int z = 0; z < 16; ++z) {
                BlockType curr = getBlockAt(x, y, z);
                if (curr != EMPTY) {
                    for (const BlockFace &f : adjacentFaces) {
                        BlockType adj;
                        if (crossesBorder(glm::ivec3(x, y, z), f.directionVec)) {
                            Chunk* neighbor = m_neighbors[f.direction];
                            if (neighbor == nullptr) {
                                adj = EMPTY;
                            }
                            else {
                                int dx = (x + f.directionVec.x) % 16;
                                if (dx < 0) dx = 16 + dx;
                                int dy = (y + f.directionVec.y) % 256;
                                if (dy < 0) dy = 256 + dy;
                                int dz = (z + f.directionVec.z) % 16;
                                if (dz < 0) dz = 16 + dz;
                                adj = neighbor->getBlockAt(dx, dy, dz);
                            }
                        }
                        else {
                            adj = getBlockAt(x + f.directionVec.x, y + f.directionVec.y, z + f.directionVec.z);
                        }
                        // TODO: Handle transparent stuff differently
                        if (adj == EMPTY) {
                            const array<VertexData, 4> &vertDat = f.vertices;
                            glm::vec2 uvOffset = blockFaceUVs.at(curr).at(f.direction);
                            for (const VertexData &vd : vertDat) {
                                // Pos
                                vboData.push_back(vd.pos.x + x);
                                vboData.push_back(vd.pos.y + y);
                                vboData.push_back(vd.pos.z + z);
                                vboData.push_back(vd.pos.w);
                                // Nor
                                vboData.push_back(f.directionVec.x);
                                vboData.push_back(f.directionVec.y);
                                vboData.push_back(f.directionVec.z);
                                vboData.push_back(0);
                                // UV
                                vboData.push_back(vd.uv.x + uvOffset.x);
                                vboData.push_back(vd.uv.y + uvOffset.y);
                                // Cosine power
                                // TODO make this not hard-coded
                                vboData.push_back(1.f);
                                // Anim flag
                                // TODO make this not hard-coded
                                vboData.push_back(0.f);
                            }
                            idxData.push_back(0 + maxIdx);
                            idxData.push_back(1 + maxIdx);
                            idxData.push_back(2 + maxIdx);
                            idxData.push_back(0 + maxIdx);
                            idxData.push_back(2 + maxIdx);
                            idxData.push_back(3 + maxIdx);
                            maxIdx += 4;
                        }
                    }
                }
            }
        }
    }

    m_count = static_cast<int>(idxData.size());
    generateIdxOpq();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxData.size() * sizeof(GLuint), idxData.data(), GL_STATIC_DRAW);
    generateOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufOpq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(float), vboData.data(), GL_STATIC_DRAW);
}


void Chunk::create(const std::vector<float> &vboDataOpaque, const vector<GLuint> &idxDataOpaque, const std::vector<float> &vboDataTransparent, const vector<GLuint> &idxDataTransparent) {
    m_count = static_cast<int>(idxDataOpaque.size());
    generateIdxOpq();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxOpq);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxDataOpaque.size() * sizeof(GLuint), idxDataOpaque.data(), GL_STATIC_DRAW);
    generateIdxTra();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdxTra);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxDataTransparent.size() * sizeof(GLuint), idxDataTransparent.data(), GL_STATIC_DRAW);
    generateOpq();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufOpq);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboDataOpaque.size() * sizeof(float), vboDataOpaque.data(), GL_STATIC_DRAW);
    generateTra();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufTra);
    mp_context->glBufferData(GL_ARRAY_BUFFER, vboDataTransparent.size() * sizeof(float), vboDataTransparent.data(), GL_STATIC_DRAW);
}

void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram, bool opaque) {
    m_blocksTexture.bind(MINECRAFT_BLOCK_TEXTURE_SLOT);
    shaderProgram->setBlockTextureSampler(MINECRAFT_BLOCK_TEXTURE_SLOT);
    for (int x = minX; x <= maxX; x += 16) {
        for (int z = minZ; z <= maxZ; z += 16) {
            if (hasChunkAt(x, z)) {
                auto &chunk = m_chunks.at(toKey(x, z));
                if (chunk->elemCount() > 0) {
                    shaderProgram->setModelMatrix(glm::translate(glm::mat4(), glm::vec3(x, 0, z)));
                    shaderProgram->draw(*chunk, opaque);
                }
            }
        }
    }
}

void Terrain::initTexture() {
    m_blocksTexture.create(":/textures/minecraft_textures_all.png");
    m_blocksTexture.load(MINECRAFT_BLOCK_TEXTURE_SLOT);
}

void Terrain::generateTerrain(int minX, int maxX, int minZ, int maxZ) {
    // Multithreaded version of initial terrain gen
    for (int x = minX; x < maxX; x += 64) {
        for (int z = minZ; z < maxZ; z += 64) {
            int64_t terrainZoneKey = toKey(x, z);
            m_generatedTerrain.insert(terrainZoneKey);
        }
    }
    spawnFBMWorkers(m_generatedTerrain);

#if 0
    // Spawn Chunks
    for (int x = minX; x < maxX; x += 16) {
        for (int z = minZ; z < maxZ; z += 16) {
            instantiateChunkAt(x, z);
        }
    }
    // Update our zones of generated terrain to contain
    // the new Chunks
    for (int x = minX; x < maxX; x += 64) {
        for (int z = minZ; z < maxZ; z += 64) {
            int64_t terrainZoneKey = toKey(x, z);
            m_generatedTerrain.insert(terrainZoneKey);
        }
    }
    // Fill Chunks with block data based on noise functions
    for (int x = minX; x < maxX; ++x) {
        for (int z = minZ; z < maxZ; ++z) {
            float height = combinedNoise(glm::vec2(x, z) / 256.f);
            for (int i = 0; i <= static_cast<int>(height * 32) + 96; ++i) {
                setBlockAt(x, i, z, GRASS);
            }
        }
    }
    // Create VBO data for the Chunks
    for (int x = minX; x < maxX; x += 16) {
        for (int z = minZ; z < maxZ; z += 16) {
            int64_t key = toKey(x, z);
            auto &chunk = m_chunks[key];
            chunk->create();
        }
    }
#endif
}


bool Terrain::terrainZoneExists(int x, int z) const {
    int64_t key = toKey(x, z);
    return terrainZoneExists(key);
}
bool Terrain::terrainZoneExists(int64_t id) const {
    return this->m_generatedTerrain.contains(id);
}

// Just tells us what zones border us, but DOESN'T
QSet<int64_t> Terrain::terrainZonesBorderingZone(ivec2 zoneCoords, unsigned int radius, bool onlyCircumference) const {
    int radiusInZoneScale = static_cast<int>(radius) * 64;
    QSet<int64_t> result;
    // Only want to look at terrain zones exactly at our radius
    if (onlyCircumference) {
        for (int i = -radiusInZoneScale; i <= radiusInZoneScale; i += 64) {
            // Nx1 to the right
            result.insert(toKey(zoneCoords.x + radiusInZoneScale, zoneCoords.y + i));
            // Nx1 to the left
            result.insert(toKey(zoneCoords.x -radiusInZoneScale, zoneCoords.y + i));
            // Nx1 above
            result.insert(toKey(zoneCoords.x + i, zoneCoords.y + radiusInZoneScale));
            // Nx1 below
            result.insert(toKey(zoneCoords.x + i, zoneCoords.y - radiusInZoneScale));
        }
    }
    // Want to look at all terrain zones encompassed by the radius
    else {
        for (int i = -radiusInZoneScale; i <= radiusInZoneScale; i += 64) {
            for (int j = -radiusInZoneScale; j <= radiusInZoneScale; j += 64) {
                result.insert(toKey(zoneCoords.x + i, zoneCoords.y + j));
            }
        }
    }
    return result;
}

void Terrain::tryExpansion(glm::vec3 playerPos, glm::vec3 playerPosPrev) {
    // Find the player's position relative
    // to their current terrain gen zone
    ivec2 currZone(64.f * glm::floor(playerPos.x / 64.f), 64.f * glm::floor(playerPos.z / 64.f));
    ivec2 prevZone(64.f * glm::floor(playerPosPrev.x / 64.f), 64.f * glm::floor(playerPosPrev.z / 64.f));
    // Determine which terrain zones border our current position and our previous position
    // This *will* include un-generated terrain zones, so we can compare them to our global set
    // and know to generate them
    QSet<int64_t> terrainZonesBorderingCurrPos = terrainZonesBorderingZone(currZone, TERRAIN_CREATE_RADIUS, false);
    QSet<int64_t> terrainZonesBorderingPrevPos = terrainZonesBorderingZone(prevZone, TERRAIN_CREATE_RADIUS, false);
    // Check which terrain zones need to be destroy()ed
    // by determining which terrain zones were previously in our radius and are now not
    for (auto id : terrainZonesBorderingPrevPos) {
        if (!terrainZonesBorderingCurrPos.contains(id)) {
            ivec2 coord = toCoords(id);
            for (int x = coord.x; x < coord.x + 64; x += 16) {
                for (int z = coord.y; z < coord.y + 64; z += 16) {
                    auto &chunk = getChunkAt(x, z);
                    chunk->destroy();
                }
            }
        }
    }

    // Determine if any terrain zones around our current position need VBO data.
    // Send these to VBOWorkers.
    // DO NOT send zones to workers if they do not exist in our global map.
    // Instead, send these to FBMWorkers.
    for (auto id : terrainZonesBorderingCurrPos) {
        // If it exists already AND IS NOT IN PREV SET, send it to a VBOWorker
        // If it's in the prev set, then it's already been sent to a VBOWorker
        // at some point, and may even already have VBOs
        if (terrainZoneExists(id)) {
            if (!terrainZonesBorderingPrevPos.contains(id)) {
                ivec2 coord = toCoords(id);
                for (int x = coord.x; x < coord.x + 64; x += 16) {
                    for (int z = coord.y; z < coord.y + 64; z += 16) {
                        auto &chunk = getChunkAt(x, z);
                        spawnVBOWorker(chunk.get());
                    }
                }
            }
        }
        // If it does not yet exist, send it to an FBMWorker
        // This also adds it to the set of generated terrain zones
        // so we don't try to repeatedly generate it.
        else {
            spawnFBMWorker(id);
        }
    }
}

// TODO: Change how this works so that Chunks
// store their VBO data so that it can be re-sent
// to the GPU, allowing us to destroy() Chunks that
// are outside the 5x5 grid. Also need to re-create()
// Chunks that re-enter our 5x5 radius
#if 0
bool Terrain::tryExpansion(glm::vec3 playerPos) {
    // Find the player's position relative
    // to their current terrain gen zone
    int zoneX = 64.f * glm::floor(playerPos.x / 64.f);
    int zoneZ = 64.f * glm::floor(playerPos.z / 64.f);

    std::unordered_set<int64_t> zonesToBeGenerated;
    for (int i = -128; i <= 128; i += 64) {
        // 5x1 to the right
        if (!terrainZoneExists(zoneX + 128, zoneZ + i)) {
            zonesToBeGenerated.insert(toKey(zoneX + 128, zoneZ + i));
        }
        // 5x1 to the left
        if (!terrainZoneExists(zoneX - 128, zoneZ + i)) {
            zonesToBeGenerated.insert(toKey(zoneX -128, zoneZ + i));
        }
        // 5x1 above
        if (!terrainZoneExists(zoneX + i, zoneZ + 128)) {
            zonesToBeGenerated.insert(toKey(zoneX + i, zoneZ + 128));
        }
        // 5x1 below
        if (!terrainZoneExists(zoneX + i, zoneZ - 128)) {
            zonesToBeGenerated.insert(toKey(zoneX + i, zoneZ - 128));
        }
    }

    // Fills m_chunksThatNeedVBOs with Chunk*s that have
    // been given FBM data but do not have VBO float data
    spawnFBMWorkers(zonesToBeGenerated);

    // Since this function is called every tick, also
    // check on the vector of Chunks that have FBM data
    // and send them to worker threads to be given VBO data
    m_chunksThatNeedVBOsLock.lock();
    // After locking m_chunksThatNeedVBOs, add
    // to it Chunk*s that were previously create()d,
    // but were de-spawned when we moved far away.
    // These would be Chunk*s that are now in our 5x5
    // radius but do not have VBO data because they were
    // destroy()ed
    spawnVBOWorkers(m_chunksThatNeedVBOs);
    m_chunksThatNeedVBOs.clear();
    m_chunksThatNeedVBOsLock.unlock();

    // Check on the Chunks that have VBO data and pass
    // that VBO data to the GPU (needs to be done from
    // the main thread)
    m_chunksThatHaveVBOsLock.lock();
    for (ChunkVBOData cd : m_chunksThatHaveVBOs) {
        cd.mp_chunk->create(cd.m_vboData, cd.m_idxData);
    }
    m_chunksThatHaveVBOs.clear();
    m_chunksThatHaveVBOsLock.unlock();

    return zonesToBeGenerated.size() > 0;

    /*
    Always draw the nine zones around the player.
    The pre-generated world should be 5x5 so that
    enough exists to have un-rendered when player begins.
    When player moves to a zone with a neighbor that has
    an un-created neighbor, then generate three more zones
    in the direction the player is moving.
    In other words, search ring of current + (-128, -128) to current + (128, 128)
    ---------
   |  |  |  |
   ----------
   |  |  |  |
   ----------
   |  |  |  |
    ---------

      */
}
#endif

void Terrain::spawnFBMWorker(int64_t zoneToGenerate) {
    m_generatedTerrain.insert(zoneToGenerate);
    vector<Chunk*> chunksForWorker;
    glm::ivec2 coords = toCoords(zoneToGenerate);
    for (int x = coords.x; x < coords.x + 64; x += 16) {
        for (int z = coords.y; z < coords.y + 64; z += 16) {
            Chunk* c = instantiateChunkAt(x, z);
            c->m_count = 0; // Allow it to be "drawn" even with no VBO data
            chunksForWorker.push_back(c);
        }
    }
    FBMWorker *worker = new FBMWorker(coords.x, coords.y, chunksForWorker,
                                      &m_chunksThatHaveBlockData, &m_chunksThatHaveBlockDataLock);
    QThreadPool::globalInstance()->start(worker);
}

void Terrain::spawnFBMWorkers(const QSet<int64_t> &zonesToGenerate) {
    // Spawn worker threads to generate more Chunks
    for (int64_t zone : zonesToGenerate) {
        spawnFBMWorker(zone);
    }
}

void Terrain::spawnVBOWorker(Chunk* chunkNeedingVBOData) {
    VBOWorker *worker = new VBOWorker(chunkNeedingVBOData, &m_chunksThatHaveVBOs, &m_chunksThatHaveVBOsLock);
    QThreadPool::globalInstance()->start(worker);
}

void Terrain::spawnVBOWorkers(const std::unordered_set<Chunk*> &chunksNeedingVBOs) {
    for (Chunk* c : chunksNeedingVBOs) {
        spawnVBOWorker(c);
    }
}

void Terrain::checkThreadResults() {
    // Send Chunks that have been processed by FBMWorkers
    // to VBOWorkers for VBO data
    m_chunksThatHaveBlockDataLock.lock();
    spawnVBOWorkers(m_chunksThatHaveBlockData);
    m_chunksThatHaveBlockData.clear();
    m_chunksThatHaveBlockDataLock.unlock();

    // Collect the Chunks that have been given VBO data
    // by VBOWorkers and send that VBO data to the GPU.
    m_chunksThatHaveVBOsLock.lock();
    for (ChunkVBOData cd : m_chunksThatHaveVBOs) {
        cd.mp_chunk->create(cd.m_vboDataOpaque, cd.m_idxDataOpaque,
                            cd.m_vboDataTransparent, cd.m_idxDataTransparent);
    }
    m_chunksThatHaveVBOs.clear();
    m_chunksThatHaveVBOsLock.unlock();
}

bool Terrain::initialTerrainDoneLoading() const {
    // 4 Chunks to a side of a terrain zone.
    // Radius x Radius number of terrain zones.
    // Therefore, total Chunks needed is r^2 * 4^2
    return m_chunks.size() >= TERRAIN_CREATE_RADIUS * 4 * TERRAIN_CREATE_RADIUS * 4;
}

void Terrain::CreateTestScene()
{
    // Create the Chunks that will
    // store the blocks for our
    // initial world space
    for (int x = 0; x < 64; x += 16) {
        for (int z = 0; z < 64; z += 16) {
            instantiateChunkAt(x, z);
        }
    }
    // Tell our existing terrain set that
    // the "generated terrain zone" at (0,0)
    // now exists.
    m_generatedTerrain.insert(toKey(0, 0));

    // Create the basic terrain floor
    for (int x = 0; x < 64; ++x) {
        for (int z = 0; z < 64; ++z) {
            if ((x + z) % 2 == 0) {
                setBlockAt(x, 128, z, STONE);
            }
            else {
                setBlockAt(x, 128, z, DIRT);
            }
        }
    }
    // Add "walls" for collision testing
    for (int x = 0; x < 64; ++x) {
        setBlockAt(x, 129, 0, GRASS);
        setBlockAt(x, 130, 0, GRASS);
        setBlockAt(x, 129, 63, GRASS);
        setBlockAt(0, 130, x, GRASS);
    }
    // Add a central column
    for (int y = 129; y < 140; ++y) {
        setBlockAt(32, y, 32, GRASS);
    }
#if 0
    for (int x = 0; x < 64; x += 16) {
        for (int z = 0; z < 64; z += 16) {
            setBlockAt(x, 128, z, GRASS);
        }
    }
#endif


    // Don't forget to create the Chunks!
    for (auto &kvp : m_chunks) {
        kvp.second->create();
    }
}
