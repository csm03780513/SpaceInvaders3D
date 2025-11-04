#include "Renderer.h"
#include "PipelineBuilder.h"
#include "DescriptorHelper.h"
#include "SFXMixer.h"
#include "ECS/systems/AilmentSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <android/native_window.h>
#include <vector>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <span>
#include <array>

struct TextData {
    VkBuffer buffer;
    std::vector<Vertex> vertices;
    VkDeviceSize offset;
};

std::unordered_map<GameText, TextData> allTextVertices;

const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};


// Add these to createInfo.enabledExtensionCount and createInfo.ppEnabledExtensionNames as well


#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


Bullet bullets_[MAX_BULLETS] = {};
Ship ship_ ={};
Alien aliens_[MAX_ALIENS] = {};

float alienMoveSpeed_ = 0.3f;
float bulletMoveSpeed_ = 2.0f;
float alienDirection_ = 1.0f; // 1 = right, -1 = left


std::vector<char> loadShaderAsset(IPlatformServices &platform, const char *filename);

VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory, VkDeviceSize customAllocSize = 0);


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

void createTextureSampler(VkDevice device, VkSampler &sampler, GameTextureType type);

void
setShaderStages(VkDevice device, IPlatformServices &platform, const char *spirvVertexFilename,
                const char *spirvFragmentFilename,
                GfxPipelineData &graphicsPipelineData);

void setColorBlending(GfxPipelineData &graphicsPipelineData);

void setViewPortState(GfxPipelineData &graphicsPipelineData);

void setInputAssembly(GfxPipelineData &graphicsPipelineData);

void setRasterizer(GfxPipelineData &graphicsPipelineData);

void setSampling(GfxPipelineData &graphicsPipelineData);

//void updateFontBuffer(VkDevice device, std::vector<Vertex> textVertices,
//                      VkDeviceMemory fontVertexBufferMemory_);
void updateFontBuffer(VkDevice device, const std::vector<Vertex> &vertices, VkDeviceMemory memory,
                      VkDeviceSize offset);

void uploadDataBuffer(VkDevice device, const void *dataToUpload, VkDeviceSize sizeOfData,
                      VkDeviceMemory bufferMemory);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                           "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        LOGE("Failed to create shader module");
        throw std::runtime_error("Failed to create shader module");
    }
    return shaderModule;
}

inline bool isCollision(const Alien &alien, const Bullet &bullet) {

    if (bullet.bulletType == BulletType::Ship) {
        auto alienAABB = Collision::getAABB(alien.x, alien.y, alien.widthHeight[0],
                                            alien.widthHeight[1]);
        auto bulletAABB = Collision::getAABB(bullet.x, -bullet.y, bullet.widthHeight[0],
                                             bullet.widthHeight[1]);

        return Collision::isColliding(alienAABB, bulletAABB);
    } else if (bullet.bulletType == BulletType::Alien) {
        auto shipAABB = Collision::getAABB(ship_.x, ship_.y, ship_.widthHeight[0],
                                           ship_.widthHeight[1]);
        auto bulletAABB = Collision::getAABB(bullet.x, bullet.y, bullet.widthHeight[0],
                                             bullet.widthHeight[1]);
        return Collision::isColliding(shipAABB, bulletAABB);

    }

    return false;
}

std::vector<char> loadShaderAsset(IPlatformServices &platform, const char *filename) {
    std::string fullPath = "shaders/" + std::string(filename);
    auto bytes = platform.loadAsset(fullPath);
    return std::vector<char>(bytes.begin(), bytes.end());
}


void Renderer::loadTexture(const char *filename, VkImage &vkImage, VkDeviceMemory &vkDeviceMemory,
                           VkImageView &imageView, VkSampler &vkSampler,
                           GameTextureType gameTextureType) {
    std::string fullPath;
    if (gameTextureType == GameTextureType::FontAtlas) {
        fullPath = "fonts/" + std::string(filename);
    } else {
        fullPath = "textures/" + std::string(filename);
    }

    LOGE("file path:%s", fullPath.c_str());
    std::vector<uint8_t> pixelData;
    int textureWidth = 0;
    int textureHeight = 0;
    bool imageIsLoaded = true;

    auto fileData = platformServices_.loadAsset(fullPath);
    if (fileData.empty()) {
        LOGE("failed to load asset: %s", fullPath.c_str());
        imageIsLoaded = false;
    } else {
        int channels = 0;
        unsigned char *decoded = stbi_load_from_memory(fileData.data(), static_cast<int>(fileData.size()),
                                                       &textureWidth, &textureHeight,
                                                       &channels, STBI_rgb_alpha);
        if (!decoded) {
            imageIsLoaded = false;
        } else {
            pixelData.assign(decoded, decoded + textureWidth * textureHeight * 4);
            stbi_image_free(decoded);
        }
    }

    if (gameTextureType == GameTextureType::FontAtlas) {
        // 2. Describe your atlas grid
        int cellW = 32, cellH = 32, cols = 16, rows = 16;

// 3. Auto-scan metrics:
        LOGE("width:%i x hieght:%i", textureWidth, textureHeight);
        fontManager_->autoPackFontAtlas(pixelData, textureWidth, textureHeight,
                                        cellW, cellH, cols, rows);
    }

    if (imageIsLoaded) {
        LOGE("loading image asset:%s", filename);
        VkBuffer stagingBuffer{VK_NULL_HANDLE};
        VkDeviceMemory stagingBufferMemory{VK_NULL_HANDLE};
        // 1. Create staging buffer & copy image data
        // (Create staging buffer, copy data, omitted for brevity)
        VkDeviceSize imageSize = textureWidth * textureHeight * 4;
        createBuffer(device_, physicalDevice_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuffer, stagingBufferMemory);

        void *imageData;
        vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &imageData);
        memcpy(imageData, pixelData.data(), imageSize);
        vkUnmapMemory(device_, stagingBufferMemory);


// 2. Create the Vulkan image (device local)
        createImage(device_, physicalDevice_, textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkImage, vkDeviceMemory);
// 3. Copy from staging buffer to Vulkan image (with transitions)
        transitionImageLayout(device_, commandPool_, graphicsQueue_, vkImage,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copyBufferToImage(device_, commandPool_, graphicsQueue_, stagingBuffer, vkImage,
                          textureWidth, textureHeight);


        transitionImageLayout(device_, commandPool_, graphicsQueue_, vkImage,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
// 4. Create image view and sampler
        createImageView(device_, vkImage, VK_FORMAT_R8G8B8A8_UNORM, imageView);
        createTextureSampler(device_, vkSampler, gameTextureType);

        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkFreeMemory(device_, stagingBufferMemory, nullptr);

    } else {
        LOGE("failed to load overlay image");
    }
}

// Create a Vulkan buffer
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer &buffer, VkDeviceMemory &bufferMemory,
                  VkDeviceSize customAllocSize) // NEW
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size; // size of the buffer, not memory
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkDeviceSize allocSize = (customAllocSize > 0) ? customAllocSize : memRequirements.size;

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = allocSize;

    // Memory type selection
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
        throw std::runtime_error("Failed to allocate buffer memory");
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

void createTextureSampler(VkDevice device, VkSampler &sampler, GameTextureType type) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    if (type == GameTextureType::FontAtlas) {
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

    } else {
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
    }
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

void Renderer::stopAudioPlayer() {
    if (sfxMixer_) {
        sfxMixer_->stop();
    }
}

void Renderer::resumeAudioPlayer() {
    if (sfxMixer_) {
        sfxMixer_->resume();
    }
}

Renderer::Renderer(IPlatformServices &platformServices) : platformServices_(platformServices) {

    initVulkan();
    sfxMixer_ = std::make_shared<SFXMixer>();
    sfxMixer_->initialize(platformServices_, SFX_SAMPLE_RATE, SFX_CHANNELS);
    sfxMixer_->loadClip("shoot", "shoot.wav");
    sfxMixer_->loadClip("explode_1", "explode_1.wav");
    sfxMixer_->loadClip("explode_2", "explode_2.wav");
    sfxMixer_->loadClip("shield", "explode_2.wav");
    explosionClipIds_ = {"explode_1", "explode_2"};

    fontManager_ = std::make_unique<FontManager>();
    util_ = std::make_shared<Util>(device_);
    powerUpManager_ = std::make_shared<PowerUpManager>(device_, util_, sfxMixer_);
    particleSystem_ = std::make_unique<ParticleSystem>(device_, powerUpManager_);


    loadAllTextures();
    loadText();
    loadGameObjects();
    createUniformBuffer();
    initAliens();
    initShip();

    mechanics_ = std::make_unique<GameMechanicsCoordinator>(
            eventBus_,
            ship_,
            std::span<Alien>(aliens_, MAX_ALIENS),
            *powerUpManager_,
            ailSys_,
            ailRules_,
            shieldRules_,
            actualScore);

    damagePopupSubscriptionId_ = eventBus_.subscribeDamagePopup([this](const DamagePopupSpawned &popup) {
        spawnDamageText(popup);
    });

    createMainGfxPipeline();
    createOverlayGfxPipeline();
    createFontGfxPipeline();

    createParticlesGfxPipeline(GfxPipelineType::ExplosionParticles);
    createParticlesGfxPipeline(GfxPipelineType::StarParticles);
    createParticlesGfxPipeline(GfxPipelineType::HaloEffect);

    createGfxPipeline(GfxPipelineType::AxisAlignedBoundingBoxes);

    gameState = GameState::MainMenu;
}

