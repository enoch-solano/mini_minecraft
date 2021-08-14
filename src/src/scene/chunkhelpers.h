#pragma once
#include "chunk.h"
#include <array>
#include <unordered_set>

using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, LAVA, SAND, SPONGE, RED_CLAY, SNOW
};


// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

const static unordered_set<BlockType, EnumHash> transparent_blocks {
    EMPTY, WATER
};


// Helper function used in Chunk::create()
bool crossesBorder(glm::ivec3 pos, glm::ivec3 dir);


struct VertexData {
    glm::vec4 pos;
    // Relative UV coords, which are offset based on BlockType
    glm::vec2 uv;
    // Absolute uv info is obtained from blockFaceUVs below
    // Surface normal is based on directionVec in BlockFace
    // cosine power and animFlag are dependent on BlockType

    VertexData(glm::vec4 p, glm::vec2 u)
        : pos(p), uv(u)
    {}
};

struct BlockFace {
    Direction direction;
    glm::ivec3 directionVec;
    array<VertexData, 4> vertices;
    BlockFace(Direction dir, glm::ivec3 dirV, const VertexData &a, const VertexData &b, const VertexData &c, const VertexData &d)
        : direction(dir), directionVec(dirV), vertices{a, b, c, d}
    {}
};

// Iterate over this in Chunk::create to check each block
// adjacent to block [x][y][z] and get the relevant vertex info
const static array<BlockFace, 6> adjacentFaces {
    // +X
    BlockFace(XPOS, glm::ivec3(1, 0, 0), VertexData(glm::vec4(1,0,1,1), glm::vec2(0,0)),
                                         VertexData(glm::vec4(1,0,0,1), glm::vec2(0.0625,0)),
                                         VertexData(glm::vec4(1,1,0,1), glm::vec2(0.0625,0.0625)),
                                         VertexData(glm::vec4(1,1,1,1), glm::vec2(0,0.0625))),
    // -X
    BlockFace(XNEG, glm::ivec3(-1, 0, 0), VertexData(glm::vec4(0,0,0,1), glm::vec2(0,0)),
                                          VertexData(glm::vec4(0,0,1,1), glm::vec2(0.0625,0)),
                                          VertexData(glm::vec4(0,1,1,1), glm::vec2(0.0625,0.0625)),
                                          VertexData(glm::vec4(0,1,0,1), glm::vec2(0,0.0625))),
    // +Y
    BlockFace(YPOS, glm::ivec3(0, 1, 0), VertexData(glm::vec4(0,1,1,1), glm::vec2(0,0)),
                                         VertexData(glm::vec4(1,1,1,1), glm::vec2(0.0625,0)),
                                         VertexData(glm::vec4(1,1,0,1), glm::vec2(0.0625,0.0625)),
                                         VertexData(glm::vec4(0,1,0,1), glm::vec2(0,0.0625))),
    // -Y
    BlockFace(YNEG, glm::ivec3(0, -1, 0), VertexData(glm::vec4(0,0,0,1), glm::vec2(0,0)),
                                          VertexData(glm::vec4(1,0,0,1), glm::vec2(0.0625,0)),
                                          VertexData(glm::vec4(1,0,1,1), glm::vec2(0.0625,0.0625)),
                                          VertexData(glm::vec4(0,0,1,1), glm::vec2(0,0.0625))),
    // +Z
    BlockFace(ZPOS, glm::ivec3(0, 0, 1), VertexData(glm::vec4(0,0,1,1), glm::vec2(0,0)),
                                         VertexData(glm::vec4(1,0,1,1), glm::vec2(0.0625,0)),
                                         VertexData(glm::vec4(1,1,1,1), glm::vec2(0.0625,0.0625)),
                                         VertexData(glm::vec4(0,1,1,1), glm::vec2(0,0.0625))),
    // -Z
    BlockFace(ZNEG, glm::ivec3(0, 0, -1), VertexData(glm::vec4(1,0,0,1), glm::vec2(0,0)),
                                          VertexData(glm::vec4(0,0,0,1), glm::vec2(0.0625,0)),
                                          VertexData(glm::vec4(0,1,0,1), glm::vec2(0.0625,0.0625)),
                                          VertexData(glm::vec4(1,1,0,1), glm::vec2(0,0.0625)))
};

