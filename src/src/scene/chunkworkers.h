#pragma once
#include "noise_functions.h"
#include "chunk.h"
#include <QRunnable>
#include <QMutex>
#include <unordered_set>

enum Biome {
    GRASSLAND, MOUNTAIN, DESERT, ISLAND
};


class FBMWorker : public QRunnable {
private:
    // Coords of the terrain zone being generated
    int m_xCorner, m_zCorner;
    std::vector<Chunk*> m_chunksToFill;
    std::unordered_set<Chunk*>* mp_chunksCompleted;
    QMutex *mp_chunksCompletedLock;

public:
    FBMWorker(int x, int z, std::vector<Chunk*> chunksToFill,
              std::unordered_set<Chunk*>* chunksCompleted, QMutex* chunksCompletedLock);
    void run() override;
    Biome biomeMap(glm::vec2 val) const;
    BlockType positionToBlockType(ivec3 pos, int maxHeight, Biome b, const unordered_map<Biome, float, EnumHash> &individualBiomeHeights) const;
    vec2 computeBiomeSlope(ivec3 pos, Biome b) const;
};

bool isTransparent(BlockType t);

class VBOWorker : public QRunnable {
private:
    Chunk* mp_chunk;
    vector<ChunkVBOData>* mp_chunkVBOsCompleted;
    QMutex *mp_chunkVBOsCompletedLock;

public:
    VBOWorker(Chunk* c, vector<ChunkVBOData>* dat, QMutex *datLock);
    void run() override;
    static void appendVBOData(vector<float> &vbo, vector<GLuint> &idx, const BlockFace &f, BlockType curr, ivec3 xyz, unsigned int &maxIdx);
};
