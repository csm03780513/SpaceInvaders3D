#version 450

layout(location = 0) in vec3 quadPos;       // Per-vertex quad position (from Vertex struct)
layout(location = 1) in vec2 inCenter;      // Per-instance: effect center
layout(location = 2) in float inSize;       // Per-instance: effect size
layout(location = 3) in vec4 inColor;       // Per-instance: color
layout(location = 4) in float inTime;       // Per-instance: time (for pulse/anim)
layout(location = 5) in float inEffectType; // Per-instance: effect type flag

layout(location = 0) out vec2 fragUV;       // Pass to fragment shader for halo math
layout(location = 1) out vec4 fragColor;
layout(location = 2) out float vTime;
layout(location = 3) out float vEffectType;

void main() {
    // Scale quad from [-0.5,0.5] to effect size, and move to center
    vec2 pos = quadPos.xy * inSize + inCenter;
    fragUV = quadPos.xy * 0.5 + 0.5; // Map to [0,1] for ring/circle logic
    fragColor = inColor;
    vTime = inTime;
    vEffectType = inEffectType;

    gl_Position = vec4(pos, 0.0, 1.0);
}
