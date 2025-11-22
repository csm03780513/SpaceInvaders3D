#version 450

layout(push_constant) uniform uiPush {
    uint texturePos;
    vec2 offset;
    vec2 scale;
} up;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out uint outTexturePos;

void main() {
    fragUV = inUV;
    gl_Position = vec4(inPos.xy + up.offset, 0.0, 1.0);
    outTexturePos = up.texturePos;

}
