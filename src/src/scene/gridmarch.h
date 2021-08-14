#pragma once
#include "glm_includes.h"
#include "scene/terrain.h"
#include <iostream>
using namespace glm;

// PURPOSE: To make player movement smoother, and split along all axes
// Allows the player to slide along walls instead of just stopping completely
// Only march along one axis of the world, indicated by the axis input
// 0 -> X, 1 -> Y, 2 -> Z
bool gridMarchAxis(vec3 rayOrigin, vec3 rayDirection, unsigned int axis, const Terrain &terrain, float *out_dist) {
    float maxLen = glm::abs(rayDirection[axis]);
    if (rayDirection[axis] == 0.f) {
        *out_dist = maxLen;
        return false;
    }
    ivec3 currCell = ivec3(floor(rayOrigin));
    rayDirection = normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float offset = glm::max(0.f, glm::sign(rayDirection[axis]));
        int nextIntercept = currCell[axis] + offset;
        float axis_t = (nextIntercept - rayOrigin[axis]) / rayDirection[axis];
        curr_t += axis_t;
        curr_t = glm::min(curr_t, maxLen);
        rayOrigin += rayDirection * glm::min(axis_t, maxLen);
        ivec3 cellOffset = ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        cellOffset[axis] = glm::min(0.f, glm::sign(rayDirection[axis]));
        currCell = ivec3(glm::floor(rayOrigin)) + cellOffset;
        if (currCell.y < 0 || currCell.y >= 256) {
            *out_dist = maxLen;
            return false;
        }

        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);

        if (cellType != EMPTY) {
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

bool gridMarch(vec3 rayOrigin, vec3 rayDirection, const Terrain &terrain, float *out_dist, ivec3 *out_blockHit) {
    float maxLen = length(rayDirection); // Farthest we search
    ivec3 currCell = ivec3(floor(rayOrigin));
    rayDirection = normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for (int i = 0; i < 3; ++i) { // Iterate over the three axes
            if (rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if (currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if (axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if (interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        ivec3 offset = ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if (cellType != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}
