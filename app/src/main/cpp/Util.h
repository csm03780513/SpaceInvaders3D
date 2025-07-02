//
// Created by carlo on 02/07/2025.
//

#ifndef SPACEINVADERS3D_UTIL_H
#define SPACEINVADERS3D_UTIL_H
#include "GameObjectData.h"

struct AABB {
    float minX, minY;
    float maxX, maxY;
};

class Util {
public:
    static std::array<float,2> getQuadWidthHeight(const Vertex *verts, size_t vertsCount);

    void recordDrawBoundingBox(VkCommandBuffer cmd, const AABB &box, const glm::vec3 &color,
                               VkPipeline linePipeline, VkPipelineLayout pipelineLayout,
                               VkBuffer vtxBuffer, VkDeviceSize vtxOffset);
};


#endif //SPACEINVADERS3D_UTIL_H
