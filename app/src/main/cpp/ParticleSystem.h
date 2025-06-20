//
// Created by carlo on 14/06/2025.
//

#ifndef SPACEINVADERS3D_PARTICLESYSTEM_H
#define SPACEINVADERS3D_PARTICLESYSTEM_H

#include "GameObjectData.h"


struct StarInstance {
    glm::vec3 position;    // NDC or world units
    float speed;           // How fast this star moves down
    float size;            // Size in pixels or NDC
    float brightness;      // White (1.0) or dim (0.2–1.0)

    // Vertex input: just position
    static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindings = {
                {0, sizeof(Vertex),           VK_VERTEX_INPUT_RATE_VERTEX},
                {1, sizeof(StarInstance), VK_VERTEX_INPUT_RATE_INSTANCE}
        };
        return bindings;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributes = {
                // Quad position (location=0)
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex, pos)},
                // Instance data (location=1,2,3,4)
                {1, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(StarInstance, position)},
                {2, 1, VK_FORMAT_R32_SFLOAT,          offsetof(StarInstance, size)},
                {3, 1, VK_FORMAT_R32_SFLOAT,          offsetof(StarInstance, brightness)}
        };
        return attributes;
    }
};


struct ParticleInstance {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    float life;
    float maxLife;
    bool active;
    glm::vec3 center;   // Center in NDC
    float size;         // Size in NDC (or world units if transforming)
    float rotation;     // Radians
    glm::vec4 color;    // RGBA

    // Vertex input: just position
   static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindings = {
                {0, sizeof(Vertex),           VK_VERTEX_INPUT_RATE_VERTEX},
                {1, sizeof(ParticleInstance), VK_VERTEX_INPUT_RATE_INSTANCE}
        };

        return bindings;
    }

   static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
       std::vector<VkVertexInputAttributeDescription> attributes = {
               // Quad position (location=0)
               {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex, pos)},
               // Instance data (location=1,2,3,4)
               {1, 1, VK_FORMAT_R32G32_SFLOAT,       offsetof(ParticleInstance, center)},
               {2, 1, VK_FORMAT_R32_SFLOAT,          offsetof(ParticleInstance, size)},
               {3, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(ParticleInstance, color)}
       };
       return attributes;
   }

};


constexpr int MAX_PARTICLES = 512;
constexpr int NUM_STARS = 256;
class ParticleSystem {

private:
    VkDevice device_;
    std::vector<ParticleInstance> liveParticles;
    int firstFree = 0;
    std::vector<StarInstance> starInstances;
    void initStarField();

public:
    ParticleSystem(VkDevice device);

    ~ParticleSystem();

    ParticleInstance particles[MAX_PARTICLES];


    void spawn(const glm::vec3 &pos, int count);

    void updateExplosionParticles(float deltaTime,VkDeviceMemory particlesInstanceBufferMemory);
    void updateStarField(float deltaTime,VkDeviceMemory starInstanceBufferMemory);

    void render(VkCommandBuffer cmd,
                VkPipelineLayout pipelineLayout,
                VkPipeline pipeline,
                VkBuffer vertexBuffer,
                VkBuffer indexBuffer,
                VkBuffer instanceBuffer,
                GraphicsPipelineType graphicsPipelineType);

    void initExplosionParticles();


};


#endif //SPACEINVADERS3D_PARTICLESYSTEM_H
