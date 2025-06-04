#include "Renderer.h"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>
#include <android/native_window.h>
#include <android/log.h>
#include <vector>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Vulkan", __VA_ARGS__)


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

OverlayVertex overlayQuadVerts[6] = {
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


static constexpr int MAX_BULLETS = 32;
Bullet bullets_[MAX_BULLETS] = {};
Ship ship_ = {};


static constexpr int NUM_ALIENS_X = 8;
static constexpr int NUM_ALIENS_Y = 3;
static constexpr int MAX_ALIENS = NUM_ALIENS_X * NUM_ALIENS_Y;
Alien aliens_[MAX_ALIENS] = {};

float alienMoveSpeed_ = 0.5f;
float bulletMoveSpeed_ = 1.0f;
int alienDirection_ = 1; // 1 = right, -1 = left

const float ALIEN_SIZE = 0.1f;   // world units, adjust as needed
const float BULLET_SIZE = 0.05f; // world units, adjust as needed

using Clock = std::chrono::high_resolution_clock;
auto lastFrameTime = Clock::now();

std::vector<char> loadAsset(AAssetManager *mgr, const char *filename);

VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory);


bool isCollision(const Alien &alien, const Bullet &bullet);

void createImageView(VkDevice device, VkImage image, VkFormat format, VkImageView &imageView);

void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height,
                 VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory);

void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkImage image, VkFormat format, VkImageLayout oldLayout,
                           VkImageLayout newLayout);

void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                       VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

void createTextureSampler(VkDevice device, VkSampler &sampler);

void
setShaderStages(VkDevice device, AAssetManager *assetManager, const char *spirvVertexFilename,
                const char *spirvFragmentFilename,
                GraphicsPipelineData &graphicsPipelineData);

void setColorBlending(GraphicsPipelineData &graphicsPipelineData);

void setViewPortState(GraphicsPipelineData &graphicsPipelineData);

void setInputAssembly(GraphicsPipelineData &graphicsPipelineData);

void setRasterizer(GraphicsPipelineData &graphicsPipelineData);

void setSampling(GraphicsPipelineData &graphicsPipelineData);

VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        LOGE("Failed to create shader module");
        abort();
    }
    return shaderModule;
}

inline bool isCollision(const Alien &alien, const Bullet &bullet) {
    float halfAlien = ALIEN_SIZE * 0.5f;
    float halfBullet = BULLET_SIZE * 0.5f;

    return std::abs(alien.x - bullet.x) < (halfAlien + halfBullet) &&
           std::abs(alien.y - bullet.y) < (halfAlien + halfBullet);
}

std::vector<char> loadAsset(AAssetManager *mgr, const char *filename) {
    AAsset *asset = AAssetManager_open(mgr, filename, AASSET_MODE_STREAMING);
    size_t size = AAsset_getLength(asset);
    std::vector<char> buffer(size);
    AAsset_read(asset, buffer.data(), size);
    AAsset_close(asset);
    return buffer;
}

void Renderer::loadTexture(const char *filename, VkImage &vkImage, VkDeviceMemory &vkDeviceMemory,
                           VkImageView &imageView, VkSampler &vkSampler) {

    std::vector<uint8_t> pixelData;
    int textureWidth, textureHeight;
    AAsset *asset = AAssetManager_open(assetManager_, filename, AASSET_MODE_STREAMING);
    bool imageIsLoaded = true;
    if (!asset) imageIsLoaded = false;

    size_t fileLength = AAsset_getLength(asset);
    std::vector<uint8_t> fileData(fileLength);
    AAsset_read(asset, fileData.data(), fileLength);
    AAsset_close(asset);

    int channels;
    unsigned char *decoded = stbi_load_from_memory(fileData.data(), fileLength, &textureWidth,
                                                   &textureHeight,
                                                   &channels, STBI_rgb_alpha);

    if (!decoded) imageIsLoaded = false;

    pixelData.assign(decoded, decoded + textureWidth * textureHeight * 4);

    stbi_image_free(decoded);

    if (imageIsLoaded) {
        LOGE("after loading image asset");
        VkBuffer stagingBuffer{VK_NULL_HANDLE};
        VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
        // 1. Create staging buffer & copy image data
        // (Create staging buffer, copy data, omitted for brevity)
        VkDeviceSize imageSize = textureWidth * textureHeight * 4;
        createBuffer(device_, physicalDevice_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        LOGE("after creating image buffer");
        void *imageData;
        vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &imageData);
        memcpy(imageData, pixelData.data(), imageSize);
        vkUnmapMemory(device_, stagingBufferMemory);
        LOGE("after mapping image data");

// 2. Create the Vulkan image (device local)
        createImage(device_, physicalDevice_, textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkDeviceMemory);

        LOGE("after creating image");
// 3. Copy from staging buffer to Vulkan image (with transitions)
        transitionImageLayout(device_, commandPool_, graphicsQueue_, vkImage,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        LOGE("after doing transition image layout");
        copyBufferToImage(device_, commandPool_, graphicsQueue_, stagingBuffer, vkImage,
                          textureWidth, textureHeight);

        LOGE("after copyBufferToImage");
        transitionImageLayout(device_, commandPool_, graphicsQueue_, vkImage,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        LOGE("last transition image layout");
// 4. Create image view and sampler
        createImageView(device_, vkImage, VK_FORMAT_R8G8B8A8_UNORM, imageView);
        createTextureSampler(device_, vkSampler);

        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkFreeMemory(device_, stagingBufferMemory, nullptr);

    } else {
        LOGE("failed to load overlay image");
    }
}

// Create a Vulkan buffer
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory) {

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        LOGE("Failed to create buffer");
        abort();
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;

    // Find memory type
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        LOGE("Failed to allocate buffer memory");
        abort();
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void createImage(VkDevice device, VkPhysicalDevice physicalDevice,
                 uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                 VkImage &image, VkDeviceMemory &imageMemory) {

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateImage(device, &imageInfo, nullptr, &image);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    // Find proper memory type
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }
    vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    vkBindImageMemory(device, image, imageMemory, 0);
}

void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkImage image, VkFormat format, VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                       VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
    );

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void createImageView(VkDevice device, VkImage image, VkFormat format, VkImageView &imageView) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(device, &viewInfo, nullptr, &imageView);
}