const static unordered_map<BlockType, unordered_map<Direction, glm::vec2, EnumHash>, EnumHash> blockFaceUVs {
    {GRASS, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(3.f/16.f, 15.f/16.f)},
                                                          {XNEG, glm::vec2(3.f/16.f, 15.f/16.f)},
                                                          {YPOS, glm::vec2(8.f/16.f, 13.f/16.f)},
                                                          {YNEG, glm::vec2(2.f/16.f, 15.f/16.f)},
                                                          {ZPOS, glm::vec2(3.f/16.f, 15.f/16.f)},
                                                          {ZNEG, glm::vec2(3.f/16.f, 15.f/16.f)}}},
    {DIRT, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(2.f/16.f, 15.f/16.f)},
                                                         {XNEG, glm::vec2(2.f/16.f, 15.f/16.f)},
                                                         {YPOS, glm::vec2(2.f/16.f, 15.f/16.f)},
                                                         {YNEG, glm::vec2(2.f/16.f, 15.f/16.f)},
                                                         {ZPOS, glm::vec2(2.f/16.f, 15.f/16.f)},
                                                         {ZNEG, glm::vec2(2.f/16.f, 15.f/16.f)}}},
    {STONE, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(1.f/16.f, 15.f/16.f)},
                                                          {XNEG, glm::vec2(1.f/16.f, 15.f/16.f)},
                                                          {YPOS, glm::vec2(1.f/16.f, 15.f/16.f)},
                                                          {YNEG, glm::vec2(1.f/16.f, 15.f/16.f)},
                                                          {ZPOS, glm::vec2(1.f/16.f, 15.f/16.f)},
                                                          {ZNEG, glm::vec2(1.f/16.f, 15.f/16.f)}}},
    {SAND, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(2.f/16.f, 14.f/16.f)},
                                                         {XNEG, glm::vec2(2.f/16.f, 14.f/16.f)},
                                                         {YPOS, glm::vec2(2.f/16.f, 14.f/16.f)},
                                                         {YNEG, glm::vec2(2.f/16.f, 14.f/16.f)},
                                                         {ZPOS, glm::vec2(2.f/16.f, 14.f/16.f)},
                                                         {ZNEG, glm::vec2(2.f/16.f, 14.f/16.f)}}},
    {LAVA, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(14.f/16.f, 1.f/16.f)},
                                                         {XNEG, glm::vec2(14.f/16.f, 1.f/16.f)},
                                                         {YPOS, glm::vec2(14.f/16.f, 1.f/16.f)},
                                                         {YNEG, glm::vec2(14.f/16.f, 1.f/16.f)},
                                                         {ZPOS, glm::vec2(14.f/16.f, 1.f/16.f)},
                                                         {ZNEG, glm::vec2(14.f/16.f, 1.f/16.f)}}},
    {WATER, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(14.f/16.f, 3.f/16.f)},
                                                          {XNEG, glm::vec2(14.f/16.f, 3.f/16.f)},
                                                          {YPOS, glm::vec2(14.f/16.f, 3.f/16.f)},
                                                          {YNEG, glm::vec2(14.f/16.f, 3.f/16.f)},
                                                          {ZPOS, glm::vec2(14.f/16.f, 3.f/16.f)},
                                                          {ZNEG, glm::vec2(14.f/16.f, 3.f/16.f)}}},
    {SPONGE, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(0.f/16.f, 12.f/16.f)},
                                                           {XNEG, glm::vec2(0.f/16.f, 12.f/16.f)},
                                                           {YPOS, glm::vec2(0.f/16.f, 12.f/16.f)},
                                                           {YNEG, glm::vec2(0.f/16.f, 12.f/16.f)},
                                                           {ZPOS, glm::vec2(0.f/16.f, 12.f/16.f)},
                                                           {ZNEG, glm::vec2(0.f/16.f, 12.f/16.f)}}},
    {RED_CLAY, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(8.f/16.f, 5.f/16.f)},
                                                             {XNEG, glm::vec2(8.f/16.f, 5.f/16.f)},
                                                             {YPOS, glm::vec2(8.f/16.f, 5.f/16.f)},
                                                             {YNEG, glm::vec2(8.f/16.f, 5.f/16.f)},
                                                             {ZPOS, glm::vec2(8.f/16.f, 5.f/16.f)},
                                                             {ZNEG, glm::vec2(8.f/16.f, 5.f/16.f)}}},
    {SNOW, unordered_map<Direction, glm::vec2, EnumHash>{{XPOS, glm::vec2(2.f/16.f, 11.f/16.f)},
                                                         {XNEG, glm::vec2(2.f/16.f, 11.f/16.f)},
                                                         {YPOS, glm::vec2(2.f/16.f, 11.f/16.f)},
                                                         {YNEG, glm::vec2(2.f/16.f, 11.f/16.f)},
                                                         {ZPOS, glm::vec2(2.f/16.f, 11.f/16.f)},
                                                         {ZNEG, glm::vec2(2.f/16.f, 11.f/16.f)}}}
};
