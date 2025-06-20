#version 450

layout(location = 0) in float vBrightness;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(vec3(vBrightness), 1.0);
}