void createTextureSampler(VkDevice device, VkSampler &sampler) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
}

Renderer::Renderer(android_app *app) : app_(app) {
    assetManager_ = app_->activity->assetManager;
    initVulkan();
}

void Renderer::loadAllTextures() {

    loadTexture("ke_ship_1.png", shipImage_, shipImageDeviceMemory_, shipImageView_, shipSampler_);
    loadTexture("alien_ship_1.png", alienImage_, alienImageDeviceMemory_, alienImageView_,
                alienSampler_);
    loadTexture("laser_2.png", shipBulletImage_, shipBulletImageDeviceMemory_, shipBulletImageView_,
                shipBulletSampler_);
    loadTexture("tap_to_restart_2.png", overlayImage_, overlayImageDeviceMemory_, overlayImageView_,
                overlaySampler_);

}

void Renderer::createImageOverlayDescriptor(GraphicsPipelineData &graphicsPipelineData) {

    VkDescriptorSetLayoutBinding overlaySamplerLayoutBinding = {};
    overlaySamplerLayoutBinding.binding = 0;
    overlaySamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    overlaySamplerLayoutBinding.descriptorCount = 1;
    overlaySamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo overlayLayoutInfo = {};
    overlayLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    overlayLayoutInfo.bindingCount = 1;
    overlayLayoutInfo.pBindings = &overlaySamplerLayoutBinding;

    createDescriptorSetLayout(overlayLayoutInfo, overlayDescriptorSetLayout_);

    VkDescriptorPoolSize vkDescriptorPoolSize = {};
    vkDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    vkDescriptorPoolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo overlayDescriptorPoolInfo = {};
    overlayDescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    overlayDescriptorPoolInfo.maxSets = 1;
    overlayDescriptorPoolInfo.poolSizeCount = 1;
    overlayDescriptorPoolInfo.pPoolSizes = &vkDescriptorPoolSize;

    VkResult res1 = vkCreateDescriptorPool(device_, &overlayDescriptorPoolInfo, nullptr,
                                           &overlayDescriptorPool_);
    if (res1 != VK_SUCCESS) {
        LOGE("Failed to create overlay descriptor pool at: %d", res1);
        abort();
    }

    VkPushConstantRange overlayPushConstantRange = {};
    overlayPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    overlayPushConstantRange.offset = 0;
    overlayPushConstantRange.size = sizeof(float) * 4; // For vec4 offset

    VkPipelineLayoutCreateInfo overlayPipelineLayoutInfo = {};
    overlayPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    overlayPipelineLayoutInfo.setLayoutCount = 1;
    overlayPipelineLayoutInfo.pSetLayouts = &overlayDescriptorSetLayout_;
    overlayPipelineLayoutInfo.pushConstantRangeCount = 1;
    overlayPipelineLayoutInfo.pPushConstantRanges = &overlayPushConstantRange;

    createPipelineLayout(overlayPipelineLayoutInfo, graphicsPipelineData);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = overlayDescriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &overlayDescriptorSetLayout_;


    vkAllocateDescriptorSets(device_, &allocInfo, &overlayDescriptorSet_);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = overlayImageView_;
    imageInfo.sampler = overlaySampler_;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = overlayDescriptorSet_;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
}

