@echo on
set "VULKAN_SDK=C:/VulkanSDK/1.4.335.0"
set "GLSLC=%VULKAN_SDK%/Bin/glslc.exe"

%GLSLC% main/main.vert -o ../../assets/shaders/main.vert.spv
%GLSLC% main/main.frag -o ../../assets/shaders/main.frag.spv

%GLSLC% overlay/overlay.vert -o ../../assets/shaders/overlay.vert.spv
%GLSLC% overlay/overlay.frag -o ../../assets/shaders/overlay.frag.spv

%GLSLC% font/font.vert -o ../../assets/shaders/font.vert.spv
%GLSLC% font/font.frag -o ../../assets/shaders/font.frag.spv

%GLSLC% particles/particles_instanced.vert -o ../../assets/shaders/particles_instanced.vert.spv
%GLSLC% particles/particles_instanced.frag -o ../../assets/shaders/particles_instanced.frag.spv

%GLSLC% particles/stars_instanced.vert -o ../../assets/shaders/stars_instanced.vert.spv
%GLSLC% particles/stars_instanced.frag -o ../../assets/shaders/stars_instanced.frag.spv

%GLSLC% particles/halo.vert -o ../../assets/shaders/halo.vert.spv
%GLSLC% particles/halo.frag -o ../../assets/shaders/halo.frag.spv

%GLSLC% aabb/aabb.vert -o ../../assets/shaders/aabb.vert.spv
%GLSLC% aabb/aabb.frag -o ../../assets/shaders/aabb.frag.spv
