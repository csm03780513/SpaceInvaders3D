#include "PipelineBuilder.h"

#include <stdexcept>
#include <utility>

#include "Renderer.h"

void setShaderStages(VkDevice device, IPlatformServices &platform, const char *spirvVertexFilename,
                     const char *spirvFragmentFilename, GfxPipelineData &graphicsPipelineData);
void setColorBlending(GfxPipelineData &graphicsPipelineData);
void setViewPortState(GfxPipelineData &graphicsPipelineData);
void setInputAssembly(GfxPipelineData &graphicsPipelineData);
void setRasterizer(GfxPipelineData &graphicsPipelineData);
void setSampling(GfxPipelineData &graphicsPipelineData);

PipelineBuilder::PipelineBuilder(VkDevice device,
                                 VkRenderPass renderPass,
                                 VkExtent2D swapchainExtent,
                                 IPlatformServices &platformServices)
        : device_(device),
          renderPass_(renderPass),
          extent_(swapchainExtent),
          platformServices_(platformServices) {
    initializeFixedFunctionStates();
}

void PipelineBuilder::setShaderFilenames(std::string vertexShader, std::string fragmentShader) {
    vertexShaderFilename_ = std::move(vertexShader);
    fragmentShaderFilename_ = std::move(fragmentShader);
}

void PipelineBuilder::setVertexInput(const std::vector<VkVertexInputBindingDescription> &bindings,
                                     const std::vector<VkVertexInputAttributeDescription> &attributes) {
    bindingDescriptions_ = bindings;
    attributeDescriptions_ = attributes;
}

void PipelineBuilder::setTopology(VkPrimitiveTopology topology) {
    topology_ = topology;
}

void PipelineBuilder::setDescriptorLayoutCallback(std::function<void(GfxPipelineData &)> callback) {
    descriptorCallback_ = std::move(callback);
}

void PipelineBuilder::setPushConstantRanges(const std::vector<VkPushConstantRange> &ranges) {
    pushConstantRanges_ = ranges;
}

PipelineHandles PipelineBuilder::build(Renderer &renderer) {
    if (vertexShaderFilename_.empty() || fragmentShaderFilename_.empty()) {
        throw std::runtime_error("PipelineBuilder requires both vertex and fragment shaders to be set");
    }

    setShaderStages(device_, platformServices_, vertexShaderFilename_.c_str(),
                    fragmentShaderFilename_.c_str(), pipelineData_);

    std::vector<VkShaderModule> shaderModules;
    shaderModules.reserve(pipelineData_.shaderStages.size());
    for (const auto &stage: pipelineData_.shaderStages) {
        shaderModules.push_back(stage.module);
    }

    pipelineData_.vertexInputState = {};
    pipelineData_.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineData_.vertexInputState.vertexBindingDescriptionCount = bindingDescriptions_.size();
    pipelineData_.vertexInputState.pVertexBindingDescriptions = bindingDescriptions_.data();
    pipelineData_.vertexInputState.vertexAttributeDescriptionCount = attributeDescriptions_.size();
    pipelineData_.vertexInputState.pVertexAttributeDescriptions = attributeDescriptions_.data();

    pipelineData_.inputAssemblyState.topology = topology_;
    pipelineData_.pushConstantRanges = pushConstantRanges_;

    if (descriptorCallback_) {
        descriptorCallback_(pipelineData_);
    }

    pipelineData_.pipelineCreateInfo = {};

    PipelineHandles handles;

    try {
        handles = renderer.createPipeline(pipelineData_);
    } catch (...) {
        for (auto module: shaderModules) {
            vkDestroyShaderModule(device_, module, nullptr);
        }
        throw;
    }

    for (auto module: shaderModules) {
        vkDestroyShaderModule(device_, module, nullptr);
    }

    return handles;
}

void PipelineBuilder::initializeFixedFunctionStates() {
    pipelineData_.viewport = {0.0f, 0.0f, static_cast<float>(extent_.width),
                              static_cast<float>(extent_.height), 0.0f, 1.0f};
    pipelineData_.scissor = {{0, 0}, extent_};
    pipelineData_.renderPass = renderPass_;
    pipelineData_.subpass = 0;

    setColorBlending(pipelineData_);
    setViewPortState(pipelineData_);
    setInputAssembly(pipelineData_);
    setRasterizer(pipelineData_);
    setSampling(pipelineData_);
}