void Renderer::createMainDescriptor(GraphicsPipelineData &graphicsPipelineData) {

    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();


    //setLayouts for ship and alien
    createDescriptorSetLayout(layoutInfo, shipDescriptorSetLayout_);
    createDescriptorSetLayout(layoutInfo, alienDescriptorSetLayout_);
    createDescriptorSetLayout(layoutInfo, shipBulletDescriptorSetLayout_);

    std::vector<VkDescriptorSetLayout> descriptionSetLayouts = {shipDescriptorSetLayout_,
                                                                alienDescriptorSetLayout_,
                                                                shipBulletDescriptorSetLayout_};

    VkPushConstantRange vkPushConstantRange = {};
    vkPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vkPushConstantRange.offset = 0;
    vkPushConstantRange.size = sizeof(float) * 2; // For vec2 offset

    std::vector<VkPushConstantRange> pushConstantRanges = {vkPushConstantRange};

    VkPipelineLayoutCreateInfo mainPipelineLayoutInfo = {};
    mainPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    mainPipelineLayoutInfo.setLayoutCount = descriptionSetLayouts.size();
    mainPipelineLayoutInfo.pSetLayouts = descriptionSetLayouts.data();
    mainPipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
    mainPipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    createPipelineLayout(mainPipelineLayoutInfo, graphicsPipelineData);
    LOGE("main pipelineLayout gd: %llu", graphicsPipelineData.pipelineLayout);

    std::vector<VkDescriptorSet> descriptorSets{shipDescriptorSet_, alienDescriptorSet_,
                                                shipBulletDescriptorSet_};

    VkDescriptorPoolSize uboPoolSize = {};
    uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboPoolSize.descriptorCount = descriptorSets.size();

    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = descriptorSets.size();

    std::vector<VkDescriptorPoolSize> poolSizes = {uboPoolSize, samplerPoolSize};

    VkDescriptorPoolCreateInfo descPoolInfo = {};
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.poolSizeCount = poolSizes.size();
    descPoolInfo.pPoolSizes = poolSizes.data();
    descPoolInfo.maxSets = descriptorSets.size();

    LOGE("About to create descriptor pool");
    VkResult res = vkCreateDescriptorPool(device_, &descPoolInfo, nullptr, &mainDescriptorPool_);
    LOGE("vkCreateDescriptorPool returned %d", res);
    if (res != VK_SUCCESS) {
        LOGE("Failed to create descriptor pool: %d", res);
        abort();
    }
    LOGE("Descriptor pool created");

    // Allocate sets
    VkDescriptorSetAllocateInfo descAllocInfo = {};
    descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAllocInfo.descriptorPool = mainDescriptorPool_;
    descAllocInfo.descriptorSetCount = descriptorSets.size();
    descAllocInfo.pSetLayouts = descriptionSetLayouts.data();

    LOGE("About to create descriptor sets");
    VkResult res1 = vkAllocateDescriptorSets(device_, &descAllocInfo, descriptorSets.data());
    if (res1 != VK_SUCCESS) {
        LOGE("Failed to create descriptor set: %d", res);
        abort();
    }

    shipDescriptorSet_ = descriptorSets[0];
    alienDescriptorSet_ = descriptorSets[1];
    shipBulletDescriptorSet_ = descriptorSets[2];

    LOGE("Descriptor set created");
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer_;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet shipBulletBufferDescriptorWrite = {};
    shipBulletBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    shipBulletBufferDescriptorWrite.dstSet = shipBulletDescriptorSet_;
    shipBulletBufferDescriptorWrite.dstBinding = 0;
    shipBulletBufferDescriptorWrite.dstArrayElement = 0;
    shipBulletBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    shipBulletBufferDescriptorWrite.descriptorCount = 1;
    shipBulletBufferDescriptorWrite.pBufferInfo = &bufferInfo;

    VkWriteDescriptorSet shipBufferDescriptorWrite = {};
    shipBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    shipBufferDescriptorWrite.dstSet = shipDescriptorSet_;
    shipBufferDescriptorWrite.dstBinding = 0;
    shipBufferDescriptorWrite.dstArrayElement = 0;
    shipBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    shipBufferDescriptorWrite.descriptorCount = 1;
    shipBufferDescriptorWrite.pBufferInfo = &bufferInfo;

    VkWriteDescriptorSet alienBufferDescriptorWrite = {};
    alienBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    alienBufferDescriptorWrite.dstSet = alienDescriptorSet_;
    alienBufferDescriptorWrite.dstBinding = 0;
    alienBufferDescriptorWrite.dstArrayElement = 0;
    alienBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    alienBufferDescriptorWrite.descriptorCount = 1;
    alienBufferDescriptorWrite.pBufferInfo = &bufferInfo;

    VkDescriptorImageInfo shipBulletImageInfo = {};
    shipBulletImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    shipBulletImageInfo.imageView = shipBulletImageView_;
    shipBulletImageInfo.sampler = shipBulletSampler_;

    VkWriteDescriptorSet shipBulletsSamplerDescriptorWrite = {};
    shipBulletsSamplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    shipBulletsSamplerDescriptorWrite.dstSet = shipBulletDescriptorSet_;
    shipBulletsSamplerDescriptorWrite.dstBinding = 1;
    shipBulletsSamplerDescriptorWrite.dstArrayElement = 0;
    shipBulletsSamplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shipBulletsSamplerDescriptorWrite.descriptorCount = 1;
    shipBulletsSamplerDescriptorWrite.pImageInfo = &shipBulletImageInfo;

    VkDescriptorImageInfo shipImageInfo = {};
    shipImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    shipImageInfo.imageView = shipImageView_;
    shipImageInfo.sampler = shipSampler_;

    VkWriteDescriptorSet shipSamplerDescriptorWrite = {};
    shipSamplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    shipSamplerDescriptorWrite.dstSet = shipDescriptorSet_;
    shipSamplerDescriptorWrite.dstBinding = 1;
    shipSamplerDescriptorWrite.dstArrayElement = 0;
    shipSamplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shipSamplerDescriptorWrite.descriptorCount = 1;
    shipSamplerDescriptorWrite.pImageInfo = &shipImageInfo;

    VkDescriptorImageInfo alienImageInfo = {};
    alienImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    alienImageInfo.imageView = alienImageView_;
    alienImageInfo.sampler = alienSampler_;

    VkWriteDescriptorSet alienSamplerDescriptorWrite = {};
    alienSamplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    alienSamplerDescriptorWrite.dstSet = alienDescriptorSet_;
    alienSamplerDescriptorWrite.dstBinding = 1;
    alienSamplerDescriptorWrite.dstArrayElement = 0;
    alienSamplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    alienSamplerDescriptorWrite.descriptorCount = 1;
    alienSamplerDescriptorWrite.pImageInfo = &alienImageInfo;


    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {shipBulletsSamplerDescriptorWrite,
                                                             shipBulletBufferDescriptorWrite,
                                                             shipBufferDescriptorWrite,
                                                             shipSamplerDescriptorWrite,
                                                             alienBufferDescriptorWrite,
                                                             alienSamplerDescriptorWrite};


    vkUpdateDescriptorSets(device_, writeDescriptorSets.size(), writeDescriptorSets.data(), 0,
                           nullptr);
}

void Renderer::initVulkan() {// Load Vulkan functions using volk
//    volkInitialize();

    // Create Vulkan instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "3D Space Invaders";
    appInfo.apiVersion = VK_API_VERSION_1_1;


    // Required extensions
    std::vector<const char *> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&instanceInfo, nullptr, &instance_) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan instance");
        abort();
    }

    // Create Android surface from ANativeWindow
    VkAndroidSurfaceCreateInfoKHR surfInfo = {};
    surfInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfInfo.window = app_->window;

    int w = ANativeWindow_getWidth(app_->window);
    int h = ANativeWindow_getHeight(app_->window);
    LOGE("ANativeWindow = %p, size = %dx%d", app_->window, w, h);

    VkResult surfaceResult = vkCreateAndroidSurfaceKHR(instance_, &surfInfo, nullptr, &surface_);
    if (surfaceResult != VK_SUCCESS) {
        LOGE("Failed to create Android Vulkan surface, error code: %d", surfaceResult);
        abort();
    }

    // 1. Enumerate physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        LOGE("No Vulkan physical devices found");
        abort();
    }
    std::__ndk1::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

