#version 450

layout(push_constant) uniform overlayPush {
    vec4 color;
} oc;

layout(location = 0) out vec4 outColor;


void main() {
    outColor = oc.color;
}
