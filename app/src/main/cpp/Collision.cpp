//
// Created by carlo on 15/07/2025.
//

#include "Collision.h"

bool Collision::isColliding(const AABB &a, const AABB &b) {
    return (a.minX < b.maxX && a.maxX > b.minX &&
            a.minY < b.maxY && a.maxY > b.minY);
}

AABB Collision::getAABB(float cx, float cy, float width, float height) {
    float halfW = width * 0.5f;
    float halfH = height * 0.5f;
    return { cx - halfW, cy - halfH, cx + halfW, cy + halfH };
}