// 2. Pick first device that supports graphics and present
    for (auto d: devices) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, nullptr);
        std::__ndk1::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &queueFamilyCount, queueFamilies.data());
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface_, &presentSupport);
            if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
                physicalDevice_ = d;
                graphicsQueueFamily_ = i;
                break;
            }
        }
        if (physicalDevice_ != VK_NULL_HANDLE) break;
    }
    if (physicalDevice_ == VK_NULL_HANDLE) {
        LOGE("No suitable Vulkan physical device/queue family found!");
        abort();
    }
    LOGE("Physical device and graphics queue family selected: %u", graphicsQueueFamily_);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamily_;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char *deviceExtensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan logical device!");
        abort();
    }
    vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
    LOGE("Logical device and graphics queue created");

    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(device_, &semInfo, nullptr, &imageAvailableSemaphore_);
    vkCreateSemaphore(device_, &semInfo, nullptr, &renderFinishedSemaphore_);


    // 1. Get surface capabilities
    VkSurfaceCapabilitiesKHR surfCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &surfCaps);

// 2. Pick a surface format
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    std::__ndk1::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());
    swapchainFormat_ = formats[0].format; // For most devices, first is fine

// 3. Pick present mode (we'll use FIFO, which is always available)
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

// 4. Choose swap extent
    swapchainExtent_ = surfCaps.currentExtent;

// 5. Decide swapchain image count (minimum + 1, but within allowed range)
    uint32_t imageCount = surfCaps.minImageCount + 1;
    if (surfCaps.maxImageCount > 0 && imageCount > surfCaps.maxImageCount)
        imageCount = surfCaps.maxImageCount;

// 6. Create swapchain info struct
    VkSwapchainCreateInfoKHR swapInfo = {};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface = surface_;
    swapInfo.minImageCount = imageCount;
    swapInfo.imageFormat = swapchainFormat_;
    swapInfo.imageColorSpace = formats[0].colorSpace;
    swapInfo.imageExtent = swapchainExtent_;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.preTransform = surfCaps.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = presentMode;
    swapInfo.clipped = VK_TRUE;
    swapInfo.oldSwapchain = VK_NULL_HANDLE;

// 7. Create the swapchain
    if (vkCreateSwapchainKHR(device_, &swapInfo, nullptr, &swapchain_) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan swapchain!");
        abort();
    }

// 8. Get swapchain images
    uint32_t scImageCount = 0;
    vkGetSwapchainImagesKHR(device_, swapchain_, &scImageCount, nullptr);
    swapchainImages_.resize(scImageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &scImageCount, swapchainImages_.data());
    LOGE("Swapchain created with %d images, format: %d, extent: %dx%d", scImageCount,
         swapchainFormat_, swapchainExtent_.width, swapchainExtent_.height);

    // After getting swapchainImages_
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); ++i) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat_;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device_, &viewInfo, nullptr, &swapchainImageViews_[i]) !=
            VK_SUCCESS) {
            LOGE("Failed to create image view for swapchain image %d", (int) i);
            abort();
        }
    }

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapchainFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        LOGE("Failed to create render pass");
        abort();
    }

    framebuffers_.resize(swapchainImageViews_.size());
    for (size_t i = 0; i < swapchainImageViews_.size(); ++i) {
        VkImageView attachments[] = {swapchainImageViews_[i]};
        VkFramebufferCreateInfo fbInfo = {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass_;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = attachments;
        fbInfo.width = swapchainExtent_.width;
        fbInfo.height = swapchainExtent_.height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &fbInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            LOGE("Failed to create framebuffer %d", (int) i);
            abort();
        }
    }

    // Command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamily_;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        LOGE("Failed to create command pool");
        abort();
    }

// Command buffers
    commandBuffers_.resize(framebuffers_.size());
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers_.size();

    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
        LOGE("Failed to allocate command buffers");
        abort();
    }


    VkDeviceSize bulletBufferSize = sizeof(bulletVerts);
    createBuffer(device_, physicalDevice_, bulletBufferSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 bulletVertexBuffer_, bulletVertexBufferMemory_);

    void *bulletData;
    vkMapMemory(device_, bulletVertexBufferMemory_, 0, bulletBufferSize, 0, &bulletData);
    memcpy(bulletData, bulletVerts, bulletBufferSize);
    vkUnmapMemory(device_, bulletVertexBufferMemory_);


    VkDeviceSize bufferSize = sizeof(triangleVerts);
    createBuffer(device_, physicalDevice_, bufferSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vertexBuffer_, vertexBufferMemory_);

    // Map and copy vertex triangleData
    void *triangleData;
    vkMapMemory(device_, vertexBufferMemory_, 0, bufferSize, 0, &triangleData);
    memcpy(triangleData, triangleVerts, (size_t) bufferSize);
    vkUnmapMemory(device_, vertexBufferMemory_);

    VkDeviceSize shipBufferSize = sizeof(shipVerts);
    createBuffer(device_, physicalDevice_, shipBufferSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 shipVertexBuffer_, shipVertexBufferMemory_);