void Renderer::loadAllTextures() {

    loadTexture("ke_ship_1.png", shipImage_, shipImageDeviceMemory_, shipImageView_, shipSampler_,
                GameTextureType::Ship);
    loadTexture("alien_ship_1.png", alienImage_, alienImageDeviceMemory_, alienImageView_,
                alienSampler_, GameTextureType::Alien);
    loadTexture("laser_2.png", shipBulletImage_, shipBulletImageDeviceMemory_, shipBulletImageView_,
                shipBulletSampler_, GameTextureType::ShipBullet);
    loadTexture("tap_to_restart_2.png", overlayImage_, overlayImageDeviceMemory_, overlayImageView_,
                overlaySampler_, GameTextureType::Overlay);
    loadTexture("8bitOperatorBold.png", fontAtlasImage_, fontAtlasImageDeviceMemory_,
                fontAtlasImageView_, fontAtlasSampler_,
                GameTextureType::FontAtlas);
    loadTexture("double_shot_2.png", doubleShotImage_, doubleShotMemory_, doubleShotView_,
                doubleShotSampler_, GameTextureType::PowerUp);
    loadTexture("shield.png", shieldImage_, shieldMemory_, shieldView_, shieldSampler_,
                GameTextureType::PowerUp);

}

void Renderer::createImageOverlayDescriptor(GfxPipelineData &gfxPipelineData) {
    VkDescriptorSetLayoutBinding overlaySamplerLayoutBinding{};
    overlaySamplerLayoutBinding.binding = 0;
    overlaySamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    overlaySamplerLayoutBinding.descriptorCount = 1;
    overlaySamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings{overlaySamplerLayoutBinding};

    VkDescriptorPoolSize overlayPoolSize{};
    overlayPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    overlayPoolSize.descriptorCount = 1;

    std::vector<VkDescriptorPoolSize> poolSizes{overlayPoolSize};

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = overlayImageView_;
    imageInfo.sampler = overlaySampler_;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    std::vector<VkPushConstantRange> pushConstants;
    if (!gfxPipelineData.pushConstantRanges.empty()) {
        pushConstants = gfxPipelineData.pushConstantRanges;
    } else {
        VkPushConstantRange overlayPushConstantRange{};
        overlayPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        overlayPushConstantRange.offset = 0;
        overlayPushConstantRange.size = sizeof(float) * 4; // For vec4 offset
        pushConstants.push_back(overlayPushConstantRange);
    }

    DescriptorSetupResult descriptorResult = createDescriptorResources(
            device_, bindings, poolSizes, 1, pushConstants, gfxPipelineData, {descriptorWrite});

    overlayDescriptorSetLayout_ = descriptorResult.layout;
    overlayDescriptorPool_ = descriptorResult.pool;
    overlayDescriptorSet_ = descriptorResult.descriptorSet;
}

void Renderer::createFontDescriptor(GfxPipelineData &gfxPipelineData) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings{layoutBinding};

    VkDescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSize.descriptorCount = 1;

    std::vector<VkDescriptorPoolSize> poolSizes{descriptorPoolSize};

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = fontAtlasImageView_;
    imageInfo.sampler = fontAtlasSampler_;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    std::vector<VkPushConstantRange> pushConstants;
    if (!gfxPipelineData.pushConstantRanges.empty()) {
        pushConstants = gfxPipelineData.pushConstantRanges;
    } else {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(FontPushConstants);
        pushConstants.push_back(pushConstantRange);
    }

    DescriptorSetupResult descriptorResult = createDescriptorResources(
            device_, bindings, poolSizes, 1, pushConstants, gfxPipelineData, {descriptorWrite});

    fontDescriptorSetLayout_ = descriptorResult.layout;
    fontDescriptorPool_ = descriptorResult.pool;
    fontDescriptorSet_ = descriptorResult.descriptorSet;
}

void Renderer::createMainDescriptor(GfxPipelineData &gfxPipelineData) {
    constexpr uint32_t textureCount = 5;

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = textureCount;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> bindings{uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorPoolSize uboPoolSize{};
    uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboPoolSize.descriptorCount = 1;

    VkDescriptorPoolSize samplerPoolSize{};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = textureCount;

    std::vector<VkDescriptorPoolSize> poolSizes{uboPoolSize, samplerPoolSize};

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer_;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet bufferDescriptorWrite{};
    bufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    bufferDescriptorWrite.dstBinding = 0;
    bufferDescriptorWrite.dstArrayElement = 0;
    bufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bufferDescriptorWrite.descriptorCount = 1;
    bufferDescriptorWrite.pBufferInfo = &bufferInfo;

    std::array<VkDescriptorImageInfo, textureCount> shipImageInfo{{
            {shipSampler_,       shipImageView_,       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {alienSampler_,      alienImageView_,      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {shipBulletSampler_, shipBulletImageView_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {doubleShotSampler_, doubleShotView_,      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {shieldSampler_,     shieldView_,          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
    }};

    VkWriteDescriptorSet samplerDescriptorWrite{};
    samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    samplerDescriptorWrite.dstBinding = 1;
    samplerDescriptorWrite.dstArrayElement = 0;
    samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerDescriptorWrite.descriptorCount = textureCount;
    samplerDescriptorWrite.pImageInfo = shipImageInfo.data();

    std::vector<VkPushConstantRange> pushConstants;
    if (!gfxPipelineData.pushConstantRanges.empty()) {
        pushConstants = gfxPipelineData.pushConstantRanges;
    } else {
        VkPushConstantRange mainPC{};
        mainPC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        mainPC.offset = 0;
        mainPC.size = sizeof(MainPushConstants);
        pushConstants.push_back(mainPC);
    }

    DescriptorSetupResult descriptorResult = createDescriptorResources(
            device_, bindings, poolSizes, 1, pushConstants, gfxPipelineData,
            {bufferDescriptorWrite, samplerDescriptorWrite});

    shipDescriptorSetLayout_ = descriptorResult.layout;
    mainDescriptorPool_ = descriptorResult.pool;
    shipDescriptorSet_ = descriptorResult.descriptorSet;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {

    LOGE("Vulkan Validation: %s", pCallbackData->pMessage);
    return VK_FALSE;
}

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName: validationLayers) {
        bool found = false;
        for (const auto &layerProperties: availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}


void Renderer::createInstance() {
    //    volkInitialize();
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        LOGE("Validation layers requested, but not available!");
        throw std::runtime_error("validation layers requested, but not available!");
    }
    // Create Vulkan instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "3D Space Invaders";
    appInfo.apiVersion = VK_API_VERSION_1_1;

    // Required extensions
    std::vector<const char *> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    if (enableValidationLayers) {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        instanceInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&instanceInfo, nullptr, &instance_) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan instance");
        throw std::runtime_error("Failed to create Vulkan instance");
    }

    if (enableValidationLayers) {
        VkDebugUtilsMessengerEXT debugMessenger;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;

        VkResult result = CreateDebugUtilsMessengerEXT(instance_, &debugCreateInfo, nullptr,
                                                       &debugMessenger);
        if (result != VK_SUCCESS) {
            LOGE("Failed to set up debug messenger!");
            throw std::runtime_error("Failed to set up debug messenger");
        }
    }
// check result...

}

void Renderer::createSurface() {
    // Create Android surface from ANativeWindow
    WindowInfo info = platformServices_.getWindowInfo();
    auto *nativeWindow = static_cast<ANativeWindow *>(info.nativeWindow);
    if (!nativeWindow) {
        throw std::runtime_error("Native window unavailable for Vulkan surface creation");
    }

    VkAndroidSurfaceCreateInfoKHR surfInfo = {};
    surfInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfInfo.window = nativeWindow;

    LOGE("ANativeWindow = %p, scale = %dx%d", nativeWindow, info.width, info.height);

    VkResult surfaceResult = vkCreateAndroidSurfaceKHR(instance_, &surfInfo, nullptr, &surface_);
    if (surfaceResult != VK_SUCCESS) {
        LOGE("Failed to create Android Vulkan surface, error code: %d", surfaceResult);
        throw std::runtime_error("Failed to create Android Vulkan surface");
    }
}

void Renderer::getPhysicalDevice() {
    // 1. Enumerate physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        LOGE("No Vulkan physical devices found");
        throw std::runtime_error("No Vulkan physical devices found");
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
        throw std::runtime_error("No suitable Vulkan physical device found");
    }
    LOGE("Physical device and graphics queue family selected: %u", graphicsQueueFamily_);

}

void Renderer::initVulkan() {// Load Vulkan functions using volk
    createInstance();
    createSurface();
    getPhysicalDevice();

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
        throw std::runtime_error("Failed to create Vulkan logical device");
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
        throw std::runtime_error("Failed to create Vulkan swapchain");
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
            throw std::runtime_error("Failed to create image view");
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
        throw std::runtime_error("Failed to create render pass");
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
            throw std::runtime_error("Failed to create framebuffer");
        }
    }

    // Command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsQueueFamily_;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        LOGE("Failed to create command pool");
        throw std::runtime_error("Failed to create command pool");
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
        throw std::runtime_error("Failed to allocate command buffers");
    }


}

void updateFontBuffer(VkDevice device, const std::vector<Vertex> &vertices, VkDeviceMemory memory,
                      VkDeviceSize offset) {
    void *mapped;
    VkDeviceSize size = vertices.size() * sizeof(Vertex);

    vkMapMemory(device, memory, offset, size, 0, &mapped);
    memcpy(mapped, vertices.data(), size);
    vkUnmapMemory(device, memory);
}

//void updateFontBuffer(VkDevice device, std::vector<Vertex> textVertices,
//                      VkDeviceMemory fontVertexBufferMemory) {
//    VkDeviceSize textBufferSize = textVertices.size() * sizeof(Vertex);
//    void *fontData;
//    vkMapMemory(device, fontVertexBufferMemory, 0, textBufferSize, 0, &fontData);
//    memcpy(fontData, textVertices.data(), (size_t) textBufferSize);
//    vkUnmapMemory(device, fontVertexBufferMemory);
//}

void uploadDataBuffer(VkDevice device, const void *dataToUpload, VkDeviceSize sizeOfData,
                      VkDeviceMemory bufferMemory) {
    void *fontData;
    vkMapMemory(device, bufferMemory, 0, sizeOfData, 0, &fontData);
    memcpy(fontData, dataToUpload, (size_t) sizeOfData);
    vkUnmapMemory(device, bufferMemory);
}

void Renderer::createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    createBuffer(device_, physicalDevice_, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffer_, uniformBufferMemory_);
    vkMapMemory(device_, uniformBufferMemory_, 0, bufferSize, 0, &uniformBuffersData);
}

