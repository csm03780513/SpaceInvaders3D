#pragma once

#include "FontManager.h"
#include "ParticleSystem.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include "GameTime.h"
#include "PowerUpManager.h"
#include "Util.h"
#include "ecs/events/CombatEvents.h"
#include "ecs/systems/AilmentSystem.h"
#include "events/EventBus.h"
#include "mechanics/CombatEventSubscribers.h"
#include "mechanics/AlienManager.h"
#include "platform/PlatformServices.h"
#include "GameObjectData.h"
constexpr int SFX_SAMPLE_RATE = 44100;
constexpr int SFX_CHANNELS = 1;

static constexpr int MAX_BULLETS = 50;


struct PipelineHandles;
class PipelineBuilder;

class Renderer {
public:
    explicit Renderer(IPlatformServices &platformServices);

    ~Renderer();

    void drawFrame();

    void updatePlayingLogic(float dt);
    void prepareFrame(bool isPlaying);

    void updateShip() const;

    void spawnBullet(BulletType bulletType,glm::vec2 spawnPos);

    void setShipPosition(float x, float y, bool fireBullet);

    void restartGame();

    void stopAudioPlayer();

    void resumeAudioPlayer();

    void setGameState(GameState state);
    [[nodiscard]] GameState getGameState() const;
    [[nodiscard]] const std::vector<UiEntry> &getUiEntries(TextureSections section) const;

    void onWindowLost();
    void onWindowResumed();

    [[nodiscard]] bool hasActiveAliens() const;
    [[nodiscard]] bool hasAlienBelow(float threshold) const;
    [[nodiscard]] bool hasShipDead() const;

    float shipX_ = 0.0f;
    float shipY_ = 0.0f;
    float rateOfFire = 0.2f;
    float lastFireTime = 0.0f;
    bool canFire = false;

    MainPushConstants shipPC_ = {.texturePos=0};
    MainPushConstants bulletPC_[MAX_BULLETS] = {};

    std::unordered_map<TextureSections, std::vector<UiEntry>> uiTextures = {
            {TextureSections::MainMenu, {
                UiEntry{{0.0f, -0.6f},{15.0f, 6.0f},0,"title"},
                UiEntry{{0.0f, 0.0f},{9.0f, 1.5f},1,"start"},
                UiEntry{{0.0f, 0.2f},{9.0f, 1.5f},2,"exit"}
            }},
            {TextureSections::Lost,{
                UiEntry{{0.0f, 0.0f},{12.0f,6.0f},3,"you_died"}
            }},
            {TextureSections::Playing,{
                UiEntry{{-0.6f, -0.75f}, {5.0f, 0.2f}, 4, "player_hull"}
            }}
    };
    // In your renderer, have a shake timer and amplitude:
    float shakeTimer = 0.0f;   // seconds remaining
    float shakeMagnitude = 0.025f; // NDC units (tune as desired)
    glm::vec2 shakeOffset{0.0f};

private:
    friend class PipelineBuilder;
    GameState gameState;

