#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform AlienPush {
    vec2 offset;
} pc;


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPos.xy + pc.offset, inPos.z, 1.0);
//    gl_Position = vec4(inPos, 0.0, 1.0);
    fragColor = vec3(inColor.xy + pc.offset,inColor.z);
}