void Renderer::updateUniformBuffer() {


    //UniformBufferObject ubo{};
    ubo_.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo_.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));
    ubo_.proj = glm::perspective(glm::radians(45.0f),
                                 swapchainExtent_.width / (float) swapchainExtent_.height, 0.1f,
                                 10.0f);
    // Flip the Y coordinate for Vulkan's coordinate system
    ubo_.proj[1][1] *= -1;

    memcpy(uniformBuffersData, &ubo_, sizeof(ubo_));
}


void Renderer::createMainGfxPipeline() {
    PipelineBuilder builder(device_, renderPass_, swapchainExtent_, platformServices_);
    builder.setShaderFilenames("main.vert.spv", "main.frag.spv");

    VkVertexInputBindingDescription mainBindingDesc{};
    mainBindingDesc.binding = 0;
    mainBindingDesc.stride = sizeof(Vertex);
    mainBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription mainPosDesc{};
    mainPosDesc.binding = 0;
    mainPosDesc.location = 0;
    mainPosDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    mainPosDesc.offset = offsetof(Vertex, pos);

    VkVertexInputAttributeDescription mainColorDesc{};
    mainColorDesc.binding = 0;
    mainColorDesc.location = 1;
    mainColorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    mainColorDesc.offset = offsetof(Vertex, color);

    VkVertexInputAttributeDescription mainUVDesc{};
    mainUVDesc.binding = 0;
    mainUVDesc.location = 2;
    mainUVDesc.format = VK_FORMAT_R32G32_SFLOAT;
    mainUVDesc.offset = offsetof(Vertex, uv);

    builder.setVertexInput({mainBindingDesc}, {mainPosDesc, mainColorDesc, mainUVDesc});

    VkPushConstantRange mainPC{};
    mainPC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    mainPC.offset = 0;
    mainPC.size = sizeof(MainPushConstants);
    builder.setPushConstantRanges({mainPC});

    builder.setDescriptorLayoutCallback([this](GfxPipelineData &data) {
        createMainDescriptor(data);
    });

    PipelineHandles handles = builder.build(*this);
    mainPipeline_ = handles.pipeline;
    mainPipelineLayout_ = handles.layout;
}

void Renderer::createOverlayGfxPipeline() {
    PipelineBuilder builder(device_, renderPass_, swapchainExtent_, platformServices_);
    builder.setShaderFilenames("overlay.vert.spv", "overlay.frag.spv");

    VkVertexInputBindingDescription overlayBindingDesc{};
    overlayBindingDesc.binding = 0;
    overlayBindingDesc.stride = sizeof(OverlayVertex);
    overlayBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription overlayPosDesc{};
    overlayPosDesc.binding = 0;
    overlayPosDesc.location = 0;
    overlayPosDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    overlayPosDesc.offset = offsetof(OverlayVertex, pos);

    VkVertexInputAttributeDescription overlayUvDesc{};
    overlayUvDesc.binding = 0;
    overlayUvDesc.location = 1;
    overlayUvDesc.format = VK_FORMAT_R32G32_SFLOAT;
    overlayUvDesc.offset = offsetof(OverlayVertex, uv);

    builder.setVertexInput({overlayBindingDesc}, {overlayPosDesc, overlayUvDesc});

    VkPushConstantRange overlayPushConstantRange{};
    overlayPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    overlayPushConstantRange.offset = 0;
    overlayPushConstantRange.size = sizeof(float) * 4;
    builder.setPushConstantRanges({overlayPushConstantRange});

    builder.setDescriptorLayoutCallback([this](GfxPipelineData &data) {
        createImageOverlayDescriptor(data);
    });

    PipelineHandles handles = builder.build(*this);
    overlayPipeline_ = handles.pipeline;
    overlayPipelineLayout_ = handles.layout;
}

void Renderer::createFontGfxPipeline() {
    PipelineBuilder builder(device_, renderPass_, swapchainExtent_, platformServices_);
    builder.setShaderFilenames("font.vert.spv", "font.frag.spv");

    VkVertexInputBindingDescription inputBindingDescription{};
    inputBindingDescription.binding = 0;
    inputBindingDescription.stride = sizeof(Vertex);
    inputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription posDesc{};
    posDesc.binding = 0;
    posDesc.location = 0;
    posDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    posDesc.offset = offsetof(Vertex, pos);

    VkVertexInputAttributeDescription colorDesc{};
    colorDesc.binding = 0;
    colorDesc.location = 1;
    colorDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorDesc.offset = offsetof(Vertex, color);

    VkVertexInputAttributeDescription uvDesc{};
    uvDesc.binding = 0;
    uvDesc.location = 2;
    uvDesc.format = VK_FORMAT_R32G32_SFLOAT;
    uvDesc.offset = offsetof(Vertex, uv);

    builder.setVertexInput({inputBindingDescription}, {posDesc, colorDesc, uvDesc});

    VkPushConstantRange fontPushConstantRange{};
    fontPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    fontPushConstantRange.offset = 0;
    fontPushConstantRange.size = sizeof(FontPushConstants);
    builder.setPushConstantRanges({fontPushConstantRange});

    builder.setDescriptorLayoutCallback([this](GfxPipelineData &data) {
        createFontDescriptor(data);
    });

    PipelineHandles handles = builder.build(*this);
    fontPipeline_ = handles.pipeline;
    fontPipelineLayout_ = handles.layout;
}

void Renderer::createGfxPipeline(GfxPipelineType gfxPipelineType) {
    if (gfxPipelineType != GfxPipelineType::AxisAlignedBoundingBoxes) {
        LOGE("Unknown graphics pipeline type");
        return;
    }

    PipelineBuilder builder(device_, renderPass_, swapchainExtent_, platformServices_);
    builder.setShaderFilenames("aabb.vert.spv", "aabb.frag.spv");

    auto bindings = Vertex::getBindingDescriptions();
    auto attributes = Vertex::getAttributeDescriptions();
    builder.setVertexInput(bindings, attributes);
    builder.setTopology(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);

    builder.setDescriptorLayoutCallback([this](GfxPipelineData &data) {
        VkPipelineLayoutCreateInfo aabbPipelineLayoutInfo{};
        aabbPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        aabbPipelineLayoutInfo.setLayoutCount = 0;
        aabbPipelineLayoutInfo.pSetLayouts = nullptr;
        aabbPipelineLayoutInfo.pushConstantRangeCount = 0;
        aabbPipelineLayoutInfo.pPushConstantRanges = nullptr;

        createPipelineLayout(aabbPipelineLayoutInfo, data);
    });

    PipelineHandles handles = builder.build(*this);
    util_->aabbPipeline = handles.pipeline;
    util_->aabbPipelineLayout = handles.layout;
}

