#pragma once

#define VK_USE_PLATFORM_ANDROID_KHR
#include <android_native_app_glue.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
//#include <volk.h>
#include <vector>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

#include <chrono>
#include <unordered_map>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
struct Vertex {
    float pos[3];
    float color[3];
};
struct Bullet {
    float x, y;
    bool active;
};
struct Ship {
    float x, y;
    float color[3];
};
struct Alien {
    float x, y;
    bool active;
};
enum class GameState {
    Playing,
    Won,
    Lost
};



class Renderer {
public:
    Renderer(android_app* app);
    ~Renderer();
    void drawFrame();
    void updateShipBuffer();
    void spawnBullet();
    float shipX_ = 0.0f;
    GameState gameState;


    void restartGame();

private:
    UniformBufferObject ubo_;
    android_app* app_;
    AAssetManager* mgr_; // assetmgr
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

    VkDescriptorSetLayout mainDescriptorSetLayout_{VK_NULL_HANDLE};

    VkPipelineLayout mainPipelineLayout_{VK_NULL_HANDLE};
    VkPipeline mainPipeline_{VK_NULL_HANDLE};

    VkPipelineLayout overlayPipelineLayout_{VK_NULL_HANDLE};
    VkPipeline overlayPipeline_{VK_NULL_HANDLE};

    VkBuffer uniformBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory uniformBufferMemory_{VK_NULL_HANDLE};
    VkDescriptorPool mainDescriptorPool_{VK_NULL_HANDLE};
    VkDescriptorSet mainDescriptorSet_{VK_NULL_HANDLE};
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

    void* uniformBuffersData;


    void recordCommandBuffer(uint32_t imageIndex);

    void updateBullet();

    void initVulkan();

    void updateAliens();

    void updateCollision();

    void updateGameState();

    void createPipeline(VkGraphicsPipelineCreateInfo &graphicsPipelineCreateInfo, VkPipeline &pipeline);

    void createPipelineLayout(VkPipelineLayoutCreateInfo &pipelineLayoutInfo, VkPipelineLayout &pipelineLayout);

    void createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo info, VkDescriptorSetLayout &layout);

    void createOverlayGraphicsPipeline();
    void createMainGraphicsPipeline();
    void initAliens();

    void updateUniformBuffer();
    void createUniformBuffer();

};