    std::unique_ptr<FontManager> fontManager_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::shared_ptr<PowerUpManager> powerUpManager_;
    std::shared_ptr<SFXMixer> sfxMixer_;
    std::vector<std::string> explosionClipIds_;
    std::shared_ptr<Util> util_;
    UniformBufferObject ubo_;
    IPlatformServices &platformServices_;
    VkInstance instance_{VK_NULL_HANDLE};
    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    uint32_t graphicsQueueFamily_ = UINT32_MAX;
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    VkFormat swapchainFormat_;
    VkExtent2D swapchainExtent_;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkRenderPass renderPass_{VK_NULL_HANDLE};
    std::vector<VkFramebuffer> framebuffers_;
    VkCommandPool commandPool_{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> commandBuffers_;
    VkBuffer vertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory vertexBufferMemory_{VK_NULL_HANDLE};

    VkDescriptorSetLayout shipDescriptorSetLayout_{VK_NULL_HANDLE};
    VkDescriptorSetLayout overlayDescriptorSetLayout_{VK_NULL_HANDLE};

    VkPipelineLayout mainPipelineLayout_{VK_NULL_HANDLE};
    VkPipeline mainPipeline_{VK_NULL_HANDLE};

    VkPipelineLayout overlayPipelineLayout_{VK_NULL_HANDLE};
    VkPipeline overlayPipeline_{VK_NULL_HANDLE};


    VkDescriptorPool overlayDescriptorPool_ = {VK_NULL_HANDLE};
    VkDescriptorSet overlayDescriptorSet_{VK_NULL_HANDLE};

    VkBuffer uniformBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory uniformBufferMemory_{VK_NULL_HANDLE};

    VkDescriptorPool mainDescriptorPool_{VK_NULL_HANDLE};

    VkDescriptorSet shipDescriptorSet_{VK_NULL_HANDLE};

    VkSemaphore imageAvailableSemaphore_{VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphore_{VK_NULL_HANDLE};

    VkBuffer shipVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory shipVertexBufferMemory_{VK_NULL_HANDLE};

    VkBuffer overlayVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory overlayVertexBufferMemory_{VK_NULL_HANDLE};

    VkBuffer bulletVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory bulletVertexBufferMemory_{VK_NULL_HANDLE};

    VkBuffer alienVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory alienVertexBufferMemory_{VK_NULL_HANDLE};

    void *uniformBuffersData{nullptr};

    VkImage overlayImage_{VK_NULL_HANDLE};
    VkDeviceMemory overlayImageDeviceMemory_{VK_NULL_HANDLE};

    VkImageView overlayImageView_{VK_NULL_HANDLE};
    VkSampler overlaySampler_{VK_NULL_HANDLE};

    VkImage shipImage_{VK_NULL_HANDLE};
    VkDeviceMemory shipImageDeviceMemory_{VK_NULL_HANDLE};

    VkImageView shipImageView_{VK_NULL_HANDLE};
    VkSampler shipSampler_{VK_NULL_HANDLE};


    VkImage alienImage_{VK_NULL_HANDLE};
    VkDeviceMemory alienImageDeviceMemory_{VK_NULL_HANDLE};

    VkImageView alienImageView_{VK_NULL_HANDLE};
    VkSampler alienSampler_{VK_NULL_HANDLE};

    VkImage shipBulletImage_{VK_NULL_HANDLE};
    VkDeviceMemory shipBulletImageDeviceMemory_{VK_NULL_HANDLE};

    VkImageView shipBulletImageView_{VK_NULL_HANDLE};
    VkSampler shipBulletSampler_{VK_NULL_HANDLE};

    VkBuffer titleTextVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory titleTextVertexBufferMemory_{VK_NULL_HANDLE};

    VkBuffer scoreTextVertexBuffer_;
    VkDeviceMemory scoreTextVertexBufferMemory_;

    VkBuffer fontVertexBuffer_;
    VkDeviceMemory fontBufferMemory_;
    VkDeviceSize fontBufferSize_ = 0;
    VkDeviceSize scoreOffset_ = 0;
    VkDeviceSize powerUpTextCDOffset = 0;
    VkDeviceSize powerUpTextStartOffset_ = 0;
    VkDeviceSize floatingDamageBufferCursor_ = 0;
    struct PendingFloatingDamageUpload {
        size_t instanceIndex = 0;
        std::vector<Vertex> vertices;
    };
    struct FloatingDamageInstance {
        std::string text;
        VkDeviceSize vertexOffset = 0;
        uint32_t vertexCount = 0;
        glm::vec2 basePos{};
        float startTime = 0.0f;
        float lifetime = 0.0f;
        float riseSpeed = 0.0f;
        float startScale = 0.0f;
        float endScale = 0.0f;
        float fadeStart = 0.7f;
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    };
    std::vector<FloatingDamageInstance> floatingDamageInstances_;
    std::vector<PendingFloatingDamageUpload> pendingFloatingDamageUploads_;
    float floatingDamageGlobalTime_ = 0.0f;
    float floatingDamageLifetime_ = 0.5f;
    float floatingDamageRiseSpeed_ = 0.05f;
    float floatingDamageStartScale_ = 0.0025f;
    float floatingDamageEndScale_ = 0.0003f;

    // Apply DOTs
    AilmentSystem ailSys_;
    AilmentRules ailRules_;
    ShieldRules  shieldRules_{ .kineticHalfOnShield = true, .darkMatterIgnoresArmor = true };


    // Score tracking and animation
    int actualScore = 0;            // Game logic value
    float displayedScore_ = 0.0f;   // Smoothed UI value
    float scoreAnimSpeed_ = 400.0f; // Units per second (tune for effect)
    std::string scoreText_;         // Current display string, e.g. "Score: 1234"

    float scoreScale_ = 0.002f;        // Current scale for the pop effect
    float scoreScaleTarget_ = 0.002f;  // Where we're scaling toward
    float scoreScaleSpeed_ = 6.0f;     // How quickly scale returns to normal
    float scorePopAmount_ = 0.0022f;   // How much to “pop” the score on change

    VkBuffer particlesVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory particlesVertexBufferMemory_{VK_NULL_HANDLE};

    VkBuffer particlesIndexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory particlesIndexBufferMemory_{VK_NULL_HANDLE};

    VkBuffer particlesInstanceBuffer_;
    VkDeviceMemory particlesInstanceBufferMemory_;

    VkBuffer starVertsBuffer_;
    VkDeviceMemory starVertsMemory_;

    VkBuffer starIndexBuffer_;
    VkDeviceMemory starIndexMemory_;

    VkBuffer starInstanceBuffer_;
    VkDeviceMemory starInstanceBufferMemory_;


    VkImage fontAtlasImage_;
    VkDeviceMemory fontAtlasImageDeviceMemory_;
    VkImageView fontAtlasImageView_;
    VkSampler fontAtlasSampler_;

    VkImage doubleShotImage_;
    VkDeviceMemory doubleShotMemory_;
    VkImageView doubleShotView_;
    VkSampler doubleShotSampler_;

    VkImage shieldImage_;
    VkDeviceMemory shieldMemory_;
    VkImageView shieldView_;
    VkSampler shieldSampler_;

    VkImage startImage_;
    VkDeviceMemory startMemory_;
    VkImageView startView_;
    VkSampler startSampler_;

    VkImage titleImage_;
    VkDeviceMemory titleMemory_;
    VkImageView titleView_;
    VkSampler titleSampler_;

    VkImage exitBtnImage_;
    VkDeviceMemory exitBtnMemory_;
    VkImageView exitBtnView_;
    VkSampler exitBtnSampler_;

    VkImage shipHpImage_;
    VkDeviceMemory shipHpMemory_;
    VkImageView shipHpView_;
    VkSampler shipHpSampler_;

    VkPipeline fontPipeline_{VK_NULL_HANDLE};
    VkPipelineLayout fontPipelineLayout_{VK_NULL_HANDLE};
    VkDescriptorSet fontDescriptorSet_{VK_NULL_HANDLE};
    VkDescriptorPool fontDescriptorPool_{VK_NULL_HANDLE};
    VkDescriptorSetLayout fontDescriptorSetLayout_{VK_NULL_HANDLE};


    VkPipeline explosionParticlesPipeline_{VK_NULL_HANDLE};
    VkPipelineLayout particlesPipelineLayout_{VK_NULL_HANDLE};
    VkDescriptorSet particlesDescriptorSet_{VK_NULL_HANDLE};
    VkDescriptorPool particlesDescriptorPool_{VK_NULL_HANDLE};
    VkDescriptorSetLayout particlesDescriptorSetLayout_{VK_NULL_HANDLE};

    VkPipeline starParticlesPipeline_{VK_NULL_HANDLE};

    void recordCommandBuffer(uint32_t imageIndex);

    void initVulkan();

    static void updateBullet();

    void updateUniformBuffer();

    void updateCollision();

    PipelineHandles createPipeline(GfxPipelineData &gfxPipelineData);

    void createPipelineLayout(VkPipelineLayoutCreateInfo &pipelineLayoutInfo,GfxPipelineData &gfxPipelineData);

    void createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo info, VkDescriptorSetLayout &layout);

    void createMainGfxPipeline();

    void createUniformBuffer();

    void createImageOverlayDescriptor(GfxPipelineData &gfxPipelineData);

    void loadTexture(const char *filename, VkImage &vkImage, VkDeviceMemory &vkDeviceMemory,
                     VkImageView &imageView, VkSampler &vkSampler, GameTextureType gameTextureType);

    void createOverlayGfxPipeline();

    void loadAllTextures();

    void createMainDescriptor(GfxPipelineData &gfxPipelineData);

    void createFontGfxPipeline();

    void createFontDescriptor(GfxPipelineData &gfxPipelineData);

    void loadText();

    void loadGameObjects();

    void animateScore();

    void createInstance();

    void createSurface();

    void getPhysicalDevice();

    void createParticlesGfxPipeline(GfxPipelineType gfxPipelineType);


    void createGfxPipeline(GfxPipelineType gfxPipelineType);

    void createAndUploadBuffer(const void *vertices, VkBuffer &buffer, VkDeviceMemory &bufferMemory,
                               VkDeviceSize size,VkBufferUsageFlags usage);

    void alienFireBullet();

    void spawnDamageText(const DamagePopupSpawned &damagePopupSpawned);
    void updateFloatingDamage();
    void drawFloatingDamageTexts(VkCommandBuffer cmd);
    void recordUiSection(VkCommandBuffer cmd, TextureSections section);
    void initShip();

    void createCommandPool();
    bool createSwapchainResources();
    void destroySwapchainResources(bool destroySurface);
    void recreateSwapchain();
    void createGraphicsPipelines();
    void destroyGraphicsPipelines();

    EventBus eventBus_;
    std::unique_ptr<GameMechanicsCoordinator> mechanics_;
    uint32_t damagePopupSubscriptionId_ = 0;
    std::unique_ptr<AlienManager> alienManager_;

    bool swapchainValid_ = false;
    bool pendingSwapchainRecreation_ = false;
    bool pipelinesInitialized_ = false;
};