void Renderer::createParticlesGfxPipeline(GfxPipelineType gfxPipelineType) {
    PipelineBuilder builder(device_, renderPass_, swapchainExtent_, platformServices_);

    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    switch (gfxPipelineType) {
        case GfxPipelineType::ExplosionParticles:
            builder.setShaderFilenames("particles_instanced.vert.spv", "particles_instanced.frag.spv");
            bindings = ParticleInstance::getBindingDescriptions();
            attributes = ParticleInstance::getAttributeDescriptions();
            break;
        case GfxPipelineType::StarParticles:
            builder.setShaderFilenames("stars_instanced.vert.spv", "stars_instanced.frag.spv");
            bindings = StarInstance::getBindingDescriptions();
            attributes = StarInstance::getAttributeDescriptions();
            break;
        case GfxPipelineType::HaloEffect:
            builder.setShaderFilenames("halo.vert.spv", "halo.frag.spv");
            bindings = ShieldInstance::getBindingDescriptions();
            attributes = ShieldInstance::getAttributeDescriptions();
            break;
        default:
            LOGE("Unknown particles pipeline type");
            return;
    }

    builder.setVertexInput(bindings, attributes);

    builder.setDescriptorLayoutCallback([this](GfxPipelineData &data) {
        VkDescriptorSetLayoutBinding particleInstanceBinding{};
        particleInstanceBinding.binding = 0;
        particleInstanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        particleInstanceBinding.descriptorCount = 0;
        particleInstanceBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo particlesLayoutInfo{};
        particlesLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        particlesLayoutInfo.bindingCount = 1;
        particlesLayoutInfo.pBindings = &particleInstanceBinding;

        createDescriptorSetLayout(particlesLayoutInfo, particlesDescriptorSetLayout_);

        VkPipelineLayoutCreateInfo particlesPipelineLayoutInfo{};
        particlesPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        particlesPipelineLayoutInfo.setLayoutCount = 0;
        particlesPipelineLayoutInfo.pSetLayouts = nullptr;
        particlesPipelineLayoutInfo.pushConstantRangeCount = 0;
        particlesPipelineLayoutInfo.pPushConstantRanges = nullptr;

        createPipelineLayout(particlesPipelineLayoutInfo, data);
    });

    PipelineHandles handles = builder.build(*this);

    switch (gfxPipelineType) {
        case GfxPipelineType::ExplosionParticles:
            explosionParticlesPipeline_ = handles.pipeline;
            particlesPipelineLayout_ = handles.layout;
            break;
        case GfxPipelineType::StarParticles:
            starParticlesPipeline_ = handles.pipeline;
            particlesPipelineLayout_ = handles.layout;
            break;
        case GfxPipelineType::HaloEffect:
            particleSystem_->haloPipeline = handles.pipeline;
            break;
        default:
            break;
    }
}

void setRasterizer(GfxPipelineData &graphicsPipelineData) {
    auto &overlayRasterizer = graphicsPipelineData.rasterizationState;
    overlayRasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    overlayRasterizer.depthClampEnable = VK_FALSE;
    overlayRasterizer.rasterizerDiscardEnable = VK_FALSE;
    overlayRasterizer.lineWidth = 1.0f;
    overlayRasterizer.cullMode = VK_CULL_MODE_NONE;
    overlayRasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    overlayRasterizer.depthBiasEnable = VK_FALSE;
}

void setShaderStages(VkDevice device, IPlatformServices &platform, const char *spirvVertexFilename,
                     const char *spirvFragmentFilename,
                     GfxPipelineData &graphicsPipelineData) {

    auto vertShaderCode = loadShaderAsset(platform, spirvVertexFilename);
    auto fragShaderCode = loadShaderAsset(platform, spirvFragmentFilename);
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

void setColorBlending(GfxPipelineData &graphicsPipelineData) {

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

void setViewPortState(GfxPipelineData &graphicsPipelineData) {

    VkPipelineViewportStateCreateInfo &viewportState = graphicsPipelineData.viewportState;
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &graphicsPipelineData.viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &graphicsPipelineData.scissor;
}

void setInputAssembly(GfxPipelineData &graphicsPipelineData) {
    auto &overlayInputAssembly = graphicsPipelineData.inputAssemblyState;
    overlayInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    overlayInputAssembly.primitiveRestartEnable = VK_FALSE;
}

void setSampling(GfxPipelineData &graphicsPipelineData) {
    VkPipelineMultisampleStateCreateInfo &multisampling = graphicsPipelineData.multisamplingState;
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
}

void Renderer::createPipelineLayout(VkPipelineLayoutCreateInfo &pipelineLayoutInfo,
                                    GfxPipelineData &gfxPipelineData) {
    VkResult res = vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr,
                                          &gfxPipelineData.pipelineLayout);
    if (res != VK_SUCCESS) {
        LOGE("Failed to create pipeline layout! error code:%d", res);
        throw std::runtime_error("Failed to create pipeline layout");
    }

}

PipelineHandles
Renderer::createPipeline(GfxPipelineData &gfxPipelineData) {

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = gfxPipelineData.pipelineCreateInfo;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = gfxPipelineData.shaderStages.size();
    pipelineCreateInfo.pStages = gfxPipelineData.shaderStages.data();
    pipelineCreateInfo.pVertexInputState = &gfxPipelineData.vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &gfxPipelineData.inputAssemblyState;
    pipelineCreateInfo.pViewportState = &gfxPipelineData.viewportState;
    pipelineCreateInfo.pRasterizationState = &gfxPipelineData.rasterizationState;
    pipelineCreateInfo.pMultisampleState = &gfxPipelineData.multisamplingState;
    pipelineCreateInfo.pColorBlendState = &gfxPipelineData.colorBlendState;
    pipelineCreateInfo.layout = gfxPipelineData.pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass_;
    pipelineCreateInfo.subpass = 0;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult res = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1,
                                             &pipelineCreateInfo, nullptr,
                                             &pipeline);
    if (res != VK_SUCCESS) {
        LOGE("Failed to create graphics pipeline! error code:%d", res);
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    gfxPipelineData.pipeline = pipeline;
    return {pipeline, gfxPipelineData.pipelineLayout};
}

void
Renderer::createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo,
                                    VkDescriptorSetLayout &descriptorSetLayout) {
    VkResult res = vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr,
                                               &descriptorSetLayout);
    if (res != VK_SUCCESS) {
        LOGE("Failed to create descriptor layout! error code:%d", res);
        throw std::runtime_error("Failed to create descriptor layout");
    }
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBuffer cmd = commandBuffers_[imageIndex];

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkClearValue clearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};
    VkRenderPassBeginInfo renderBeginPassInfo = {};
    renderBeginPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBeginPassInfo.renderPass = renderPass_;
    renderBeginPassInfo.framebuffer = framebuffers_[imageIndex];
    renderBeginPassInfo.renderArea.offset = {0, 0};
    renderBeginPassInfo.renderArea.extent = swapchainExtent_;
    renderBeginPassInfo.clearValueCount = 1;
    renderBeginPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderBeginPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    particleSystem_->recordCommandBuffer(cmd,
                                         particlesPipelineLayout_,
                                         starParticlesPipeline_,
                                         starVertsBuffer_,
                                         starIndexBuffer_,
                                         starInstanceBuffer_,
                                         GfxPipelineType::StarParticles);


    VkDeviceSize offsets[] = {0};
    // --- Draw triangle (or any background)
    float trianglePos[2] = {0.0, 0.0};
    MainPushConstants trianglePC;
    trianglePC.pos = {0.0f, -0.9f};
    trianglePC.shakeOffset = {0.0f, 0.0f};
    trianglePC.flashAmount = 0.0f;
    trianglePC.texturePos = 4;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline_);
    vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(MainPushConstants),
                       &trianglePC);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_, 0, 1,
                            &shipDescriptorSet_, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer_, offsets);
    vkCmdDraw(cmd, sizeof(quadVerts) / sizeof(Vertex), 1, 0, 0);


    powerUpManager_->recordCommandBuffer(cmd, mainPipelineLayout_, mainPipeline_, shakeOffset,
                                         shipDescriptorSet_);

    // --- Draw ship
    float flashAmount = {0.0f};

    shipPC_.pos = {shipX_, ship_.y};
    shipPC_.shakeOffset = shakeOffset;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_, 0, 1,
                            &shipDescriptorSet_, 0, nullptr);
    vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(MainPushConstants),
                       &shipPC_);

    vkCmdBindVertexBuffers(cmd, 0, 1, &shipVertexBuffer_, offsets);
    vkCmdDraw(cmd, 6, 1, 0, 0);

    auto shipAABB = Collision::getAABB(ship_.x, ship_.y, ship_.widthHeight[0],
                                       ship_.widthHeight[1]);
