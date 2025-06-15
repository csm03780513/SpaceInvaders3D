//
// Created by carlo on 14/06/2025.
//

#ifndef SPACEINVADERS3D_PARTICLESYSTEM_H
#define SPACEINVADERS3D_PARTICLESYSTEM_H

#include "GameObjectData.h"


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
};

constexpr int MAX_PARTICLES = 512;

class ParticleSystem {

private:

    int firstFree = 0;

public:
    ParticleSystem();

    ~ParticleSystem();

    ParticleInstance particles[MAX_PARTICLES];

    void spawn(const glm::vec3 &pos, int count);

    void update(float deltaTime);

    void render(VkCommandBuffer cmd,
                VkPipelineLayout pipelineLayout,
                VkPipeline pipeline,
                VkBuffer vertexBuffer,
                VkBuffer indexBuffer,
                VkBuffer instanceBuffer,
                uint32_t activeCount);

    void getActiveParticles(std::vector<ParticleInstance> &out) const;
};


#endif //SPACEINVADERS3D_PARTICLESYSTEM_H
