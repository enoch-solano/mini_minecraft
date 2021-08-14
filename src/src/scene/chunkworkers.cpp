#include "chunkworkers.h"
#include <iostream>


FBMWorker::FBMWorker(int x, int z, std::vector<Chunk*> chunksToFill, std::unordered_set<Chunk *> *chunksCompleted, QMutex* chunksCompletedLock)
    : m_xCorner(x), m_zCorner(z), m_chunksToFill(chunksToFill),
      mp_chunksCompleted(chunksCompleted), mp_chunksCompletedLock(chunksCompletedLock)
{}

BlockType grasslandHeightFill(int y, int noiseHeight) {
    if (y < 96) {
        return STONE;
    }
    else if (y < noiseHeight) {
        return DIRT;
    }
    else {
        return GRASS;
    }
}

BlockType FBMWorker::positionToBlockType(ivec3 pos, int maxHeight, Biome b,
                                         const unordered_map<Biome, float, EnumHash> &individualBiomeHeights) const {
    switch(b) {
    case GRASSLAND:
        if (pos.y < 127 || individualBiomeHeights.at(DESERT) >= 134) {
            return STONE;
        }
        else if (pos.y < maxHeight) {
            return DIRT;
        }
        else if (pos.y < 145 && individualBiomeHeights.at(DESERT) < 134) {
            return GRASS;
        }
        else {
            if (perlinNoise(vec3(pos) / 8.f) > 0.2f) {
                return DIRT;
            }
            else {
                return STONE;
            }
        }
    case DESERT:
        if (pos.y < 127) {
            return STONE;
        }
        // Ensures that only desert pillars will be red clay.
        // Any height above 134 that is from biome interpolation will be sand.
        else if (pos.y < 134 || pos.y > individualBiomeHeights.at(DESERT)) {
            return SAND;
        }
        else {
            return RED_CLAY;
        }
    case MOUNTAIN:
        if (pos.y < 128) {
            return STONE;
        }
        else if (pos.y < 200 || pos.y < maxHeight) {
            if (perlinNoise(vec3(pos) / 8.f) > 0.2f) {
                return DIRT;
            }
            else {
                return STONE;
            }
        }
        else {
            return SNOW;
        }
    case ISLAND:
        if (pos.y < 127) {
            return STONE;
        }
        else if (pos.y < 130) {
            return SAND;
        }
        else if (pos.y < maxHeight) {
            return DIRT;
        }
        else if (pos.y < 145) {
            return GRASS;
        }
        else {
            return STONE;
        }
    default:
        return LAVA;
    }
}

vec2 FBMWorker::computeBiomeSlope(ivec3 pos, Biome b) const {
    switch(b) {
    case DESERT:
        return vec2(desertValue(vec2(pos.x + 1, pos.z)) - desertValue(vec2(pos.x - 1, pos.z)),
                    desertValue(vec2(pos.x, pos.z + 1)) - desertValue(vec2(pos.x, pos.z - 1)));
    default:
        return vec2(0.f);
    }
}

Biome FBMWorker::biomeMap(glm::vec2 val) const {
    if (val.y < 0.5f) {
        if (val.x < 0.5f) {
            return DESERT;
        }
        else {
            return ISLAND;;
        }
    }
    else {
        if (val.x < 0.5f) {
            return GRASSLAND;
        }
        else {
            return MOUNTAIN;
        }
    }
}

// Four quadrants of biome right now
// X is moisture, Y is global height
// (-1,-1) -> (0, 0) is desert
// (-1, 0, -> (0, 1) is mountain
// (0, -1) -> (1, 0) is islands
// (0, 0) -> (1, 1) is grassland