//    util_->recordDrawBoundingBox(cmd, shipAABB, {0.0f, 1.0f, 1.0f});


    // --- Draw bullets (for each active bullet, updateExplosionParticles buffer and draw)
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets_[i].active) continue;
        bulletPC_[i].pos = {bullets_[i].x, bullets_[i].y};
        bulletPC_[i].shakeOffset = shakeOffset;
        bulletPC_[i].texturePos = 2;
        bulletPC_[i].scale = {0.5f, 0.5f};

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline_);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_, 0, 1,
                                &shipDescriptorSet_, 0, nullptr);
        vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(MainPushConstants), &bulletPC_[i]);
        vkCmdBindVertexBuffers(cmd, 0, 1, &bulletVertexBuffer_, offsets);
        vkCmdDraw(cmd, 6, 1, 0, 0);

    }

    for (int i = 0; i < MAX_ALIENS; ++i) {
        if (!aliens_[i].active) continue;
        alienPC_[i].pos = {aliens_[i].x, -aliens_[i].y};
        alienPC_[i].shakeOffset = shakeOffset;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline_);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_, 0, 1,
                                &shipDescriptorSet_, 0, nullptr);
        vkCmdBindVertexBuffers(cmd, 0, 1, &alienVertexBuffer_, offsets);
        vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(MainPushConstants),
                           &alienPC_[i]);
        vkCmdDraw(cmd, 6, 1, 0, 0);
//        util_->recordDrawBoundingBox(cmd, alienAABB, {1.0f,0.0f,0.0f});

    }

    if (gameState != GameState::Playing) {
        // Set special color in push constant or UBO (e.g. red for GAME OVER)
        float overlayColor[4];
        if (gameState == GameState::Lost) {
            overlayColor[0] = 1.0f; // Red
            overlayColor[1] = 0.0f;
            overlayColor[2] = 0.0f;
            overlayColor[3] = 0.8f; // Alpha, for see-through
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


    for (const auto &[textName, textData]: allTextVertices) {
        FontPushConstants fontPushConstants{};
        fontPushConstants.currentTime = floatingDamageGlobalTime_;
        fontPushConstants.startTime = 0.0f;
        fontPushConstants.lifetime = 0.0f;
        fontPushConstants.riseSpeed = 0.0f;
        fontPushConstants.startScale = 1.0f;
        fontPushConstants.endScale = 1.0f;
        fontPushConstants.fadeStart = 1.0f;
        fontPushConstants.color = {1.0f, 1.0f, 1.0f, 1.0f};
        switch (textName) {
            case GameText::Title:
                fontPushConstants.color = {0.85f, 0.95f, 1.0f, 1.0f};
                break;
            case GameText::DoubleShotCD:
                fontPushConstants.color = {1.0f, 0.85f, 0.35f, 1.0f};
                break;
            case GameText::ShieldCD:
                fontPushConstants.color = {0.4f, 0.9f, 1.0f, 1.0f};
                break;
            default:
                break;
        }
        auto it = powerUpManager_->collectedPowerUps.find(textName);
        if (it != powerUpManager_->collectedPowerUps.end()) {
            if (it->second.active) {
                auto textPos = it->second.textPos;
                fontPushConstants.pos = {textPos.x, textPos.y};

                MainPushConstants pushConstants = {};
                pushConstants.pos = {textPos.x + 0.1, textPos.y + 0.03};
                pushConstants.scale = {0.7f, 0.7f};
                pushConstants.time = 0.0f;
                pushConstants.canPulse = 0;

                //show power up icons
                if (textName == GameText::DoubleShotCD) pushConstants.texturePos = 3;
                if (textName == GameText::ShieldCD) pushConstants.texturePos = 4;

                VkDeviceSize offsets[] = {0};
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline_);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_,
                                        0, 1,
                                        &shipDescriptorSet_, 0, nullptr);
                vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                                   sizeof(MainPushConstants), &pushConstants);

                vkCmdBindVertexBuffers(cmd, 0, 1, &powerUpManager_->powerUpBuffer, offsets);
                vkCmdDraw(cmd, 6, 1, 0, 0);


            } else {
                continue;
            }
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipeline_);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipelineLayout_, 0, 1,
                                &fontDescriptorSet_, 0, nullptr);
        vkCmdPushConstants(cmd, fontPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(FontPushConstants), &fontPushConstants);

        VkDeviceSize vertexOffset = textData.offset;
        vkCmdBindVertexBuffers(cmd, 0, 1, &textData.buffer, &vertexOffset);
        vkCmdDraw(cmd, static_cast<uint32_t>(textData.vertices.size()), 1, 0, 0);
    }

    drawFloatingDamageTexts(cmd);


    particleSystem_->recordCommandBuffer(cmd,
                                         particlesPipelineLayout_,
                                         explosionParticlesPipeline_,
                                         particlesVertexBuffer_,
                                         particlesIndexBuffer_,
                                         particlesInstanceBuffer_,
                                         GfxPipelineType::ExplosionParticles);

    particleSystem_->recordCommandBuffer(cmd,
                                         particlesPipelineLayout_,
                                         starParticlesPipeline_,
                                         starInstanceBuffer_,
                                         starIndexBuffer_,
                                         starInstanceBuffer_,
                                         GfxPipelineType::HaloEffect);

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

void Renderer::restartGame() {
    // Reset player

    // Reset aliens
    initAliens();
    ship_.health.hull = 100.0f;
    alienMoveSpeed_ = 0.3f;

    // Reset bullets
    for (auto &bullet: bullets_) {
        bullet.active = false;
    }

    floatingDamageInstances_.clear();
    pendingFloatingDamageUploads_.clear();
    powerUpTextCDOffset = powerUpTextStartOffset_;
    floatingDamageBufferCursor_ = powerUpTextCDOffset;
    floatingDamageGlobalTime_ = 0.0f;

    // Reset score, level, etc.
    gameState = GameState::Playing;
}

void Renderer::setShipPosition(float x, float y, bool fireBullet) {
    shipX_ = x;
    shipY_ = y - 0.12f;
    if (fireBullet) {
        spawnBullet(BulletType::Ship, {x, y - 0.12f});
    }
}

void Renderer::spawnBullet(BulletType bulletType, glm::vec2 spawnPos) {

    if (gameState == GameState::Playing) {
        int spawned = 0;
        bool doubleShot = powerUpManager_->doubleShotActive;
        for (int i = 0; i < MAX_BULLETS; ++i) {
            if (!bullets_[i].active && BulletType::Ship == bulletType && canFire) {
                bullets_[i].bulletType = bulletType;
                if (doubleShot && spawned == 0) {
                    // Left bullet
                    bullets_[i].x = spawnPos.x - 0.05f;
                    bullets_[i].y = spawnPos.y - 0.04f;
                    bullets_[i].active = true;
                    bullets_[i].payload = makeKinetic(Util::getRandomFloat(10.0f,40.0f), 0.2f);
//                    if (sfxMixer_) sfxMixer_->playClip("shoot", 0.05f);
                    spawned++;
                } else if (doubleShot && spawned == 1) {
                    // Right bullet
                    bullets_[i].x = spawnPos.x + 0.05f;
                    bullets_[i].y = spawnPos.y - 0.04f;
                    bullets_[i].active = true;
                    bullets_[i].payload = makeKinetic(Util::getRandomFloat(10.0f,40.0f));
//                    if (sfxMixer_) sfxMixer_->playClip("shoot", 0.05f);
                    spawned++;
                    break; // Spawned both bullets
                } else if (!doubleShot) {
                    // Normal shot: center
                    bullets_[i].x = spawnPos.x;
                    bullets_[i].y = spawnPos.y - 0.04f;
                    bullets_[i].active = true;
                    bullets_[i].payload = makePlasma(Util::getRandomFloat(10.0f,40.0f));
                  //  if (sfxMixer_) sfxMixer_->playClip("shoot", 0.05f);
                    break;
                }
//                 canFire = false;
//                 lastFireTime = 0.0f;
            }

            if (!bullets_[i].active && bulletType == BulletType::Alien) {
                bullets_[i].x = spawnPos.x;
                bullets_[i].y = spawnPos.y + 0.04f;
                bullets_[i].active = true;
                bullets_[i].bulletType = bulletType;
                bullets_[i].payload = makeKinetic(Util::getRandomFloat(10.0f,30.0f));
                break;
            }
        }
    }
}

