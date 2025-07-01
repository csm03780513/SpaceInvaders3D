//
// Created by carlo on 14/06/2025.
//

#include "ParticleSystem.h"


std::mt19937 rng(std::random_device{}());
std::uniform_real_distribution<float> xDist(-1.0f, 1.0f);
std::uniform_real_distribution<float> yDist(-1.0f, 1.0f);
std::uniform_real_distribution<float> speedDist(0.05f, 0.3f);
std::uniform_real_distribution<float> sizeDist(0.005f, 0.015f);
std::uniform_real_distribution<float> brightDist(0.3f, 1.0f);


void ParticleSystem::spawn(const glm::vec3 &pos, int count) {

    for (int i = 0; i < count; ++i) {
        ParticleInstance &p = particles[firstFree++ % MAX_PARTICLES];
        float angle = ((float) rand() / RAND_MAX) * 2.0f * M_PI;
        float speed = 0.15f + ((float) rand() / RAND_MAX) * 0.15f;
        p.position = pos;
        p.velocity = glm::vec3(cos(angle), sin(angle), 0.0f) * speed;
        p.acceleration = glm::vec3(0.0f);
        p.life = p.maxLife = 0.5f + ((float) rand() / RAND_MAX) * 0.3f;
        p.color = glm::vec4(1, 0.5, 0, 1); // yellowish, can randomize
        p.size = 0.005f + ((float) rand() / RAND_MAX) * 0.005f;
        p.center = pos;
        p.rotation = 0.0f;
        p.active = true;
    }
}


void ParticleSystem::recordCommandBuffer(VkCommandBuffer cmd,
                                         VkPipelineLayout pipelineLayout,
                                         VkPipeline pipeline,
                                         VkBuffer vertexBuffer,
                                         VkBuffer indexBuffer,
                                         VkBuffer instanceBuffer,
                                         GraphicsPipelineType graphicsPipelineType) {
    if(graphicsPipelineType == GraphicsPipelineType::ExplosionParticles) {
        if (liveParticles.empty()) return;

        VkDeviceSize offsets[] = {0, 0};
        VkBuffer vertexBuffers[] = {vertexBuffer, instanceBuffer};

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(cmd, 0, 2, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd, 6, liveParticles.size(), 0, 0, 0);
//    vkCmdDraw(cmd, 4, 1, 0, 0);
    }
    if(graphicsPipelineType == GraphicsPipelineType::StarParticles) {
        if (starInstances.empty()) return;

        VkDeviceSize offsets[] = {0, 0};
        VkBuffer vertexBuffers[] = {vertexBuffer, instanceBuffer};

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(cmd, 0, 2, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd, 6, starInstances.size(), 0, 0, 0);
//    vkCmdDraw(cmd, 4, 1, 0, 0);
    }


}

void ParticleSystem::updateExplosionParticles(VkDeviceMemory particlesInstanceBufferMemory) {
    liveParticles.clear();
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        ParticleInstance &p = particles[i];
        if (!p.active) continue;
        p.velocity += p.acceleration * Time::deltaTime;
        p.position += p.velocity * Time::deltaTime;
        p.center = p.position;
        p.life -= Time::deltaTime;
        liveParticles.push_back(p);
        p.color.a = glm::clamp(p.life / p.maxLife, 0.0f, 1.0f); // Fade out
        if (p.life <= 0) p.active = false;
    }

    if (!liveParticles.empty()) {
        void *data;
        VkDeviceSize size = liveParticles.size() * sizeof(ParticleInstance);
        vkMapMemory(device_, particlesInstanceBufferMemory, 0, size, 0, &data);
        memcpy(data, liveParticles.data(), size);
        vkUnmapMemory(device_, particlesInstanceBufferMemory);
    }

}

void ParticleSystem::updateStarField(VkDeviceMemory starInstanceBufferMemory) {
    for (auto& star : starInstances) {
        star.position.y += star.speed * Time::deltaTime;
        if (star.position.y > 1.1f) { // Slightly below bottom, wrap to top
            star.position.y = -1.1f;
            // Optionally randomize X/speed/size/brightness for more variation
            star.position.x = xDist(rng);
            star.speed = speedDist(rng);
            star.size = sizeDist(rng);
            star.brightness = brightDist(rng);
        }
    }
    // Map and upload starInstances to your instance buffer (same as particles)
    void *data;
    VkDeviceSize size = starInstances.size() * sizeof(StarInstance);
    vkMapMemory(device_, starInstanceBufferMemory, 0, size, 0, &data);
    memcpy(data, starInstances.data(), size);
    vkUnmapMemory(device_, starInstanceBufferMemory);

}

ParticleSystem::ParticleSystem(VkDevice device):device_(device) {
    initExplosionParticles();
    initStarField();
}

void ParticleSystem::initExplosionParticles(){
    for (auto &p: particles) {
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

void ParticleSystem::initStarField() {
    starInstances.clear();
    for (int i = 0; i < NUM_STARS; ++i) {
        starInstances.push_back({
            {xDist(rng), yDist(rng), 0.0f},
            speedDist(rng),
            sizeDist(rng),
            brightDist(rng)
        });
    }
}

ParticleSystem::~ParticleSystem() {

}
