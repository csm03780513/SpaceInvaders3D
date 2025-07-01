//
// Created by carlo on 01/07/2025.
//

#include "PowerUpManager.h"


void PowerUpManager::spawnPowerUp(PowerUpType type, const glm::vec2 &pos) {
    float randomChance = rand() / float(RAND_MAX);
    if (randomChance < 0.1f) { // 10% chance
        PowerUpData p{};
        p.type = (rand() % 2 == 0) ? PowerUpType::DoubleShot : PowerUpType::Shield;
        p.pos = glm::vec2(pos.x, -pos.y); // Spawn at alien’s last position
        p.fallSpeed = 0.3f + 0.2f * (rand() / float(RAND_MAX)); // vary slightly
        p.active = true;
        powerUps_.push_back(p);
    }
}

void PowerUpManager::updatePowerUpData() {
    updatePowerUpExpiry();
    for (auto& p : powerUps_) {
        if (!p.active) continue;
        p.pos.y += p.fallSpeed * Time::deltaTime; // Move downwards
        // Deactivate if off-screen
        if (p.pos.y < -1.1f) p.active = false;
    }

}

PowerUpManager::PowerUpManager(){

}

void PowerUpManager::recordCommandBuffer(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                                         VkPipeline pipeline,
                                         glm::vec2 shakeOffset) {

    for (auto powerUp:powerUps_) {
        MainPushConstants pushConstants = {};
        pushConstants.pos = {powerUp.pos.x, -powerUp.pos.y};
        pushConstants.shakeOffset = shakeOffset;

        VkDeviceSize offsets[] = {0};
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &doubleShotDescriptorSet, 0, nullptr);
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MainPushConstants),&pushConstants);

        vkCmdBindVertexBuffers(cmd, 0, 1, &powerUpBuffer, offsets);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }



}

void PowerUpManager::activatePowerUp(PowerUpType type) {
    switch(type) {
        case PowerUpType::DoubleShot:
            doubleShotActive = true;
            doubleShotTimer = 6.0f; // lasts 6 seconds
            break;
        case PowerUpType::Shield:
            shieldActive = true;
            shieldTimer = 8.0f;
            break;
        case PowerUpType::Life:
            break;
    }
}

void PowerUpManager::updatePowerUpExpiry() {
    // In your update:
    if (doubleShotActive) {
        doubleShotTimer -= Time::deltaTime;
        if (doubleShotTimer <= 0.0f) doubleShotActive = false;
    }
    if (shieldActive) {
        shieldTimer -= Time::deltaTime;
        if (shieldTimer <= 0.0f) shieldActive = false;
    }

}

void PowerUpManager::checkIfPowerUpCollected(Ship ship) {
    for (auto& p : powerUps_) {
        if (!p.active) continue;
        // Rectangle bounds for ship
        float shipHalfW = ship.size * 0.5f;   // or whatever your ship uses
        float shipHalfH = ship.size * 0.5f;   // use separate var if ship is not square

// Rectangle bounds for powerup (from your quadVerts)
        float puHalfW = 0.07f;
        float puHalfH = 0.045f;

        if (fabs(ship.x - p.pos.x) < (shipHalfW + puHalfW) &&
            fabs(ship.y - p.pos.y) < (shipHalfH + puHalfH)) {

            p.active = false;
            LOGE("PowerUp collected!");
            activatePowerUp(p.type);
        }
    }
}