// Map/copy ship vertex triangleData
    void *shipData;
    vkMapMemory(device_, shipVertexBufferMemory_, 0, shipBufferSize, 0, &shipData);
    memcpy(shipData, shipVerts, (size_t) shipBufferSize);
    vkUnmapMemory(device_, shipVertexBufferMemory_);

    VkDeviceSize alienBufferSize = sizeof(alienVerts);
    createBuffer(device_, physicalDevice_,
                 alienBufferSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 alienVertexBuffer_, alienVertexBufferMemory_);

    void *alienData;
    vkMapMemory(device_, alienVertexBufferMemory_, 0, alienBufferSize, 0, &alienData);
    memcpy(alienData, alienVerts, (size_t) alienBufferSize);
    vkUnmapMemory(device_, alienVertexBufferMemory_);

    VkDeviceSize overlayBufferSize = sizeof(overlayQuadVerts);
    createBuffer(device_, physicalDevice_,
                 overlayBufferSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 overlayVertexBuffer_, overlayVertexBufferMemory_);

    void *overlayData;
    vkMapMemory(device_, overlayVertexBufferMemory_, 0, overlayBufferSize, 0, &overlayData);
    memcpy(overlayData, overlayQuadVerts, (size_t) overlayBufferSize);
    vkUnmapMemory(device_, overlayVertexBufferMemory_);

    createUniformBuffer();
    initAliens();
    loadAllTextures();
    createMainGraphicsPipeline();
    createOverlayGraphicsPipeline();

}

void Renderer::createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    createBuffer(device_, physicalDevice_, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffer_, uniformBufferMemory_);
    vkMapMemory(device_, uniformBufferMemory_, 0, bufferSize, 0, &uniformBuffersData);
}

void Renderer::updateUniformBuffer(float deltaTime) {


    //UniformBufferObject ubo{};
    ubo_.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo_.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));
    ubo_.proj = glm::perspective(glm::radians(45.0f),
                                 swapchainExtent_.width / (float) swapchainExtent_.height, 0.1f,
                                 10.0f);
    ubo_.proj[1][1] *= 1;

    memcpy(uniformBuffersData, &ubo_, sizeof(ubo_));
}

void Renderer::initAliens() {
    float startX = -0.7f;
    float startY = 0.8f;
    float dx = 0.2f;
    float dy = 0.15f;
    for (int y = 0; y < NUM_ALIENS_Y; ++y) {
        for (int x = 0; x < NUM_ALIENS_X; ++x) {
            int idx = y * NUM_ALIENS_X + x;
            aliens_[idx].x = startX + x * dx;
            aliens_[idx].y = startY - y * dy;
            aliens_[idx].active = true;
        }
    }
}

void Renderer::createMainGraphicsPipeline() {

    // One binding (per-vertex data)
    VkVertexInputBindingDescription mainBindingDesc = {};
    mainBindingDesc.binding = 0;
    mainBindingDesc.stride = sizeof(Vertex);
    mainBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

// Position (location=0)
    VkVertexInputAttributeDescription mainPosDesc = {};
    mainPosDesc.binding = 0;
    mainPosDesc.location = 0;
    mainPosDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    mainPosDesc.offset = offsetof(Vertex, pos);

// Color (location=1)
    VkVertexInputAttributeDescription mainColorDesc = {};
    mainColorDesc.binding = 0;
    mainColorDesc.location = 1;
    mainColorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    mainColorDesc.offset = offsetof(Vertex, color);

    // Color (location=1)
    VkVertexInputAttributeDescription mainUVDesc = {};
    mainUVDesc.binding = 0;
    mainUVDesc.location = 2;
    mainUVDesc.format = VK_FORMAT_R32G32_SFLOAT;
    mainUVDesc.offset = offsetof(Vertex, uv);

    std::vector<VkVertexInputAttributeDescription> vertexAttributeDesc = {mainPosDesc,
                                                                          mainColorDesc,
                                                                          mainUVDesc};
    VkPipelineVertexInputStateCreateInfo mainVertexInputInfo = {};
    mainVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    mainVertexInputInfo.vertexBindingDescriptionCount = 1;
    mainVertexInputInfo.pVertexBindingDescriptions = &mainBindingDesc;
    mainVertexInputInfo.vertexAttributeDescriptionCount = vertexAttributeDesc.size();
    mainVertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDesc.data();

    GraphicsPipelineData graphicsPipelineData{
            .pipeline = mainPipeline_,
            .vertexInputState = mainVertexInputInfo,
            .pipelineLayout = mainPipelineLayout_,
            .viewport {.x=0.0, .y=0.0f, .width=(float) swapchainExtent_.width, .height=(float) swapchainExtent_.height, .minDepth=0.0f, .maxDepth=1.0f},
            .scissor {.offset{0, 0}, .extent = swapchainExtent_}
    };

    setShaderStages(device_, assetManager_, "main.vert.spv", "main.frag.spv",
                    graphicsPipelineData);
    setColorBlending(graphicsPipelineData);
    setViewPortState(graphicsPipelineData);
    setInputAssembly(graphicsPipelineData);
    setRasterizer(graphicsPipelineData);
    setSampling(graphicsPipelineData);
    createMainDescriptor(graphicsPipelineData);
    createPipeline(graphicsPipelineData, "main");


}