void FBMWorker::run() {
    for (Chunk* c : m_chunksToFill) {
        // Fill Chunks with block data based on noise functions
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                glm::vec2 p(x + c->m_minX, z + c->m_minZ);
                vec2 biome = 0.5f * (biomeValue(p / 750.f) + glm::vec2(1.f)); // [0, 1)
                // Low Y is desert/island, high Y is grassland/mountain
                // Low X is desert/grassland, high X is island/mountain
                biome = smoothstep(0.f, 1.f, smoothstep(0.25f, 0.75f, biome));
                unordered_map<Biome, float, EnumHash> biomeHeights {
                    {GRASSLAND, grasslandValue(p)},
                    {DESERT, desertValue(p)},
                    {MOUNTAIN, mountainValue(p)},
                    {ISLAND, islandValue(p)}
                };

                float mixDesertIsland = mix(biomeHeights[DESERT], biomeHeights[ISLAND], biome.x);
                float mixGrasslandMountain = mix(biomeHeights[GRASSLAND], biomeHeights[MOUNTAIN], biome.x);
                float mixAll = mix(mixDesertIsland, mixGrasslandMountain, biome.y);
                mixAll = clamp(mixAll, 0.f, 255.f);
                Biome currBiome = biomeMap(biome);

                // Fill in terrain
                for (int i = 0; i < mixAll; ++i) {
                    BlockType t = positionToBlockType(ivec3(x, i, z), static_cast<int>(mixAll), currBiome, biomeHeights);
                    c->setBlockAt(x, i, z, t);
                }
                // Do water table
                for (int i = 127; i >= 0; --i) {
                    if (c->getBlockAt(x, i, z) != EMPTY) {
                        break;
                    }
                    c->setBlockAt(x, i, z, WATER);
                }
#if 0
                vec2 biome = 0.5f * (biomeValue(p / 1024.f) + glm::vec2(1.f)); // [0, 1)
                glm::vec4 allBiomes(desertValue(p / 256.f),
                                    mountainValue(p / 256.f),
                                    grasslandValue(p / 256.f),
                                    islandValue(p / 256.f));
                float mixLowX = glm::mix(allBiomes[0], allBiomes[3], glm::smoothstep(0.45f, 0.55f, biome.x));
                float mixHiX = glm::mix(allBiomes[1], allBiomes[2], glm::smoothstep(0.45f, 0.55f, biome.x));
                float interpolatedBiomeHeight = glm::mix(mixLowX, mixHiX, glm::smoothstep(0.45f, 0.55f, biome.y));

                // If the biome value is within 0.1 of
                // 0 on X or Y, then we compute the
                // interpolation
                if (biome.x < 0.5f) {
                    // (0, 0) is desert
                    if (biome.y < 0) {
                        for (int i = 0; i < DESERT_MAX_HEIGHT; ++i) {
                            if (i > interpolatedBiomeHeight) break;
                            c->setBlockAt(x, i, z, SAND);
                        }
                    }
                    // (0, 0.5) is mountain
                    else {
                        for (int i = 0; i < MOUNTAIN_MAX_HEIGHT; ++i) {
                            if (i > interpolatedBiomeHeight) break;
                            c->setBlockAt(x, i, z, STONE);
                        }
                    }
                }
                else {
                    // (0.5, 0) is islands
                    if (biome.y < 0) {
                        for (int i = 0; i < ISLAND_MAX_HEIGHT; ++i) {
                            if (i > interpolatedBiomeHeight) break;
                            c->setBlockAt(x, i, z, SAND);
                        }
                    }
                    // (0.5, 1) is grassland
                    else {
                        for (int i = 0; i < GRASSLAND_MAX_HEIGHT; ++i) {
                            if (i > interpolatedBiomeHeight) break;
                            c->setBlockAt(x, i, z, GRASS);
                        }
                    }
                }
                for (int i = 127; i >= 0; --i) {
                    if (c->getBlockAt(x, i, z) != EMPTY) {
                        break;
                    }
                    c->setBlockAt(x, i, z, WATER);
                }
#endif
#if 0
#define WATER_THRESHOLD 0.125f
                float height = combinedNoise(p / 256.f);
                float water = 10.f * riverNoise(p / 256.f);
                water = water - WATER_THRESHOLD;
                height *= glm::smoothstep(- WATER_THRESHOLD, 1.f - WATER_THRESHOLD, water);
                for (int i = 0; i <= static_cast<int>(height * 16) + 96; ++i) {
                    c->setBlockAt(x, i, z, grasslandHeightFill(i, static_cast<int>(height * 16) + 96));
                }
                // Carve water out based on absolute value Perlin noise
                if (water < 0.f) {
                    for (int i = water; i < 10; ++i) {
                        c->setBlockAt(x, i + 87, z, WATER);
                    }
                    for (int i = 97; i < 256; ++i) {
                        if (c->getBlockAt(x, i, z) == EMPTY) {
                            break;
                        }
                        else {
                            c->setBlockAt(x, i, z, EMPTY);
                        }
                    }
                }
#endif
            }
        }
    }
    mp_chunksCompletedLock->lock();
    for (Chunk* c : m_chunksToFill) {
        // Add the Chunks to the list of Chunks that are ready
        // for VBO creation by the main thread
        mp_chunksCompleted->insert(c);
    }
    mp_chunksCompletedLock->unlock();
}

VBOWorker::VBOWorker(Chunk *c, vector<ChunkVBOData> *dat, QMutex *datLock)
    : mp_chunk(c), mp_chunkVBOsCompleted(dat), mp_chunkVBOsCompletedLock(datLock)
{}


bool isTransparent(BlockType t) {
    return transparent_blocks.find(t) != transparent_blocks.end();
}

void VBOWorker::appendVBOData(vector<float> &vboData, vector<GLuint> &idxData,
                              const BlockFace &f, BlockType curr, ivec3 xyz, unsigned int &maxIdx) {

    int x = xyz.x, y = xyz.y, z = xyz.z;

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

void VBOWorker::run() {
    ChunkVBOData c(mp_chunk);
    std::vector<float> vboDataOpaque, vboDataTransparent;
    std::vector<GLuint> idxDataOpaque, idxDataTransparent;
    GLuint maxIdxOpq = 0, maxIdxTra = 0;
    for (int x = 0; x < 16; ++x) {
        for (int y = 0; y < 256; ++y) {
            for (int z = 0; z < 16; ++z) {
                BlockType curr = mp_chunk->getBlockAt(x, y, z);
                if (curr != EMPTY) {
                    for (const BlockFace &f : adjacentFaces) {
                        BlockType adj;
                        if (crossesBorder(glm::ivec3(x, y, z), f.directionVec)) {
                            Chunk* neighbor = mp_chunk->m_neighbors[f.direction];
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
                            adj = mp_chunk->getBlockAt(x + f.directionVec.x, y + f.directionVec.y, z + f.directionVec.z);
                        }
                        // If the block we're creating faces for is transparent
                        if (isTransparent(curr)) {
                            if (adj == EMPTY) {
                                appendVBOData(c.m_vboDataTransparent, c.m_idxDataTransparent, f, curr, ivec3(x,y,z), maxIdxOpq);
                            }
                        }
                        // If the block we're creating faces for is opaque
                        else {
                            if (isTransparent(adj)) {
                                appendVBOData(c.m_vboDataOpaque, c.m_idxDataOpaque, f, curr, ivec3(x,y,z), maxIdxTra);
                            }
                        }
                    }
                }
            }
        }
    }
    mp_chunkVBOsCompletedLock->lock();
    mp_chunkVBOsCompleted->push_back(c);
    mp_chunkVBOsCompletedLock->unlock();
}
