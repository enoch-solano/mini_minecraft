#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include <array>
#include <unordered_map>
#include <unordered_set>
#include "shaderprogram.h"
#include "texture.h"
#include "chunk.h"
#include <QMutex>
#include <QSet>

using namespace std;

// Draw all 64x64 terrain zones in a radius of 1 from our current terrain zone
#define TERRAIN_DRAW_RADIUS 3
// Keep the VBO data of all terrain zones within a radius of 2 from our current zone
#define TERRAIN_CREATE_RADIUS 4

// The container class for all of the Chunks in the game.
// Ultimately, while Terrain will always store all Chunks,
// not all Chunks will be drawn at any given time as the world
// expands.
class Terrain {
private:
    // Stores every Chunk according to the location of its lower-left corner
    // in world space.
    // We combine the X and Z coordinates of the Chunk's corner into one 64-bit int
    // so that we can use them as a key for the map, as objects like std::pairs or
    // glm::ivec2s are not hashable by default, so they cannot be used as keys.
    unordered_map<int64_t, uPtr<Chunk>> m_chunks;

    // We will designate every 64 x 64 area of the world's x-z plane
    // as one "terrain generation zone". Every time the player moves
    // near a portion of the world that has not yet been generated
    // (i.e. its lower-left coordinates are not in this set), a new
    // 4 x 4 collection of Chunks is created to represent that area
    // of the world.
    // The world that exists when the base code is run consists of exactly
    // one 64 x 64 area with its lower-left corner at (0, 0).
    // When milestone 1 has been implemented, the Player can move around the
    // world to add more "terrain generation zone" IDs to this set.
    // While only the 3 x 3 collection of terrain generation zones
    // surrounding the Player should be rendered, the Chunks
    // in the Terrain will never be deleted until the program is terminated.
    QSet<int64_t> m_generatedTerrain;

    OpenGLContext *mp_context;

    Texture m_blocksTexture;

    // Passed to each worker thread so it can pass Chunks that need
    // VBO data to the main thread
    unordered_set<Chunk*> m_chunksThatHaveBlockData;
    QMutex m_chunksThatHaveBlockDataLock;
    vector<ChunkVBOData> m_chunksThatHaveVBOs;
    QMutex m_chunksThatHaveVBOsLock;

public:
    Terrain(OpenGLContext *context);
    ~Terrain();

    // Instantiates a new Chunk and stores it in
    // our chunk map at the given coordinates.
    // Returns a pointer to the instantiated Chunk.
    // DOES NOT CALL create() ON THE CHUNK
    Chunk* instantiateChunkAt(int x, int z);
    // Do these world-space coordinates lie within
    // a Chunk that exists?
    bool hasChunkAt(int x, int z) const;
    // Assuming a Chunk exists at these coords,
    // return a mutable reference to it
    uPtr<Chunk>& getChunkAt(int x, int z);
    // Assuming a Chunk exists at these coords,
    // return a const reference to it
    const uPtr<Chunk>& getChunkAt(int x, int z) const;
    // Given a world-space coordinate (which may have negative
    // values) return the block stored at that point in space.
    BlockType getBlockAt(int x, int y, int z) const;
    BlockType getBlockAt(glm::vec3) const;
    // Given a world-space coordinate (which may have negative
    // values) set the block at that point in space to the
    // given type.
    void setBlockAt(int x, int y, int z, BlockType t);
    void setBlockAt(glm::ivec3 p, BlockType t);

    void changeBlockAt(glm::ivec3 toChange, BlockType t);

    bool terrainZoneExists(int x, int z) const;
    bool terrainZoneExists(int64_t id) const;

    // Draws every Chunk that falls within the bounding box
    // described by the min and max coords, using the provided
    // ShaderProgram
    void draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram, bool opaque);
    void initTexture();

    // Generate procedural terrain height for all the blocks in the given bounding box
    // Also creates the Chunks that will fall into this zone.
    void generateTerrain(int minX, int maxX, int minZ, int maxZ);
    void tryExpansion(glm::vec3 playerPos, glm::vec3 playerPosPrev);
    void spawnFBMWorkers(const QSet<int64_t> &zonesToGenerate);
    void spawnFBMWorker(int64_t zoneToGenerate);
    void spawnVBOWorkers(const std::unordered_set<Chunk *> &chunksNeedingVBOs);
    void spawnVBOWorker(Chunk* chunkNeedingVBOData);
    void checkThreadResults();
    bool initialTerrainDoneLoading() const;
    QSet<int64_t> terrainZonesBorderingZone(glm::ivec2 zoneCoords, unsigned int radius, bool onlyCircumference) const;

    // Initializes the Chunks that store the 64 x 256 x 64 block scene you
    // see when the base code is run.
    void CreateTestScene();
};
