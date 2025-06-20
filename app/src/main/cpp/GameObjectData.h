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
#include <random>
#include <cmath>

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

enum class GraphicsPipelineType {
    Main,
    Overlay,
    Font,
    Particles,
    ExplosionParticles,
    StarParticles
};


static const Vertex particleVerts[4] = {
        {{-0.5f,-0.5f, 1.0f},{1.0f,1.0f,0.0f},{0.0f, 0.0f}},
        {{0.5f,-0.5f, 1.0f},{1.0f,1.0f,0.0f},{1.0f, 0.0f}},
        {{0.5f,0.5f,  1.0f},{1.0f,0.5f,0.0f},{1.0f, 1.0f}},
        {{-0.5f,0.5f, 1.0f},{0.0f,1.0f,0.0f},{0.0f, 1.0f}}
};

static const uint16_t particlesIndices[6] = {0, 1, 2, 2, 3, 0};

// Relative to (0, 0); will offset per-bullet in the shader or CPU
static const Vertex bulletVerts[6] = {
        // First triangle
        {{-0.02f, -0.05f, 1.0f}, {1, 1, 0}, {0.0f, 0.0f}},
        {{0.02f,  -0.05f, 1.0f}, {1, 1, 0}, {1.0f, 0.0f}},
        {{-0.02f, 0.00f,  1.0f}, {1, 1, 0}, {0.0f, 1.0f}},
        // Second triangle
        {{0.02f,  -0.05f, 1.0f}, {1, 1, 0}, {1.0f, 0.0f}},
        {{0.02f,  0.00f,  1.0f}, {1, 1, 0}, {1.0f, 1.0f}},
        {{-0.02f, 0.00f,  1.0f}, {1, 1, 0}, {0.0f, 1.0f}}
};

static Vertex triangleVerts[3] = {
        {{0.0f,  -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  0.5f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
};

static OverlayVertex overlayQuadVerts[6] = {
        {{-0.6f, -0.3f,  1.0f}, {0.0f, 0.0f}}, // bottom-left
        {{0.6f,  -0.3f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
        {{-0.6f, -0.05f, 1.0f}, {0.0f, 1.0f}}, // top-left

        {{0.6f,  -0.3f,  1.0f}, {1.0f, 0.0f}}, // bottom-right
        {{0.6f,  -0.05f, 1.0f}, {1.0f, 1.0f}}, // top-right
        {{-0.6f, -0.05f, 1.0f}, {0.0f, 1.0f}} // top-left

};


// Ship is a rectangle at y = -0.8f, width = 0.2, height = 0.05 (adjust as you like)
static Vertex shipVerts[6] = {
        // First triangle
        {{0.1f,  0.9f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // bottom left (white)
        {{-0.1f, 0.9f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // bottom right (cyan)
        {{0.1f,  0.8f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // top left (magenta)
        // Second triangle
        {{-0.1f, 0.9f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // bottom right
        {{-0.1f, 0.8f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // top right (blue)
        {{0.1f,  0.8f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}  // top left
};

static const Vertex alienVerts[6] = {
        // Rectangle centered at (0, 0), width 0.12, height 0.07
        {{-0.07f, -0.045f, 1.0f}, {0.3f, 1.0f, 0.3f}, {0.0f, 0.0f}}, // green
        {{0.07f,  -0.045f, 1.0f}, {0.3f, 1.0f, 0.3f}, {1.0f, 0.0f}},
        {{-0.07f, 0.045f,  1.0f}, {0.6f, 1.0f, 0.6f}, {0.0f, 1.0f}},

        {{0.07f,  -0.045f, 1.0f}, {0.3f, 1.0f, 0.3f}, {1.0f, 0.0f}},
        {{0.07f,  0.045f,  1.0f}, {0.6f, 1.0f, 0.6f}, {1.0f, 1.0f}},
        {{-0.07f, 0.045f,  1.0f}, {0.6f, 1.0f, 0.6f}, {0.0f, 1.0f}},
};

class GameObjectData {


};


#endif //SPACEINVADERS3D_GAMEOBJECTDATA_H