void Renderer::createOverlayGraphicsPipeline() {

    GraphicsPipelineData graphicsPipelineData{
        .pipeline = overlayPipeline_,
            .viewport {.x=0.0, .y=0.0f, .width=(float) swapchainExtent_.width, .height=(float) swapchainExtent_.height, .minDepth=0.0f, .maxDepth=1.0f},
            .scissor {.offset{0, 0}, .extent = swapchainExtent_}
    };

    setShaderStages(device_, assetManager_, "overlay.vert.spv", "overlay.frag.spv",
                    graphicsPipelineData);
    setColorBlending(graphicsPipelineData);
    setViewPortState(graphicsPipelineData);
    setInputAssembly(graphicsPipelineData);
    setRasterizer(graphicsPipelineData);
    setSampling(graphicsPipelineData);
    createImageOverlayDescriptor(graphicsPipelineData);

    // Vertex input: just position
    VkVertexInputBindingDescription overlayBindingDesc = {};
    overlayBindingDesc.binding = 0;
    overlayBindingDesc.stride = sizeof(OverlayVertex);
    overlayBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Position (location=0)
    VkVertexInputAttributeDescription overlayPosDesc = {};
    overlayPosDesc.binding = 0;
    overlayPosDesc.location = 0;
    overlayPosDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    overlayPosDesc.offset = offsetof(OverlayVertex, pos);

    VkVertexInputAttributeDescription overlayUvDesc = {};
    overlayUvDesc.binding = 0;
    overlayUvDesc.location = 1;
    overlayUvDesc.format = VK_FORMAT_R32G32_SFLOAT;
    overlayUvDesc.offset = offsetof(OverlayVertex, uv);

    std::vector<VkVertexInputAttributeDescription> overlayVertexAttributeDesc = {overlayPosDesc,
                                                                                 overlayUvDesc};

    VkPipelineVertexInputStateCreateInfo overlayVertexInputInfo = {};
    overlayVertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    overlayVertexInputInfo.vertexBindingDescriptionCount = 1;
    overlayVertexInputInfo.pVertexBindingDescriptions = &overlayBindingDesc;
    overlayVertexInputInfo.vertexAttributeDescriptionCount = overlayVertexAttributeDesc.size();
    overlayVertexInputInfo.pVertexAttributeDescriptions = overlayVertexAttributeDesc.data();


    LOGE("overlay pipelineLayout: %llu", graphicsPipelineData.pipelineLayout);

    graphicsPipelineData.vertexInputState = overlayVertexInputInfo;

    createPipeline(graphicsPipelineData, "overlayPipeline");


}

void setRasterizer(GraphicsPipelineData &graphicsPipelineData) {
    VkPipelineRasterizationStateCreateInfo overlayRasterizer = graphicsPipelineData.rasterizationState;
    overlayRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    overlayRasterizer.depthClampEnable = VK_FALSE;
    overlayRasterizer.rasterizerDiscardEnable = VK_FALSE;
    overlayRasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    overlayRasterizer.lineWidth = 1.0f;
    overlayRasterizer.cullMode = VK_CULL_MODE_NONE;
    overlayRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    overlayRasterizer.depthBiasEnable = VK_FALSE;
}

void setShaderStages(VkDevice device, AAssetManager *assetManager, const char *spirvVertexFilename,
                const char *spirvFragmentFilename,
                GraphicsPipelineData &graphicsPipelineData) {

    auto vertShaderCode = loadAsset(assetManager, spirvVertexFilename);
    auto fragShaderCode = loadAsset(assetManager, spirvFragmentFilename);
    VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    graphicsPipelineData.shaderStages = {vertShaderStageInfo,
                                         fragShaderStageInfo};
}

void setColorBlending(GraphicsPipelineData &graphicsPipelineData) {

    auto &colorBlendAttachment = graphicsPipelineData.colorBlendAttachment;
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
//    colorBlending.blendConstants[0] = 0.0f;
//    colorBlending.blendConstants[1] = 0.0f;
//    colorBlending.blendConstants[2] = 0.0f;
//    colorBlending.blendConstants[3] = 0.0f;
    graphicsPipelineData.colorBlendState = colorBlending;
}

void setViewPortState(GraphicsPipelineData &graphicsPipelineData) {

    VkPipelineViewportStateCreateInfo &viewportState = graphicsPipelineData.viewportState;
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &graphicsPipelineData.viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &graphicsPipelineData.scissor;
}

void setInputAssembly(GraphicsPipelineData &graphicsPipelineData) {
    auto &overlayInputAssembly = graphicsPipelineData.inputAssemblyState;
    overlayInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    overlayInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    overlayInputAssembly.primitiveRestartEnable = VK_FALSE;
}

void setSampling(GraphicsPipelineData &graphicsPipelineData) {
    VkPipelineMultisampleStateCreateInfo &multisampling = graphicsPipelineData.multisamplingState;
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
}

void Renderer::createPipelineLayout(VkPipelineLayoutCreateInfo &pipelineLayoutInfo,GraphicsPipelineData &graphicsPipelineData) {
    VkResult res = vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &graphicsPipelineData.pipelineLayout);
    if (res != VK_SUCCESS) {
        LOGE("Failed to create pipeline layout! error code:%d", res);
        abort();
    }

}

void
Renderer::createPipeline(GraphicsPipelineData &graphicsPipelineData, const char pipelineName[10]) {

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = graphicsPipelineData.pipelineCreateInfo;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = graphicsPipelineData.shaderStages.size();
    pipelineCreateInfo.pStages = graphicsPipelineData.shaderStages.data();
    pipelineCreateInfo.pVertexInputState = &graphicsPipelineData.vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &graphicsPipelineData.inputAssemblyState;
    pipelineCreateInfo.pViewportState = &graphicsPipelineData.viewportState;
    pipelineCreateInfo.pRasterizationState = &graphicsPipelineData.rasterizationState;
    pipelineCreateInfo.pMultisampleState = &graphicsPipelineData.multisamplingState;
    pipelineCreateInfo.pColorBlendState = &graphicsPipelineData.colorBlendState;
    pipelineCreateInfo.layout = graphicsPipelineData.pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass_;
    pipelineCreateInfo.subpass = 0;
    LOGE("creating pipeline name 1: %c", pipelineName[0]);
    VkResult res = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1,
                                             &pipelineCreateInfo, nullptr,
                                             &graphicsPipelineData.pipeline);
    LOGE("creating pipeline name: %c", pipelineName[0]);
    if (res != VK_SUCCESS) {
        LOGE("Failed to create graphics pipeline! error code:%d", res);
        abort();
    }

    switch (pipelineName[0]) {
        case 'm':
            mainPipeline_ = graphicsPipelineData.pipeline;
            mainPipelineLayout_ = graphicsPipelineData.pipelineLayout;
            LOGE("main pipelineLayout: %llu", mainPipelineLayout_);
            break;
        case 'o':
            overlayPipeline_ = graphicsPipelineData.pipeline;
            overlayPipelineLayout_ = graphicsPipelineData.pipelineLayout;
            break;
        default:
            LOGE("Unknown pipeline name: %s", pipelineName);
            break;
    }

    vkDestroyShaderModule(device_, graphicsPipelineData.shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device_, graphicsPipelineData.shaderStages[1].module, nullptr);
}

