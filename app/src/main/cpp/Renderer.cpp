#include "Renderer.h"
#include <android/native_window.h>
#include <android/log.h>
#include <vector>

std::vector<char> loadAsset(AAssetManager *mgr, const char *filename);

VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

std::array<float, 16> makePerspective(float fovY, float aspect, float zNear, float zFar);

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory);

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Vulkan", __VA_ARGS__)

struct Vertex {
    float pos[3];
    float color[3];
};

struct Bullet {
    float x, y;
    bool active;
};

// Relative to (0, 0); will offset per-bullet in the shader or CPU
static const Vertex bulletVerts[6] = {
        // First triangle
        { { -0.01f,  -0.05f, -1.0f }, { 1, 1, 0 } },
        { {  0.01f,  -0.05f, -1.0f }, { 1, 1, 0 } },
        { { -0.01f,  0.00f, -1.0f }, { 1, 1, 0 } },
        // Second triangle
        { {  0.01f,  -0.05f, -1.0f }, { 1, 1, 0 } },
        { {  0.01f,  0.00f, -1.0f }, { 1, 1, 0 } },
        { { -0.01f,  0.00f, -1.0f }, { 1, 1, 0 } }
};

static constexpr int MAX_BULLETS = 32;
Bullet bullets_[MAX_BULLETS] = {};


static Vertex triangleVerts[3] = {
        {{0.0f,  -0.5f, -1.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f,  0.5f,  -1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f,  -1.0f}, {0.0f, 0.0f, 1.0f}}
};

// Ship is a rectangle at y = -0.8f, width = 0.2, height = 0.05 (adjust as you like)
static Vertex shipVerts[6] = {
        // First triangle
        { { 0.1f, 0.85f, 1.0f }, { 1.0f, 1.0f, 1.0f } }, // bottom left (white)
        { {  -0.1f, 0.85f, 1.0f }, { 0.0f, 1.0f, 1.0f } }, // bottom right (cyan)
        { { 0.1f, 0.8f,  1.0f }, { 1.0f, 0.0f, 1.0f } }, // top left (magenta)
        // Second triangle
        { {  -0.1f, 0.85f, 1.0f }, { 0.0f, 1.0f, 1.0f } }, // bottom right
        { {  -0.1f, 0.8f,  1.0f }, { 0.0f, 0.0f, 1.0f } }, // top right (blue)
        { { 0.1f, 0.8f,  1.0f }, { 1.0f, 0.0f, 1.0f } }  // top left
};

std::array<float, 16> makePerspective(float fovY, float aspect, float zNear, float zFar) {
    float f = 1.0f / std::tan(fovY / 2.0f);
    std::array<float, 16> m = {};
    m[0] = f / aspect;
    m[5] = -f; // NEGATIVE to flip Y for Vulkan's NDC
    m[10] = (zFar + zNear) / (zNear - zFar);
    m[11] = -1.0f;
    m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    return m;
}




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

