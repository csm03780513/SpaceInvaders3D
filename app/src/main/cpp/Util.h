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
    VkDevice device;
    VkBuffer vtxBuffer{VK_NULL_HANDLE};
    VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
    VkPipeline aabbPipeline{VK_NULL_HANDLE};
    VkPipelineLayout aabbPipelineLayout{VK_NULL_HANDLE};
    static std::array<float,2> getQuadWidthHeight(const Vertex *verts, size_t vertsCount);

    void recordDrawBoundingBox(VkCommandBuffer cmd, const AABB& box, const glm::vec3& color);
};


#endif //SPACEINVADERS3D_UTIL_H
