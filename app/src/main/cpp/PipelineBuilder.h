#pragma once

#include <functional>
#include <string>
#include <vector>

#include "GameObjectData.h"
#include "platform/PlatformServices.h"

class Renderer;

class PipelineBuilder {
public:
    PipelineBuilder(VkDevice device,
                    VkRenderPass renderPass,
                    VkExtent2D swapchainExtent,
                    IPlatformServices &platformServices);

    void setShaderFilenames(std::string vertexShader, std::string fragmentShader);

    void setVertexInput(const std::vector<VkVertexInputBindingDescription> &bindings,
                        const std::vector<VkVertexInputAttributeDescription> &attributes);

    void setTopology(VkPrimitiveTopology topology);

    void setDescriptorLayoutCallback(std::function<void(GfxPipelineData &)> callback);

    void setPushConstantRanges(const std::vector<VkPushConstantRange> &ranges);

    PipelineHandles build(Renderer &renderer);

private:
    VkDevice device_;
    VkRenderPass renderPass_;
    VkExtent2D extent_;
    IPlatformServices &platformServices_;

    GfxPipelineData pipelineData_{};
    std::vector<VkVertexInputBindingDescription> bindingDescriptions_;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions_;
    std::vector<VkPushConstantRange> pushConstantRanges_;
    std::function<void(GfxPipelineData &)> descriptorCallback_{};
    std::string vertexShaderFilename_;
    std::string fragmentShaderFilename_;
    VkPrimitiveTopology topology_{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

    void initializeFixedFunctionStates();
};