void
Renderer::createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo,
                                    VkDescriptorSetLayout &descriptorSetLayout) {
    VkResult res = vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr,
                                               &descriptorSetLayout);
    if (res != VK_SUCCESS) {
        LOGE("Failed to create descriptor layout! error code:%d", res);
        abort();
    }
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBuffer cmd = commandBuffers_[imageIndex];

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkClearValue clearColor = {{0.1f, 0.2f, 0.3f, 1.0f}};
    VkRenderPassBeginInfo renderBeginPassInfo = {};
    renderBeginPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBeginPassInfo.renderPass = renderPass_;
    renderBeginPassInfo.framebuffer = framebuffers_[imageIndex];
    renderBeginPassInfo.renderArea.offset = {0, 0};
    renderBeginPassInfo.renderArea.extent = swapchainExtent_;
    renderBeginPassInfo.clearValueCount = 1;
    renderBeginPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderBeginPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline_);


    VkDeviceSize offsets[] = {0};
    // --- Draw triangle (or any background)
    float trianglePos[2] = {0.0, 0.0};

    vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(trianglePos),
                       trianglePos);
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer_, offsets);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    // --- Draw ship
    float shipPos[2] = {shipX_, ship_.y};
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_, 0, 1,
                            &shipDescriptorSet_, 0, nullptr);
    vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(shipPos),
                       &shipPos);
    vkCmdBindVertexBuffers(cmd, 0, 1, &shipVertexBuffer_, offsets);
    vkCmdDraw(cmd, 6, 1, 0, 0);

    // --- Draw bullets (for each active bullet, update buffer and draw)
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets_[i].active) continue;
        float bulletPos[2] = {bullets_[i].x, -bullets_[i].y};
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_, 0, 1,
                                &shipBulletDescriptorSet_, 0, nullptr);
        vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(bulletPos), bulletPos);
        vkCmdBindVertexBuffers(cmd, 0, 1, &bulletVertexBuffer_, offsets);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }

    for (int i = 0; i < MAX_ALIENS; ++i) {
        if (!aliens_[i].active) continue;
        float offset[2] = {aliens_[i].x, -aliens_[i].y};
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_, 0, 1,
                                &alienDescriptorSet_, 0, nullptr);
        vkCmdBindVertexBuffers(cmd, 0, 1, &alienVertexBuffer_, offsets);
        vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(offset),
                           offset);
        vkCmdDraw(cmd, 6, 1, 0, 0);

    }

    if (gameState != GameState::Playing) {
        // Set special color in push constant or UBO (e.g. red for GAME OVER)
        float overlayColor[4];
        if (gameState == GameState::Lost) {
            overlayColor[0] = 1.0f; // Red
            overlayColor[1] = 0.0f;
            overlayColor[2] = 0.0f;
            overlayColor[3] = 0.5f; // Alpha, for see-through
        } else {
            overlayColor[0] = 0.0f; // Green or Blue
            overlayColor[1] = 1.0f;
            overlayColor[2] = 0.5f;
            overlayColor[3] = 0.5f;
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, overlayPipeline_);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, overlayPipelineLayout_, 0, 1,
                                &overlayDescriptorSet_, 0, nullptr);
        vkCmdBindVertexBuffers(cmd, 0, 1, &overlayVertexBuffer_, offsets);
        vkCmdPushConstants(cmd, overlayPipelineLayout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(overlayColor), overlayColor);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }


    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

void Renderer::restartGame() {
    // Reset player

    // Reset aliens
    initAliens();

    // Reset bullets
    for (auto &bullet: bullets_) {
        bullet.active = false;
    }

    // Reset score, level, etc.

    // ---- FIX: Reset frame timer ----
    lastFrameTime = Clock::now();

    gameState = GameState::Playing;
}

void Renderer::spawnBullet() {
    if (gameState == GameState::Playing)
        for (int i = 0; i < MAX_BULLETS; ++i) {
            if (!bullets_[i].active) {
                // Spawn at ship position (x, just above ship)
                bullets_[i].x = shipX_;
                bullets_[i].y = -0.8f; // Start just above ship
                bullets_[i].active = true;
                break;
            }
        }
}

void Renderer::updateShipBuffer() {


    ship_.x = shipX_;
    //ship_.y = shipY_;
    ship_.color[0] = shipX_;
//    Vertex movedVerts[6];
//    for (int i = 0; i < 6; ++i) {
//        movedVerts[i] = baseShipVerts[i];
//        movedVerts[i].pos[0] += shipX_; // Move horizontally
//        movedVerts[i].color[1] += shipX_;
//    }

//    void *data;
//    vkMapMemory(device_, shipVertexBufferMemory_, 0, sizeof(movedVerts), 0, &data);
//    memcpy(data, movedVerts, sizeof(movedVerts));
//    vkUnmapMemory(device_, shipVertexBufferMemory_);

//    void *shipData;
//    vkMapMemory(device_, shipVertexBufferMemory_, 0, sizeof(ship_), 0, &data);
//    memcpy(shipData, ship_, sizeof(ship_));
//    vkUnmapMemory(device_, shipVertexBufferMemory_);
}

void Renderer::updateBullet(float deltaTime) {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets_[i].active) {
            bullets_[i].y += bulletMoveSpeed_ * deltaTime; // Move up
            if (bullets_[i].y > 1.0f)
                bullets_[i].active = false; // Off screen
        }
    }

}

