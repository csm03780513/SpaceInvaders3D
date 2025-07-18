#version 450

layout (set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (push_constant) uniform AlienPush {
    vec2 offset;
    vec2 shakeOffset;
    float flashAmount;
    uint texturePos;
    float deltaTime;
    uint enablePulse;
    vec2 size;
} pc;


layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out float outFlashAmount;
layout (location = 3) out uint outTexturePos;
layout (location = 4) out float outTime;
layout (location = 5) out uint outCanPulse;

void main() {

    gl_Position = vec4((inPos.xy * pc.size) + pc.offset , inPos.z, 1.0);
    gl_Position.xy += pc.shakeOffset; // shifts everything
    //    gl_Position = vec4(inPos, 0.0, 1.0);
    fragColor = vec4(inColor.xy, inColor.z, 1.0);
    outUV = inUV;
    outFlashAmount = pc.flashAmount;
    outTexturePos = pc.texturePos;
    outTime = pc.deltaTime;
    outCanPulse = pc.enablePulse;
}
