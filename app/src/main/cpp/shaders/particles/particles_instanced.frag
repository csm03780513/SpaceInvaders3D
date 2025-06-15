
#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    float dist = length(fragUV - vec2(0.5));
    float alpha = smoothstep(0.5, 0.45, dist); // Soft-edged circle
    outColor = fragColor;
    outColor.a *= alpha;
}