void Renderer::updateShip() const {
    ship_.x = shipX_;
    ship_.y = shipY_;
    ship_.color[0] = shipX_;
}

void Renderer::setGameState(GameState state) {
    gameState = state;
}

GameState Renderer::getGameState() const {
    return gameState;
}

bool Renderer::hasActiveAliens() const {
    for (const auto &alien: aliens_) {
        if (alien.active) {
            return true;
        }
    }
    return false;
}

bool Renderer::hasAlienBelow(float threshold) const {
    for (const auto &alien: aliens_) {
        if (alien.active && alien.y < threshold) {
            return true;
        }
    }
    return false;
}

void Renderer::updateBullet() {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets_[i].active) {
            if (bullets_[i].bulletType == BulletType::Ship)
                bullets_[i].y -= bulletMoveSpeed_ * GameTime::deltaTime; // Move up
            if (bullets_[i].bulletType == BulletType::Alien)
                bullets_[i].y += 0.5f * GameTime::deltaTime;             // Move down

            if ((bullets_[i].bulletType == BulletType::Ship && bullets_[i].y < -1.0f) ||
                (bullets_[i].bulletType == BulletType::Alien && bullets_[i].y > 1.0f)) {
                bullets_[i].active = false; // Off screen
            }
        }
    }

}

//float timePassed = 0.0f;

void Renderer::updateAliens() {
    bool hitEdge = false;

    for (int i = 0; i < MAX_ALIENS; ++i) {
//        timePassed += Time::deltaTime;
        if (!aliens_[i].active) continue;
        // update flash amount (fade in/out) smoothly
        alienPC_[i].flashAmount -= GameTime::deltaTime * 5.0f; // fade speed (0.2s)
        if (alienPC_[i].flashAmount < 0.0f) alienPC_[i].flashAmount = 0.0f;

        switch (aliens_[i].movementType) {
            case TogetherOne:
                aliens_[i].y -= aliens_[i].vy * GameTime::deltaTime;
                break;
            case SineWave:
                aliens_[i].movementTimer += GameTime::deltaTime;
                aliens_[i].x = aliens_[i].baseX + aliens_[i].amplitude *
                                                  sin((aliens_[i].movementTimer *
                                                       aliens_[i].frequency));
                aliens_[i].y -= aliens_[i].vy * GameTime::deltaTime;
                break;
            case MySnakeWave:
                aliens_[i].movementTimer += GameTime::deltaTime;
                aliens_[i].x = sin((aliens_[i].movementTimer + aliens_[i].baseX) * aliens_[i].frequency);
                aliens_[i].y -= aliens_[i].vy * GameTime::deltaTime;
                break;
            case SnakeWave: {
                aliens_[i].movementTimer += GameTime::deltaTime;

                const int row = i / NUM_ALIENS_X;
                const int col = i % NUM_ALIENS_X;

                // Travelling wave that offsets rows/columns for a snaking motion.
                const float basePhase = aliens_[i].movementTimer * aliens_[i].frequency;
                const float rowPhase = row * 0.45f;
                const float colPhase = col * 0.25f;

                const float primaryWave = sin(basePhase + rowPhase);
                const float secondaryWave = sin(basePhase * 0.65f + colPhase);

                aliens_[i].x = aliens_[i].baseX +
                               aliens_[i].amplitude * (0.75f * primaryWave + 0.35f * secondaryWave);

                // Add a subtle vertical bob to make the formation feel alive.
                const float verticalBobVelocity =
                        cos(basePhase + rowPhase) * aliens_[i].frequency * 0.12f;
                aliens_[i].y -= aliens_[i].vy * GameTime::deltaTime;
                aliens_[i].y += verticalBobVelocity * GameTime::deltaTime;

                // Nudge columns out of phase to emphasize the snake-like trail.
                aliens_[i].x += 0.05f * sin(basePhase * 1.8f + colPhase + rowPhase);

                break;
            }
            case JustGoDown:
                aliens_[i].y -= aliens_[i].vy * GameTime::deltaTime;
                break;
            case Circle:
                break;
            case LeftRight:
                // Clamp X position just inside the edge
                if (aliens_[i].x > 0.85f) aliens_[i].x = 0.85f;
                if (aliens_[i].x < -0.85f) aliens_[i].x = -0.85f;

                aliens_[i].x += alienMoveSpeed_ * alienDirection_ * GameTime::deltaTime;

                // Check if any alien hits the left or right edge
                if (aliens_[i].x > 0.85f || aliens_[i].x < -0.85f) {
                    hitEdge = true;
                    if (hitEdge) {
                        alienDirection_ *= -1;
                        // Move all aliens down a bit
                        for (auto &alien: aliens_) {
                            if (alien.active)
                                alien.y -= 0.04f;
                        }
                    }
                }
                break;
        }

    }

}

uint32_t wave = 0;
uint32_t numOfAliens = 100;
uint32_t level = 0;

void Renderer::initAliens() {
    Resistances enemyRes{};
    enemyRes.byType[(int)DamageType::Kinetic]   = 0.10f;
    enemyRes.byType[(int)DamageType::Fire]      = 0.10f;
    enemyRes.byType[(int)DamageType::Lightning] = 0.05f;
    enemyRes.byType[(int)DamageType::Cold]      = 0.00f;
    enemyRes.byType[(int)DamageType::Poison]    = 0.00f;
    enemyRes.byType[(int)DamageType::Radiation] = 0.15f;
    enemyRes.byType[(int)DamageType::Plasma]    = 0.05f;
    enemyRes.byType[(int)DamageType::DarkMatter]= -0.10f; // vulnerable
    enemyRes.byType[(int)DamageType::Cosmic]    = 0.20f;
//    numOfAliens = Util::getRandomUint(0, NUM_ALIENS_X * NUM_ALIENS_Y);
    float startX = -0.7f;
    float startY = 0.8f;
    float dx = 0.2f;
    float dy = 0.15f;
    level++;
    if (wave >= 4) wave = 0; // reset wave
    for (int y = 0; y < NUM_ALIENS_Y; ++y) {
        for (int x = 0; x < NUM_ALIENS_X; ++x) {
            int idx = y * NUM_ALIENS_X + x;
            aliens_[idx].x = startX + x * dx;
            aliens_[idx].baseX = startX + x * dx;
            aliens_[idx].movementTimer = 0.0f;
            aliens_[idx].y = startY - y * dy;
            aliens_[idx].active = true;
            aliens_[idx].health.hull = 100.0f;
            aliens_[idx].health.dead = false;
            aliens_[idx].resistances = enemyRes;
            aliens_[idx].ailments = {};

            aliens_[idx].widthHeight = Util::getQuadWidthHeight(alienVerts, 6, {1.0, 1.0});
            alienPC_[idx].texturePos = 1;
            if (numOfAliens >= 1) {
                switch (wave) {
                    case 0:
                        aliens_[idx].movementType = AlienMovementType::LeftRight;
                        break;
                    case 1:
                        aliens_[idx].frequency = aliens_[idx].baseFrequency * 7 + (level * 0.05);
                        aliens_[idx].movementType = AlienMovementType::SnakeWave;
                        aliens_[idx].vy += 0.01f;
                        break;
                    case 2:
                        aliens_[idx].movementType = AlienMovementType::SineWave;
                        aliens_[idx].frequency = aliens_[idx].baseFrequency * 5 + (level * 0.05);
                        aliens_[idx].vy += 0.01f;
                        break;
                    case 3:
                        aliens_[idx].movementType = AlienMovementType::TogetherOne;
                        aliens_[idx].x = 0.0f;
                        aliens_[idx].vy += 0.01f;
                        break;
                    default:
                        aliens_[idx].movementType = AlienMovementType::JustGoDown;
                        break;
                }
                // numOfAliens--;
            }
        }
    }
    wave++;
}

void Renderer::initShip() {
    Resistances enemyRes{};
    enemyRes.byType[(int)DamageType::Kinetic]   = 0.10f;
    enemyRes.byType[(int)DamageType::Fire]      = 0.10f;
    enemyRes.byType[(int)DamageType::Lightning] = 0.05f;
    enemyRes.byType[(int)DamageType::Cold]      = 0.00f;
    enemyRes.byType[(int)DamageType::Poison]    = 0.00f;
    enemyRes.byType[(int)DamageType::Radiation] = 0.15f;
    enemyRes.byType[(int)DamageType::Plasma]    = 0.05f;
    enemyRes.byType[(int)DamageType::DarkMatter]= -0.10f; // vulnerable
    enemyRes.byType[(int)DamageType::Cosmic]    = 0.20f;

    ship_.resistances = enemyRes;
    ship_.widthHeight = Util::getQuadWidthHeight(shipVerts, 6, {1, 1});

}


