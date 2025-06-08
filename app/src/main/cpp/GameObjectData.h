//
// Created by carlo on 07/06/2025.
//

#ifndef SPACEINVADERS3D_GAMEOBJECTDATA_H
#define SPACEINVADERS3D_GAMEOBJECTDATA_H

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
#include <android/log.h>
#include <chrono>
#include <unordered_map>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Vulkan", __VA_ARGS__)


struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
struct Vertex {
    float pos[3];
    float color[3];
    float uv[2];
};

struct OverlayVertex {
    float pos[3];
    float uv[2];
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

struct GraphicsPipelineData {
    VkPipeline pipeline{VK_NULL_HANDLE};
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineMultisampleStateCreateInfo multisamplingState;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    uint32_t subpass;
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
};

enum class GameState {
    Playing,
    Won,
    Lost
};

enum class GameText {
    Score,
    Title
};

enum class GameTextureType {
    Ship,
    Alien,
    ShipBullet,
    FontAtlas,
    Overlay
};

class GameObjectData {


};


#endif //SPACEINVADERS3D_GAMEOBJECTDATA_H
