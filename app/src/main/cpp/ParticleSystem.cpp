//
// Created by carlo on 14/06/2025.
//

#include "ParticleSystem.h"

void ParticleSystem::spawn(const glm::vec3 &pos, int count) {

    for (int i = 0; i < count; ++i) {
        ParticleInstance& p = particles[firstFree++ % MAX_PARTICLES];
        float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
        float speed = 0.15f + ((float)rand() / RAND_MAX) * 0.15f;
        p.position = pos;
        p.velocity = glm::vec3(cos(angle), sin(angle),0.0f) * speed;
        p.life = p.maxLife = 0.5f + ((float)rand() / RAND_MAX) * 0.3f;
        p.color = glm::vec4(1, 1, 0, 1); // yellowish, can randomize
        p.size = 0.025f + ((float)rand() / RAND_MAX) * 0.025f;
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


void
ParticleSystem::render(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, VkPipeline pipeline, ...) {

}

ParticleSystem::ParticleSystem() {

}

ParticleSystem::~ParticleSystem() {

}

void ParticleSystem::update(float deltaTime) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        ParticleInstance& p = particles[i];
        if (!p.active) continue;
        p.position += p.velocity * deltaTime;
        p.life -= deltaTime;
        p.color.a = glm::clamp(p.life / p.maxLife, 0.0f, 1.0f); // Fade out
        if (p.life <= 0) p.active = false;
    }
}