uint x = 0;

// process only spawned projectile collisions
void Renderer::updateCollision() {
    for (auto& bullet : bullets_) {
        if (!bullet.active) continue;

        for (uint i = 0; i < MAX_ALIENS; i++) {
            if (!aliens_[i].active) continue;

            // Player bullet hits alien
            if (isCollision(aliens_[i], bullet) && bullet.bulletType == BulletType::Ship) {
                bullet.active = false;

                // Queue a HitEvent for later DamageSystem processing
                eventBus_.publish(HitEvent{
                        .attacker   = /* optional: player entity id */ 0,
                        .target     = i, // or aliens_[i].entityId
                        .payload    = bullet.payload,
                        .hitWorldPos= glm::vec2(aliens_[i].x, aliens_[i].y)
                });

                // simple visual feedback
                alienPC_[i].flashAmount = 1.0f;

                // short particle burst on hit
                particleSystem_->spawn(glm::vec3(aliens_[i].x, -aliens_[i].y, 1.0f), 5);
                break;
            }

            // Alien bullet hits ship
            if (isCollision(aliens_[i], bullet) && bullet.bulletType == BulletType::Alien && !powerUpManager_->shieldActive) {

                bullet.active = false;

                eventBus_.publish(HitEvent{
                        .attacker   = i,  // alien index
                        .target     = ShipEntityId,
                        .payload    = bullet.payload,
                        .hitWorldPos= glm::vec2(ship_.x, ship_.y)
                });

                shipPC_.flashAmount = 1.0f;
                particleSystem_->spawn(glm::vec3(bullet.x, bullet.y, 0.0f), 10);
                break;
            }
        }
    }
}

void Renderer::spawnDamageText(const DamagePopupSpawned &damagePopupSpawned) {
    std::vector<Vertex> vertices = fontManager_->buildTextVertices(damagePopupSpawned.text, 0.0f, 0.0f, 1.0f, floatingDamageStartScale_);


    FloatingDamageInstance instance{};
    instance.text = damagePopupSpawned.text;
    instance.vertexOffset = 0;
    instance.vertexCount = 0;
    instance.basePos = {damagePopupSpawned.worldPos.x, -damagePopupSpawned.worldPos.y};
    instance.startTime = floatingDamageGlobalTime_;
    instance.lifetime = damagePopupSpawned.ttl;
    instance.riseSpeed = -damagePopupSpawned.riseSpeed;
    instance.startScale = damagePopupSpawned.startScale;
    instance.endScale = floatingDamageEndScale_;
    instance.fadeStart = 0.5f;
    instance.color = damagePopupSpawned.rgba;

    size_t instanceIndex = floatingDamageInstances_.size();
    floatingDamageInstances_.push_back(instance);

    PendingFloatingDamageUpload upload{};
    upload.instanceIndex = instanceIndex;
    upload.vertices = std::move(vertices);
    pendingFloatingDamageUploads_.push_back(std::move(upload));
}

void Renderer::updateFloatingDamage() {
    if (floatingDamageInstances_.empty()) {
        return;
    }

    const float currentTime = floatingDamageGlobalTime_;
    floatingDamageInstances_.erase(
            std::remove_if(floatingDamageInstances_.begin(),
                           floatingDamageInstances_.end(),
                           [currentTime](const FloatingDamageInstance &instance) {
                               return (currentTime - instance.startTime) >= instance.lifetime;
                           }),
            floatingDamageInstances_.end());
}

void Renderer::drawFloatingDamageTexts(VkCommandBuffer cmd) {
    if (floatingDamageInstances_.empty()) {
        return;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipeline_);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fontPipelineLayout_, 0, 1,
                            &fontDescriptorSet_, 0, nullptr);

    for (const auto &instance: floatingDamageInstances_) {
        FontPushConstants push{};
        push.pos = instance.basePos;
        push.currentTime = floatingDamageGlobalTime_;
        push.startTime = instance.startTime;
        push.lifetime = instance.lifetime;
        push.riseSpeed = instance.riseSpeed;
        push.startScale = instance.startScale;
        push.endScale = instance.endScale;
        push.fadeStart = instance.fadeStart;
        push.color = instance.color;

        vkCmdPushConstants(cmd, fontPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(FontPushConstants), &push);

        VkDeviceSize offset = instance.vertexOffset;
        vkCmdBindVertexBuffers(cmd, 0, 1, &fontVertexBuffer_, &offset);
        vkCmdDraw(cmd, instance.vertexCount, 1, 0, 0);
    }
}
void Renderer::animateScore() {
    int newScore = actualScore;
    int prevDisplay = static_cast<int>(displayedScore_);

    // Roll toward actualScore_
    if (displayedScore_ != newScore) {
        float diff = newScore - displayedScore_;
        float step = scoreAnimSpeed_ * GameTime::deltaTime;
        if (fabs(diff) < step)
            displayedScore_ = static_cast<float>(newScore);
        else
            displayedScore_ += (diff > 0 ? 1 : -1) * step;
    }

    int nowDisplay = static_cast<int>(displayedScore_);

    bool needsUpdate = false;

    // Only rebuild vertices if digit has changed!
    if (nowDisplay != prevDisplay) {
        scoreScale_ = scorePopAmount_;    // Trigger POP!
        scoreScaleTarget_ = 0.002f;       // Target scale
        needsUpdate = true;
    }

    // Animate the scale back to normal (damped spring)
    if (scoreScale_ != scoreScaleTarget_) {
        float delta = scoreScaleTarget_ - scoreScale_;
        float snap = scoreScaleSpeed_ * GameTime::deltaTime;
        if (fabs(delta) < 0.0001f)
            scoreScale_ = scoreScaleTarget_;
        else
            scoreScale_ += delta * snap;
        needsUpdate = true;
    }

    if (needsUpdate) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Score:%d", nowDisplay);
        scoreText_ = buf;
        std::vector<Vertex> scoreVertices = fontManager_->buildTextVertices(
                scoreText_, -0.95f, -0.80f, 1.0f, scoreScale_);
        allTextVertices[GameText::Score] = {fontVertexBuffer_, scoreVertices, scoreOffset_};
        updateFontBuffer(device_, scoreVertices, fontBufferMemory_, scoreOffset_);
        powerUpTextStartOffset_ = scoreOffset_ + scoreVertices.size() * sizeof(Vertex);
        powerUpTextCDOffset = std::max(powerUpTextCDOffset, powerUpTextStartOffset_);
    }
}


void Renderer::drawFrame() {

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageAvailableSemaphore_, VK_NULL_HANDLE,
                          &imageIndex);

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

void Renderer::updatePlayingLogic(float dt) {
    if (lastFireTime > rateOfFire) {
        lastFireTime = 0.0f;
        canFire = true;
    } else {
        lastFireTime += dt;
        canFire = false;
    }
    updateUniformBuffer();
    updateShip();

    updateAliens();
    updateCollision();
    if (mechanics_) {
        mechanics_->update(dt);
    }
    powerUpManager_->updatePowerUpData();
    powerUpManager_->checkIfPowerUpCollected(ship_);
}

void Renderer::prepareFrame(bool isPlaying) {
    animateScore();

    updateBullet();
    alienFireBullet();
    particleSystem_->updateHaloEffect(ship_);
    particleSystem_->updateStarField(starInstanceBufferMemory_);
    particleSystem_->updateExplosionParticles(particlesInstanceBufferMemory_);

    shakeOffset = {0.0f, 0.0f};
    if (shakeTimer > 0.0f) {
        shakeOffset.x = (rand() / (float) RAND_MAX - 0.5f) * 2.0f * shakeMagnitude;
        shakeOffset.y = (rand() / (float) RAND_MAX - 0.5f) * 2.0f * shakeMagnitude;
        shakeTimer -= GameTime::deltaTime;
    }
    shipPC_.flashAmount -= GameTime::deltaTime * 5.0f; // fade speed (0.2s)
    if (shipPC_.flashAmount < 0.0f) shipPC_.flashAmount = 0.0f;

    glm::vec2 offsetPos{0.75, -0.8f};

    VkDeviceSize powerUpWriteCursor = powerUpTextStartOffset_;
    for (auto &powerup: powerUpManager_->collectedPowerUps) {
        if (!powerup.second.active) {
            continue;
        }

        std::vector<Vertex> powerupVertices = fontManager_->buildTextVertices(
                std::to_string(powerup.second.expiryTime), 0.0, 0.0, 1.0f, 0.002f);

        powerup.second.textPos = offsetPos;
        if (isPlaying) {
            allTextVertices[powerup.first] = {fontVertexBuffer_, powerupVertices, powerUpWriteCursor};
            updateFontBuffer(device_, powerupVertices, fontBufferMemory_, powerUpWriteCursor);
        }
        powerUpWriteCursor += powerupVertices.size() * sizeof(Vertex);
        offsetPos += glm::vec2(0.0f, 0.05f);
    }

    powerUpTextCDOffset = std::max(powerUpTextStartOffset_, powerUpWriteCursor);

    if (!pendingFloatingDamageUploads_.empty()) {
        floatingDamageBufferCursor_ = std::max(floatingDamageBufferCursor_, powerUpTextCDOffset);
        for (auto &upload: pendingFloatingDamageUploads_) {
            VkDeviceSize allocationOffset = floatingDamageBufferCursor_;
            updateFontBuffer(device_, upload.vertices, fontBufferMemory_, allocationOffset);

            auto &instance = floatingDamageInstances_[upload.instanceIndex];
            instance.vertexOffset = allocationOffset;
            instance.vertexCount = static_cast<uint32_t>(upload.vertices.size());

            floatingDamageBufferCursor_ += upload.vertices.size() * sizeof(Vertex);
        }
        pendingFloatingDamageUploads_.clear();
    }

    floatingDamageBufferCursor_ = std::max(floatingDamageBufferCursor_, powerUpTextCDOffset);
    floatingDamageGlobalTime_ += GameTime::deltaTime;
    updateFloatingDamage();
}

