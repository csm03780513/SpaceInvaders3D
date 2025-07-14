//
// Created by carlo on 01/07/2025.
//

#include "PowerUpManager.h"
AABB powerupBox;
AABB shipBox;
AABB getAABB(float cx, float cy, float width, float height) {
    float halfW = width * 0.5f;
    float halfH = height * 0.5f;
    return { cx - halfW, cy - halfH, cx + halfW, cy + halfH };
}

bool isColliding(const AABB& a, const AABB& b) {
    return (a.minX < b.maxX && a.maxX > b.minX &&
            a.minY < b.maxY && a.maxY > b.minY);
}

void PowerUpManager::spawnPowerUp(PowerUpType type, const glm::vec2 &pos) {
    float randomChance = rand() / float(RAND_MAX);
    if (randomChance < 0.1f) { // 10% chance
        PowerUpData p{};
        p.type = (rand() % 2 == 0) ? PowerUpType::DoubleShot : PowerUpType::Shield;
        p.pos = glm::vec3(pos.x, pos.y,0.0f); // Spawn at alien’s last position
        p.fallSpeed = 0.3f + 0.2f * (rand() / float(RAND_MAX)); // vary slightly
        p.active = true;
        powerUps_.push_back(p);
    }
}

void PowerUpManager::updatePowerUpData() {
    updatePowerUpExpiry();
    for (auto& p : powerUps_) {
        if (!p.active) continue;
        p.pos.y -= p.fallSpeed * Time::deltaTime; // Move downwards
        // Deactivate if off-screen
        if (p.pos.y < -1.1f){
            p.active = false;
        }

    }

}

PowerUpManager::PowerUpManager(){

}
float elapsedTime =0.0f;
void PowerUpManager::recordCommandBuffer(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                                         VkPipeline pipeline,
                                         glm::vec2 shakeOffset,
                                         VkDescriptorSet descriptorSet) {

    for (auto powerUp:powerUps_) {
        elapsedTime += Time::deltaTime;
        MainPushConstants pushConstants = {};
        pushConstants.pos = {powerUp.pos.x, -powerUp.pos.y};
        pushConstants.shakeOffset = shakeOffset;
        pushConstants.time = elapsedTime;
        pushConstants.canPulse = 1;
        if(powerUp.type == PowerUpType::DoubleShot) pushConstants.texturePos = 3;
        if(powerUp.type == PowerUpType::Shield) pushConstants.texturePos = 4;


        VkDeviceSize offsets[] = {0};
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &descriptorSet, 0, nullptr);
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MainPushConstants),&pushConstants);

        vkCmdBindVertexBuffers(cmd, 0, 1, &powerUpBuffer, offsets);
        vkCmdDraw(cmd, 6, 1, 0, 0);
//        util->recordDrawBoundingBox(cmd, powerupBox, {0.0f, 1.0f, 0.0f});
    }


//    util->recordDrawBoundingBox(cmd, shipBox, {1.0f, 0.0f, 0.0f});




}

void PowerUpManager::activatePowerUp(PowerUpType type) {
    switch(type) {
        case PowerUpType::DoubleShot:
            doubleShotActive = true;
            doubleShotTimer = 6.0f; // lasts 6 seconds
            LOGE("Activated DoubleShot");
            break;
        case PowerUpType::Shield:
            shieldActive = true;
            shieldTimer = 8.0f;
            LOGE("Activated Shield");
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
    powerUps_.erase(std::remove_if(powerUps_.begin(), powerUps_.end(),
                                   [](const PowerUpData& p) { return !p.active; }),
                    powerUps_.end());
}

void PowerUpManager::checkIfPowerUpCollected(Ship ship) {
    for (auto& powerup : powerUps_) {
        if (!powerup.active) continue;

         powerupBox = getAABB(powerup.pos.x, -powerup.pos.y, powerup.widthHeight[0], powerup.widthHeight[1]);
         shipBox    = getAABB(ship.x, ship.y, ship.widthHeight[0], ship.widthHeight[1]);

        if (isColliding(powerupBox, shipBox)) {
            powerup.active = false;
            activatePowerUp(powerup.type);
        }
    }
}
