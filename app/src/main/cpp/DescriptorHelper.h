#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "GameObjectData.h"

struct DescriptorSetupResult {
    VkDescriptorSetLayout layout{VK_NULL_HANDLE};
    VkDescriptorPool pool{VK_NULL_HANDLE};
    VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
};

DescriptorSetupResult createDescriptorResources(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding> &bindings,
        const std::vector<VkDescriptorPoolSize> &poolSizes,
        uint32_t descriptorSetCount,
        const std::vector<VkPushConstantRange> &pushConstants,
        GfxPipelineData &gfxPipelineData,
        const std::vector<VkWriteDescriptorSet> &descriptorWrites);
