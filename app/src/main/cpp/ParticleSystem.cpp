//
// Created by carlo on 14/06/2025.
//

#include "ParticleSystem.h"
#include <cmath>

void ParticleSystem::spawn(const glm::vec3 &pos, int count) {

    for (int i = 0; i < count; ++i) {
        ParticleInstance& p = particles[firstFree++ % MAX_PARTICLES];
        float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
        float speed = 0.15f + ((float)rand() / RAND_MAX) * 0.15f;
        p.position = pos;
        p.velocity = glm::vec3(cos(angle), sin(angle),0.0f) * speed;
        p.acceleration = glm::vec3(0.0f);
        p.life = p.maxLife = 0.5f + ((float)rand() / RAND_MAX) * 0.3f;
        p.color = glm::vec4(1, 1, 0, 1); // yellowish, can randomize
        p.size = 0.025f + ((float)rand() / RAND_MAX) * 0.025f;
        p.center = pos;
        p.rotation = 0.0f;
        p.active = true;
    }
}

void ParticleSystem::getActiveParticles(std::vector<ParticleInstance>& out) const {
    out.clear();
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (particles[i].active) {
            out.push_back(particles[i]);
        }
    }
}


void ParticleSystem::render(VkCommandBuffer cmd,
                            VkPipelineLayout pipelineLayout,
                            VkPipeline pipeline,
                            VkBuffer vertexBuffer,
                            VkBuffer indexBuffer,
                            VkBuffer instanceBuffer,
                            uint32_t activeCount) {
    if (activeCount == 0) return;

    VkDeviceSize offsets[] = {0, 0};
    VkBuffer vertexBuffers[] = {vertexBuffer, instanceBuffer};

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindVertexBuffers(cmd, 0, 2, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(cmd, 6, activeCount, 0, 0, 0);
}

ParticleSystem::ParticleSystem() {
    for (auto &p : particles) {
        p.position = glm::vec3(0.0f);
        p.velocity = glm::vec3(0.0f);
        p.acceleration = glm::vec3(0.0f);
        p.life = 0.0f;
        p.maxLife = 0.0f;
        p.active = false;
        p.center = glm::vec3(0.0f);
        p.size = 0.0f;
        p.rotation = 0.0f;
        p.color = glm::vec4(0.0f);
    }
}

ParticleSystem::~ParticleSystem() {

}

void ParticleSystem::update(float deltaTime) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        ParticleInstance& p = particles[i];
        if (!p.active) continue;
        p.velocity += p.acceleration * deltaTime;
        p.position += p.velocity * deltaTime;
        p.center = p.position;
        p.life -= deltaTime;
        p.color.a = glm::clamp(p.life / p.maxLife, 0.0f, 1.0f); // Fade out
        if (p.life <= 0) p.active = false;
    }
}