std::vector<char> loadAsset(AAssetManager *mgr, const char *filename) {
    AAsset *asset = AAssetManager_open(mgr, filename, AASSET_MODE_STREAMING);
    size_t size = AAsset_getLength(asset);
    std::vector<char> buffer(size);
    AAsset_read(asset, buffer.data(), size);
    AAsset_close(asset);
    return buffer;
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

Renderer::Renderer(android_app *app) : app_(app) {
    initVulkan();


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

    AAssetManager *mgr = app_->activity->assetManager;

    auto vertShaderCode = loadAsset(mgr, "triangle.vert.spv");
    auto fragShaderCode = loadAsset(mgr, "triangle.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(device_, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(device_, fragShaderCode);


    VkDeviceSize bulletBufferSize = sizeof(bulletVerts);
    createBuffer(device_, physicalDevice_, bulletBufferSize,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 bulletVertexBuffer_, bulletVertexBufferMemory_);
    void* bulletData;
    vkMapMemory(device_, bulletVertexBufferMemory_, 0, bulletBufferSize, 0, &bulletData);
    memcpy(bulletData, bulletVerts, bulletBufferSize);
    vkUnmapMemory(device_, bulletVertexBufferMemory_);


    VkDeviceSize bufferSize = sizeof(triangleVerts);
    createBuffer(
            device_,
            physicalDevice_,
            bufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertexBuffer_,
            vertexBufferMemory_
    );
    // Map and copy vertex data
    void* data;
    vkMapMemory(device_, vertexBufferMemory_, 0, bufferSize, 0, &data);
    memcpy(data, triangleVerts, (size_t)bufferSize);
    vkUnmapMemory(device_, vertexBufferMemory_);

    VkDeviceSize shipBufferSize = sizeof(shipVerts);
    createBuffer(
            device_,
            physicalDevice_,
            shipBufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            shipVertexBuffer_,
            shipVertexBufferMemory_
    );

// Map/copy ship vertex data
    void* shipData;
    vkMapMemory(device_, shipVertexBufferMemory_, 0, shipBufferSize, 0, &shipData);
    memcpy(shipData, shipVerts, (size_t)shipBufferSize);
    vkUnmapMemory(device_, shipVertexBufferMemory_);


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

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // One binding (per-vertex data)
    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

// Position (location=0)
    VkVertexInputAttributeDescription attrDesc0 = {};
    attrDesc0.binding = 0;
    attrDesc0.location = 0;
    attrDesc0.format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc0.offset = offsetof(Vertex, pos);

// Color (location=1)
    VkVertexInputAttributeDescription attrDesc1 = {};
    attrDesc1.binding = 0;
    attrDesc1.location = 1;
    attrDesc1.format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc1.offset = offsetof(Vertex, color);

    VkVertexInputAttributeDescription attrDescs[] = { attrDesc0, attrDesc1 };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchainExtent_.width;
    viewport.height = (float) swapchainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;


    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;

    vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_);

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_) != VK_SUCCESS) {
        LOGE("Failed to create graphics pipeline!");
        abort();
    }
    vkDestroyShaderModule(device_, vertShaderModule, nullptr);
    vkDestroyShaderModule(device_, fragShaderModule, nullptr);

    VkDeviceSize uboSize = sizeof(float) * 16;
    createBuffer(device_, physicalDevice_, uboSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffer_, uniformBufferMemory_);

    float aspect = (float) swapchainExtent_.width / (float) swapchainExtent_.height;
    auto proj = makePerspective(3.14159265f / 4.0f, aspect, 0.1f, 10.0f);

    void* data1;
    vkMapMemory(device_, uniformBufferMemory_, 0, uboSize, 0, &data1);
    memcpy(data1, proj.data(), uboSize);
    vkUnmapMemory(device_, uniformBufferMemory_);

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descPoolInfo = {};
    descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolInfo.poolSizeCount = 1;
    descPoolInfo.pPoolSizes = &poolSize;
    descPoolInfo.maxSets = 1;

    LOGE("About to create descriptor pool");
    VkResult res = vkCreateDescriptorPool(device_, &descPoolInfo, nullptr, &descriptorPool_);
    LOGE("vkCreateDescriptorPool returned %d", res);
    if (res != VK_SUCCESS) {
        LOGE("Failed to create descriptor pool: %d", res);
        abort();
    }
    LOGE("Descriptor pool created");
    VkDescriptorSetAllocateInfo descAllocInfo = {};
    descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAllocInfo.descriptorPool = descriptorPool_;
    descAllocInfo.descriptorSetCount = 1;
    descAllocInfo.pSetLayouts = &descriptorSetLayout_;
    LOGE("About to create descriptor sets");
    VkResult res1 = vkAllocateDescriptorSets(device_, &descAllocInfo, &descriptorSet_);
    if (res1 != VK_SUCCESS) {
        LOGE("Failed to create descriptor set: %d", res);
        abort();
    }
    LOGE("Descriptor set created");
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniformBuffer_;
    bufferInfo.offset = 0;
    bufferInfo.range = uboSize;

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet_;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);

    // Continue: pick physical device, create logical device, swapchain, etc.
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBuffer cmd = commandBuffers_[imageIndex];

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkClearValue clearColor = { { 0.1f, 0.2f, 0.3f, 1.0f } };
    VkRenderPassBeginInfo renderBeginPassInfo = {};
    renderBeginPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBeginPassInfo.renderPass = renderPass_;
    renderBeginPassInfo.framebuffer = framebuffers_[imageIndex];
    renderBeginPassInfo.renderArea.offset = {0, 0};
    renderBeginPassInfo.renderArea.extent = swapchainExtent_;
    renderBeginPassInfo.clearValueCount = 1;
    renderBeginPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderBeginPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // --- Draw triangle (or any background)
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer_, offsets);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_, 0, 1, &descriptorSet_, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    // --- Draw ship
    vkCmdBindVertexBuffers(cmd, 0, 1, &shipVertexBuffer_, offsets);
    vkCmdDraw(cmd, 6, 1, 0, 0);

    // --- Draw bullets (for each active bullet, update buffer and draw)
    for (int b = 0; b < MAX_BULLETS; ++b) {
        if (!bullets_[b].active) continue;
        Vertex movedBullet[6];
        for (int j = 0; j < 6; ++j) {
            movedBullet[j] = bulletVerts[j];
            movedBullet[j].pos[0] += bullets_[b].x;
            movedBullet[j].pos[1] -= bullets_[b].y;
        }
        void* bulletBufData;
        vkMapMemory(device_, bulletVertexBufferMemory_, 0, sizeof(movedBullet), 0, &bulletBufData);
        memcpy(bulletBufData, movedBullet, sizeof(movedBullet));
        vkUnmapMemory(device_, bulletVertexBufferMemory_);

        vkCmdBindVertexBuffers(cmd, 0, 1, &bulletVertexBuffer_, offsets);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

void Renderer::spawnBullet() {
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
    static Vertex baseShipVerts[6] = {
            // First triangle
            { { 0.1f, 0.85f, 1.0f }, { 1.0f, 1.0f, 1.0f } }, // bottom left (white)
            { {  -0.1f, 0.85f, 1.0f }, { 0.0f, 1.0f, 1.0f } }, // bottom right (cyan)
            { { 0.1f, 0.8f,  1.0f }, { 1.0f, 0.0f, 1.0f } }, // top left (magenta)
            // Second triangle
            { {  -0.1f, 0.85f, 1.0f }, { 0.0f, 1.0f, 1.0f } }, // bottom right
            { {  -0.1f, 0.8f,  1.0f }, { 0.0f, 0.0f, 1.0f } }, // top right (blue)
            { { 0.1f, 0.8f,  1.0f }, { 1.0f, 0.0f, 1.0f } }  // top left
    };

    Vertex movedVerts[6];
    for (int i = 0; i < 6; ++i) {
        movedVerts[i] = baseShipVerts[i];
        movedVerts[i].pos[0] += shipX_; // Move horizontally
        movedVerts[i].color[1] += shipX_;
    }

    void* data;
    vkMapMemory(device_, shipVertexBufferMemory_, 0, sizeof(movedVerts), 0, &data);
    memcpy(data, movedVerts, sizeof(movedVerts));
    vkUnmapMemory(device_, shipVertexBufferMemory_);
}
void Renderer::updateBullet() {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets_[i].active) {
            bullets_[i].y += 0.01f; // Move up
            LOGE("bullet at y:%f", bullets_[i].y);
            if (bullets_[i].y > 1.0f)
                bullets_[i].active = false; // Off screen
        }
    }

}
void Renderer::drawFrame() {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageAvailableSemaphore_, VK_NULL_HANDLE, &imageIndex);
    updateShipBuffer();
    updateBullet();
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
    vkDestroyBuffer(device_, bulletVertexBuffer_, nullptr);
    vkFreeMemory(device_, bulletVertexBufferMemory_, nullptr);
    if (shipVertexBuffer_ != VK_NULL_HANDLE)
        vkDestroyBuffer(device_, shipVertexBuffer_, nullptr);
    if (shipVertexBufferMemory_ != VK_NULL_HANDLE)
        vkFreeMemory(device_, shipVertexBufferMemory_, nullptr);
    if (imageAvailableSemaphore_ != VK_NULL_HANDLE)
        vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);
    if (renderFinishedSemaphore_ != VK_NULL_HANDLE)
        vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
    if (descriptorPool_ != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    if (uniformBuffer_ != VK_NULL_HANDLE)
        vkDestroyBuffer(device_, uniformBuffer_, nullptr);
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


