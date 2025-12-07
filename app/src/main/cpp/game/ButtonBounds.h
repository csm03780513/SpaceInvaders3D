#pragma once

#include "../Collision.h"

// Utility helpers to derive button bounding boxes in NDC and test hits.
inline AABB makeButtonAABB(float centerX, float centerY, float halfWidth, float halfHeight) {
    // Collision::getAABB expects full width/height; our caller usually has half extents.
    return Collision::getAABB(centerX, centerY, halfWidth * 2.0f, halfHeight * 2.0f);
}

inline bool isPointInside(const AABB& box, float x, float y) {
    return x >= box.minX && x <= box.maxX &&
           y >= box.minY && y <= box.maxY;
}
