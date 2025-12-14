//
// Created by carlo on 01/07/2025.
//

#ifndef SPACEINVADERS3D_POWERUPMANAGER_H
#define SPACEINVADERS3D_POWERUPMANAGER_H

#include "GameObjectData.h"
#include "GameTime.h"
#include "Util.h"
#include "Collision.h"
#include "SFXMixer.h"
#include "ecs/components/CombatComponents.h"

struct PowerUpData {
    PowerUpType type;
    glm::vec3 pos;      // NDC or world units
    std::array<float,2> widthHeight = Util::getQuadWidthHeight(quadVerts,6,{1,1});
    float fallSpeed;    // e.g., 0.5f per sec
    float timeLeft;     // for active power-ups, e.g. 5.0f
    bool active;
};

struct PowerUpIndicator {
    glm::vec2 textPos;
    bool active;
    uint32_t expiryTime;
    uint32_t offset;
};

class PowerUpManager {
private:
    PowerUpManager();
    void updatePowerUpExpiry();
    void activatePowerUp(PowerUpType type);
    std::vector<PowerUpData> powerUps_;

    VkDevice device_;
    const std::shared_ptr<Util> util_;
    const std::shared_ptr<SFXMixer> sfxMixer_;
    float elapsedTime_ = 0.0f;
public:
    std::unordered_map<GameText,PowerUpIndicator> collectedPowerUps;
    bool doubleShotActive = false;
    float doubleShotTimer = 0.0f;
    float powerUpChance = 0.1f;
    bool shieldActive = false;
    float shieldTimer = 0.0f;
    VkBuffer powerUpBuffer;
    VkDeviceMemory powerUpBufferMemory;
    explicit PowerUpManager(VkDevice device, const std::shared_ptr<Util> &util, const std::shared_ptr<SFXMixer> &sfxMixer);
    ~PowerUpManager();
    void update(float deltaTime);
    void spawnPowerUp(const glm::vec2& pos);
    void updatePowerUpData();
    void checkIfPowerUpCollected(const Ship &ship);
    void recordCommandBuffer(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,VkPipeline pipeline,glm::vec2 shakeOffset,VkDescriptorSet descriptorSet);
};


#endif //SPACEINVADERS3D_POWERUPMANAGER_H
