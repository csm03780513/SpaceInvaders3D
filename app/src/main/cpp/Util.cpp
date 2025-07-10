//
// Created by carlo on 02/07/2025.
//

#include "Util.h"

//calculates the width and height of the quad based on the vertices
std::array<float, 2> Util::getQuadWidthHeight(const Vertex *verts, size_t vertsCount) {
    float minX = verts[0].pos[0], maxX = verts[0].pos[0];
    float minY = verts[0].pos[1], maxY = verts[0].pos[1];
    for (size_t i = 1; i < vertsCount; ++i) {
        if (verts[i].pos[0] < minX) minX = verts[i].pos[0];
        if (verts[i].pos[0] > maxX) maxX = verts[i].pos[0];
        if (verts[i].pos[1] < minY) minY = verts[i].pos[1];
        if (verts[i].pos[1] > maxY) maxY = verts[i].pos[1];
    }
    return {maxX - minX, maxY - minY};
}

void Util::recordDrawBoundingBox(VkCommandBuffer cmd, const AABB& box, const glm::vec3& color)
{
    Vertex verts[5] = {
            { {box.minX, box.minY}, {color.r, color.g, color.b} },
            { {box.maxX, box.minY}, {color.r, color.g, color.b} },
            { {box.maxX, box.maxY}, {color.r, color.g, color.b} },
            { {box.minX, box.maxY}, {color.r, color.g, color.b} },
            { {box.minX, box.minY}, {color.r, color.g, color.b} } // close loop
    };

    void *mapped;
    vkMapMemory(device, stagingBufferMemory, 0, sizeof(verts), 0, &mapped);
    memcpy(mapped, verts, sizeof(verts));
    vkUnmapMemory(device, stagingBufferMemory);

    // Assuming vtxBuffer is ready and contains verts...
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, aabbPipeline);
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &vtxBuffer, offsets);
    vkCmdDraw(cmd, 5, 1, 0, 0); // 5 vertices, 1 instance

}
