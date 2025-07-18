//
// Created by carlo on 15/07/2025.
//

#ifndef SPACEINVADERS3D_COLLISION_H
#define SPACEINVADERS3D_COLLISION_H

struct AABB {
    float minX, minY;
    float maxX, maxY;
};

class Collision {
public:
    static bool isColliding(const AABB& a, const AABB& b);
    static AABB getAABB(float cx, float cy, float width, float height);

};


#endif //SPACEINVADERS3D_COLLISION_H
