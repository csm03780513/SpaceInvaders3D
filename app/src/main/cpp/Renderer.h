#pragma once

#include "FontManager.h"
#include "ParticleSystem.h"
#include <memory>
#include <string>
#include "Time.h"
#include "PowerUpManager.h"
#include "Util.h"

static constexpr int NUM_ALIENS_X = 8;
static constexpr int NUM_ALIENS_Y = 3;
static constexpr int MAX_ALIENS = NUM_ALIENS_X * NUM_ALIENS_Y;
constexpr int SFX_SAMPLE_RATE = 44100;
constexpr int SFX_CHANNELS = 1;

static constexpr int MAX_BULLETS = 50;


class Renderer {
public:
    explicit Renderer(android_app *app);

    ~Renderer();

    void drawFrame();

    void updateShipBuffer() const;

    void spawnBullet(BulletType bulletType,glm::vec2 spawnPos);

    float shipX_ = 0.0f;
    float shipY_ = 0.0f;
    float rateOfFire = 0.2f;
    float lastFireTime = 0.0f;
    bool canFire = false;

    MainPushConstants shipPC_ = {.texturePos=0};
    MainPushConstants bulletPC_[MAX_BULLETS] = {};
    MainPushConstants alienPC_[MAX_ALIENS] = {};
    // In your renderer, have a shake timer and amplitude:
    float shakeTimer = 0.0f;   // seconds remaining
    float shakeMagnitude = 0.025f; // NDC units (tune as desired)
    glm::vec2 shakeOffset{0.0f};

    GameState gameState;


    void restartGame();

    void stopAudioPlayer();

    void resumeAudioPlayer();

private:
    std::unique_ptr<FontManager> fontManager_;
    std::unique_ptr<ParticleSystem> particleSystem_;
    std::shared_ptr<PowerUpManager> powerUpManager_;
    std::shared_ptr<Util> util_;
    UniformBufferObject ubo_;
    android_app *app_;
    AAssetManager *assetManager_; // assetmgr
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

    void updateAliens();

    void updateUniformBuffer();

    void updateCollision();

    void updateGameState();

    void createPipeline(GfxPipelineData &gfxPipelineData,GfxPipelineType gfxPipelineType);

    void createPipelineLayout(VkPipelineLayoutCreateInfo &pipelineLayoutInfo,GfxPipelineData &gfxPipelineData);

    void createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo info, VkDescriptorSetLayout &layout);

    void createMainGfxPipeline();

    void initAliens();


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
};
