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


class Renderer {
public:
    Renderer(android_app* app);
    ~Renderer();
    void drawFrame();
    void updateShipBuffer();
    void spawnBullet();
    float shipX_ = 0.0f;


private:
    android_app* app_;
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
    VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
    VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
    VkPipeline graphicsPipeline_{VK_NULL_HANDLE};
    VkBuffer uniformBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory uniformBufferMemory_{VK_NULL_HANDLE};
    VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
    VkDescriptorSet descriptorSet_{VK_NULL_HANDLE};
    VkSemaphore imageAvailableSemaphore_{VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphore_{VK_NULL_HANDLE};

    VkBuffer shipVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory shipVertexBufferMemory_{VK_NULL_HANDLE};
    // Centered horizontally

    VkBuffer bulletVertexBuffer_{VK_NULL_HANDLE};
    VkDeviceMemory bulletVertexBufferMemory_{VK_NULL_HANDLE};


    void recordCommandBuffer(uint32_t imageIndex);

    void updateBullet();

    void initVulkan();
};
