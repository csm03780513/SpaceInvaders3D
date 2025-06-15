#pragma once

#include "FontManager.h"
#include "ParticleSystem.h"

class Renderer {
public:
    Renderer(android_app* app);
    ~Renderer();
    void drawFrame();
    void updateShipBuffer();
    void spawnBullet();
    float shipX_ = 0.0f;
    float shipY_ = 0.0f;

    GameState gameState;


    void restartGame();

private:
    FontManager *fontManager_;
    ParticleSystem *particleSystem_;
    UniformBufferObject ubo_;
    android_app* app_;
    AAssetManager* assetManager_; // assetmgr
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
    VkDescriptorSetLayout alienDescriptorSetLayout_{VK_NULL_HANDLE};
    VkDescriptorSetLayout shipBulletDescriptorSetLayout_{VK_NULL_HANDLE};
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
    VkDescriptorSet alienDescriptorSet_{VK_NULL_HANDLE};
    VkDescriptorSet shipBulletDescriptorSet_{VK_NULL_HANDLE};

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

    void* uniformBuffersData{nullptr};

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

    VkBuffer particlesVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory particlesVertexBufferMemory_{VK_NULL_HANDLE};

    VkBuffer particlesIndexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory particlesIndexBufferMemory_{VK_NULL_HANDLE};

    VkBuffer particlesInstanceBuffer_;
    VkDeviceMemory particlesInstanceBufferMemory_;


    VkImage fontAtlasImage_;
    VkDeviceMemory fontAtlasImageDeviceMemory_;
    VkImageView fontAtlasImageView_;
    VkSampler fontAtlasSampler_;

    VkPipeline fontPipeline_{VK_NULL_HANDLE};
    VkPipelineLayout fontPipelineLayout_{VK_NULL_HANDLE};
    VkDescriptorSet fontDescriptorSet_{VK_NULL_HANDLE};
    VkDescriptorPool fontDescriptorPool_{VK_NULL_HANDLE};
    VkDescriptorSetLayout fontDescriptorSetLayout_{VK_NULL_HANDLE};


    VkPipeline particlesPipeline_{VK_NULL_HANDLE};
    VkPipelineLayout particlesPipelineLayout_{VK_NULL_HANDLE};
    VkDescriptorSet particlesDescriptorSet_{VK_NULL_HANDLE};
    VkDescriptorPool particlesDescriptorPool_{VK_NULL_HANDLE};
    VkDescriptorSetLayout particlesDescriptorSetLayout_{VK_NULL_HANDLE};

    void recordCommandBuffer(uint32_t imageIndex, uint32_t particleCount);
    void initVulkan();
    void updateBullet(float deltaTime);
    void updateAliens(float deltaTime);
    void updateUniformBuffer(float deltaTime);

    void updateCollision();

    void updateGameState();

    void createPipeline(GraphicsPipelineData &graphicsPipelineData, GraphicsPipelineType graphicsPipelineType);

    void createPipelineLayout(VkPipelineLayoutCreateInfo &pipelineLayoutInfo,
                              GraphicsPipelineData &graphicsPipelineData);

    void createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo info, VkDescriptorSetLayout &layout);

    void createMainGraphicsPipeline();
    void initAliens();


    void createUniformBuffer();

    void createImageOverlayDescriptor(GraphicsPipelineData &graphicsPipelineData);

    void loadTexture(const char *filename, VkImage &vkImage, VkDeviceMemory &vkDeviceMemory, VkImageView &imageView, VkSampler &vkSampler,GameTextureType gameTextureType);

    void createOverlayGraphicsPipeline();

    void loadAllTextures();

    void createMainDescriptor(GraphicsPipelineData &graphicsPipelineData);

    void createFontGraphicsPipeline();

    void createFontDescriptor(GraphicsPipelineData &graphicsPipelineData);

    void loadText();
    void loadGameObjects();

    void animateScore(float deltaTime);

    void createInstance();

    void createSurface();

    void getPhysicalDevice();

    void updateParticleInstances(const std::vector<ParticleInstance> &particles);

    void createParticlesGraphicsPipeline();

    void createParticleDescriptor(GraphicsPipelineData &graphicsPipelineData);

    void createDescriptorSetLayout();
};
