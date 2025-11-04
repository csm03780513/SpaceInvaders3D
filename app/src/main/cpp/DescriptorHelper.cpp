#include "DescriptorHelper.h"

#include <stdexcept>

DescriptorSetupResult createDescriptorResources(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding> &bindings,
        const std::vector<VkDescriptorPoolSize> &poolSizes,
        uint32_t descriptorSetCount,
        const std::vector<VkPushConstantRange> &pushConstants,
        GfxPipelineData &gfxPipelineData,
        const std::vector<VkWriteDescriptorSet> &descriptorWrites) {
    DescriptorSetupResult result{};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &result.layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = descriptorSetCount;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &result.pool) != VK_SUCCESS) {
        vkDestroyDescriptorSetLayout(device, result.layout, nullptr);
        throw std::runtime_error("Failed to create descriptor pool");
    }

    std::vector<VkDescriptorSetLayout> setLayouts(descriptorSetCount, result.layout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = result.pool;
    allocInfo.descriptorSetCount = descriptorSetCount;
    allocInfo.pSetLayouts = setLayouts.data();

    std::vector<VkDescriptorSet> descriptorSets(descriptorSetCount);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        vkDestroyDescriptorPool(device, result.pool, nullptr);
        vkDestroyDescriptorSetLayout(device, result.layout, nullptr);
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    result.descriptorSet = descriptorSets.empty() ? VK_NULL_HANDLE : descriptorSets.front();

    std::vector<VkWriteDescriptorSet> writes = descriptorWrites;
    for (auto &write: writes) {
        write.dstSet = result.descriptorSet;
    }

    if (!writes.empty()) {
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &gfxPipelineData.pipelineLayout) != VK_SUCCESS) {
        vkDestroyDescriptorPool(device, result.pool, nullptr);
        vkDestroyDescriptorSetLayout(device, result.layout, nullptr);
        throw std::runtime_error("Failed to create pipeline layout");
    }

    gfxPipelineData.pushConstantRanges = pushConstants;

    return result;
}