void Renderer::loadText() {
    VkDeviceSize fontSize = 512 * 500 * 2;
    createBuffer(device_, physicalDevice_,
                 fontSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 fontVertexBuffer_, fontBufferMemory_, fontSize);

    // Title text
    std::vector<Vertex> titleVertices = fontManager_->buildTextVertices(
            "Space Endure v0.0.3", -0.95f, -0.85f, 0.0f, 0.002f);
    VkDeviceSize titleOffset = 0;
    updateFontBuffer(device_, titleVertices, fontBufferMemory_, titleOffset);
    allTextVertices[GameText::Title] = {fontVertexBuffer_, titleVertices, titleOffset};

    // Score text
    std::vector<Vertex> scoreVertices = fontManager_->buildTextVertices(
            "Score:0", -0.95f, -0.8f, 0.0f, 0.002f);
    scoreOffset_ = titleOffset + titleVertices.size() * sizeof(Vertex);
    updateFontBuffer(device_, scoreVertices, fontBufferMemory_, scoreOffset_);
    allTextVertices[GameText::Score] = {fontVertexBuffer_, scoreVertices, scoreOffset_};
    powerUpTextCDOffset = scoreOffset_ + scoreVertices.size() * sizeof(Vertex);
    powerUpTextStartOffset_ = powerUpTextCDOffset;
    floatingDamageBufferCursor_ = powerUpTextCDOffset;
    floatingDamageInstances_.clear();
    pendingFloatingDamageUploads_.clear();
    floatingDamageGlobalTime_ = 0.0f;

}

void Renderer::createAndUploadBuffer(const void *vertices, VkBuffer &buffer,
                                     VkDeviceMemory &bufferMemory, VkDeviceSize size,
                                     VkBufferUsageFlags usage) {
    createBuffer(device_, physicalDevice_, size, usage,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 buffer, bufferMemory);
    uploadDataBuffer(device_, vertices, size,
                     bufferMemory);
}

void Renderer::loadGameObjects() {
    createAndUploadBuffer(quadVerts, util_->vtxBuffer, util_->stagingBufferMemory,
                          sizeof(quadVerts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    createAndUploadBuffer(quadVerts, powerUpManager_->powerUpBuffer,
                          powerUpManager_->powerUpBufferMemory, sizeof(quadVerts),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    createAndUploadBuffer(starVerts, starVertsBuffer_, starVertsMemory_, sizeof(starVerts),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    createAndUploadBuffer(particlesIndices, starIndexBuffer_, starIndexMemory_,
                          sizeof(particlesIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    createAndUploadBuffer(starVerts, starInstanceBuffer_, starInstanceBufferMemory_,
                          sizeof(StarInstance) * NUM_STARS,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);


    createAndUploadBuffer(particleVerts, particlesVertexBuffer_, particlesVertexBufferMemory_,
                          sizeof(particleVerts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    createAndUploadBuffer(particlesIndices, particlesIndexBuffer_, particlesIndexBufferMemory_,
                          sizeof(particlesIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    createAndUploadBuffer(particleVerts, particlesInstanceBuffer_, particlesInstanceBufferMemory_,
                          sizeof(ParticleInstance) * MAX_PARTICLES,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);


    createAndUploadBuffer(quadVerts, bulletVertexBuffer_, bulletVertexBufferMemory_,
                          sizeof(quadVerts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    for (auto &bullet: bullets_) {
        bullet.active = false;
        bullet.widthHeight = Util::getQuadWidthHeight(quadVerts, 6, {0.2, 0.5});
    }

    createAndUploadBuffer(quadVerts, vertexBuffer_, vertexBufferMemory_, sizeof(quadVerts),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);


    createAndUploadBuffer(shipVerts, shipVertexBuffer_, shipVertexBufferMemory_, sizeof(shipVerts),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    createAndUploadBuffer(alienVerts, alienVertexBuffer_, alienVertexBufferMemory_,
                          sizeof(alienVerts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    createAndUploadBuffer(overlayQuadVerts, overlayVertexBuffer_, overlayVertexBufferMemory_,
                          sizeof(overlayQuadVerts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    createAndUploadBuffer(particleVerts, particleSystem_->haloVertexBuffer,
                          particleSystem_->haloVertexBufferMemory, sizeof(particleVerts),
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    createAndUploadBuffer(particlesIndices, particleSystem_->haloIndexBuffer,
                          particleSystem_->haloIndexBufferMemory, sizeof(particlesIndices),
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    createAndUploadBuffer(particleVerts, particleSystem_->haloInstanceBuffer,
                          particleSystem_->haloInstanceBufferMemory, sizeof(ShieldInstance) * 6,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

}

Renderer::~Renderer() {

    if (damagePopupSubscriptionId_ != 0) {
        eventBus_.unsubscribeDamagePopup(damagePopupSubscriptionId_);
        damagePopupSubscriptionId_ = 0;
    }

    if (sfxMixer_) {
        sfxMixer_->shutdown();
    }

    vkDestroySampler(device_, fontAtlasSampler_, nullptr);
    vkDestroyImageView(device_, fontAtlasImageView_, nullptr);
    vkFreeMemory(device_, fontAtlasImageDeviceMemory_, nullptr);
    vkDestroyImage(device_, fontAtlasImage_, nullptr);


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

    vkDestroyBuffer(device_, starVertsBuffer_, nullptr);
    vkFreeMemory(device_, starVertsMemory_, nullptr);
    vkDestroyBuffer(device_, starIndexBuffer_, nullptr);
    vkFreeMemory(device_, starIndexMemory_, nullptr);
    vkDestroyBuffer(device_, starInstanceBuffer_, nullptr);
    vkFreeMemory(device_, starInstanceBufferMemory_, nullptr);

    vkDestroyBuffer(device_, titleTextVertexBuffer_, nullptr);
    vkFreeMemory(device_, titleTextVertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, scoreTextVertexBuffer_, nullptr);
    vkFreeMemory(device_, scoreTextVertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, alienVertexBuffer_, nullptr);
    vkFreeMemory(device_, alienVertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, bulletVertexBuffer_, nullptr);
    vkFreeMemory(device_, bulletVertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, overlayVertexBuffer_, nullptr);
    vkFreeMemory(device_, overlayVertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, particlesVertexBuffer_, nullptr);
    vkFreeMemory(device_, particlesVertexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, particlesIndexBuffer_, nullptr);
    vkFreeMemory(device_, particlesIndexBufferMemory_, nullptr);

    vkDestroyBuffer(device_, particlesInstanceBuffer_, nullptr);
    vkFreeMemory(device_, particlesInstanceBufferMemory_, nullptr);

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

    if (uniformBufferMemory_ != VK_NULL_HANDLE) {
        if (uniformBuffersData) {
            vkUnmapMemory(device_, uniformBufferMemory_);
        }
        vkFreeMemory(device_, uniformBufferMemory_, nullptr);
    }

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


void Renderer::alienFireBullet() {
    if (gameState == GameState::Playing) {
        static float timer = 0.0f;
        timer += GameTime::deltaTime;
        if (timer > 1.0f) {
            timer = 0.0f;
            Alien alien = aliens_[Util::getRandomUint(0, MAX_ALIENS)];
            if (alien.active) {
                spawnBullet(BulletType::Alien, {alien.x, -alien.y});
            }
        }

    }
}

