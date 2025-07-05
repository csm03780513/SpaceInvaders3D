//
// Created by carlo on 01/07/2025.
//

#ifndef SPACEINVADERS3D_POWERUPMANAGER_H
#define SPACEINVADERS3D_POWERUPMANAGER_H

#include "GameObjectData.h"
#include "Time.h"
#include "Util.h"

struct PowerUpData {
    PowerUpType type;
    glm::vec3 pos;      // NDC or world units
    std::array<float,2> widthHeight = Util::getQuadWidthHeight(quadVerts,6);
    float fallSpeed;    // e.g., 0.5f per sec
    float timeLeft;     // for active power-ups, e.g. 5.0f
    bool active;
};

class PowerUpManager {
private:

    void updatePowerUpExpiry();
    void activatePowerUp(PowerUpType type);
    std::vector<PowerUpData> powerUps_;
public:
    std::shared_ptr<Util> util;
    VkDevice device;
    bool doubleShotActive = false;
    float doubleShotTimer = 0.0f;
    bool shieldActive = false;
    float shieldTimer = 0.0f;
    VkBuffer powerUpBuffer;
    VkDeviceMemory powerUpBufferMemory;
    explicit PowerUpManager();
    void spawnPowerUp(PowerUpType type, const glm::vec2& pos);
    void updatePowerUpData();
    void checkIfPowerUpCollected(Ship ship);
    void recordCommandBuffer(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,VkPipeline pipeline,glm::vec2 shakeOffset,VkDescriptorSet descriptorSet);
};


#endif //SPACEINVADERS3D_POWERUPMANAGER_H
