#pragma once
#include "shaderprogram.h"
#include <unordered_map>
#include "smartpointerhelp.h"
#include "chunkhelpers.h"


using namespace std;

// Helper functions to convert (x, z) to and from hash map key
int64_t toKey(int x, int z);
glm::ivec2 toCoords(int64_t k);



// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// TODO have Chunk inherit from Drawable
class Chunk : public Drawable {
private:
    // All of the blocks contained within this Chunk
    array<BlockType, 65536> m_blocks;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    int m_minX, m_minZ;

public:
    Chunk(OpenGLContext *context, int x, int z);
    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void setBlockAt(int x, int y, int z, BlockType t);
    void create() override;
    void create(const std::vector<float> &vboDataOpaque, const vector<GLuint> &idxDataOpaque,
                const std::vector<float> &vboDataTransparent, const vector<GLuint> &idxDataTransparent);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);

    // Allow Terrain to access our private members
    // if it wants to.
    friend class Terrain;
    friend class FBMWorker;
    friend class VBOWorker;
};


struct ChunkVBOData {
    Chunk* mp_chunk;
    vector<float> m_vboDataOpaque, m_vboDataTransparent;
    vector<GLuint> m_idxDataOpaque, m_idxDataTransparent;

    ChunkVBOData(Chunk* c) : mp_chunk(c),
                             m_vboDataOpaque{}, m_vboDataTransparent{},
                             m_idxDataOpaque{}, m_idxDataTransparent{}
    {}
};