void Renderer::updateAliens(float deltaTime) {
    bool hitEdge = false;
    for (int i = 0; i < MAX_ALIENS; ++i) {
        if (!aliens_[i].active) continue;
        // Clamp X position just inside the edge
        if (aliens_[i].x > 0.85f) aliens_[i].x = 0.85f;
        if (aliens_[i].x < -0.85f) aliens_[i].x = -0.85f;

        aliens_[i].x += alienMoveSpeed_ * alienDirection_ * deltaTime;

        // Check if any alien hits the left or right edge
        if (aliens_[i].x > 0.85f || aliens_[i].x < -0.85f)
            hitEdge = true;
    }
    if (hitEdge) {
        alienDirection_ *= -1;
        // Move all aliens down a bit
        for (int i = 0; i < MAX_ALIENS; ++i) {
            if (aliens_[i].active)
                aliens_[i].y -= 0.04f;
        }
    }
}

void Renderer::updateCollision() {
    for (auto &bullet: bullets_) {
        if (!bullet.active) continue;

        for (auto &alien: aliens_) {
            if (!alien.active) continue;

            if (isCollision(alien, bullet)) {
                alien.active = false;    // Destroy alien
                bullet.active = false;   // Destroy bullet
                // Optionally: score++, play sound, create explosion, etc.
                break; // Stop checking this bullet (it's now gone)
            }
        }
    }
}

void Renderer::updateGameState() {
    if (gameState == GameState::Playing) {
        // --- Game Over: Any alien reaches the bottom (e.g. y < -0.9f)
        for (const auto &alien: aliens_) {
            if (alien.active && alien.y < -0.9f) {
                gameState = GameState::Lost;
                // Optionally: Play sound, trigger animation, etc.
                break;
            }
        }
        // --- Win Condition: All aliens destroyed
        bool anyAlienAlive = false;
        for (const auto &alien: aliens_) {
            if (alien.active) {
                anyAlienAlive = true;
                break;
            }
        }
        if (!anyAlienAlive) {
            gameState = GameState::Won;
            // Optionally: Play win sound, trigger animation, etc.
        }
    }

    if (gameState == GameState::Lost) {
        // Render "GAME OVER" text overlay (could be a texture, or draw a quad)
    }
    if (gameState == GameState::Won) {
        // Render "YOU WIN!" text overlay
    }

}

void Renderer::drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageAvailableSemaphore_, VK_NULL_HANDLE,
                          &imageIndex);

    auto now = Clock::now();
    float deltaTime = std::chrono::duration<float>(now - lastFrameTime).count();
    lastFrameTime = now;

    if (gameState == GameState::Playing) {
        deltaTime = std::min(deltaTime, 0.0167f);
        updateUniformBuffer(deltaTime);
        updateShipBuffer();
        updateBullet(deltaTime);
        updateAliens(deltaTime);
        updateCollision();
        updateGameState();
    }

    recordCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphore_;
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphore_;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphore_;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(graphicsQueue_, &presentInfo);
    vkQueueWaitIdle(graphicsQueue_);
}

Renderer::~Renderer() {

    vkDestroySampler(device_, shipSampler_, nullptr);
    vkDestroyImageView(device_, shipImageView_, nullptr);
    vkFreeMemory(device_, shipImageDeviceMemory_, nullptr);
    vkDestroyImage(device_, shipImage_, nullptr);

    vkDestroySampler(device_, alienSampler_, nullptr);
    vkDestroyImageView(device_, alienImageView_, nullptr);
    vkFreeMemory(device_, alienImageDeviceMemory_, nullptr);
    vkDestroyImage(device_, alienImage_, nullptr);

    vkDestroySampler(device_, shipBulletSampler_, nullptr);
    vkDestroyImageView(device_, shipBulletImageView_, nullptr);
    vkFreeMemory(device_, shipBulletImageDeviceMemory_, nullptr);
    vkDestroyImage(device_, shipBulletImage_, nullptr);

    vkDestroyImageView(device_, overlayImageView_, nullptr);
    vkDestroySampler(device_, overlaySampler_, nullptr);
    vkFreeMemory(device_, overlayImageDeviceMemory_, nullptr);
    vkDestroyImage(device_, overlayImage_, nullptr);

    vkDestroyBuffer(device_, alienVertexBuffer_, nullptr);
    vkFreeMemory(device_, alienVertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, bulletVertexBuffer_, nullptr);
    vkFreeMemory(device_, bulletVertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, overlayVertexBuffer_, nullptr);
    vkFreeMemory(device_, overlayVertexBufferMemory_, nullptr);

    if (shipVertexBuffer_ != VK_NULL_HANDLE)
        vkDestroyBuffer(device_, shipVertexBuffer_, nullptr);
    if (shipVertexBufferMemory_ != VK_NULL_HANDLE)
        vkFreeMemory(device_, shipVertexBufferMemory_, nullptr);
    if (imageAvailableSemaphore_ != VK_NULL_HANDLE)
        vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);
    if (renderFinishedSemaphore_ != VK_NULL_HANDLE)
        vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
    if (mainDescriptorPool_ != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device_, mainDescriptorPool_, nullptr);
    if (uniformBuffer_ != VK_NULL_HANDLE)
        vkDestroyBuffer(device_, uniformBuffer_, nullptr);
    if (uniformBuffersData != nullptr) {
        vkUnmapMemory(device_, uniformBufferMemory_);
        uniformBuffersData = nullptr;
    }
    if (uniformBufferMemory_ != VK_NULL_HANDLE)
        vkFreeMemory(device_, uniformBufferMemory_, nullptr);

    if (vertexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    }
    if (vertexBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, vertexBufferMemory_, nullptr);
    }

    for (auto framebuffer: framebuffers_) {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    for (auto imageView: swapchainImageViews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }

    vkDestroyCommandPool(device_, commandPool_, nullptr);
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
    }
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
    }


}






