#version 450

// Star quad: pos = [-0.5,-0.5] to [0.5,0.5], instanced
layout(location = 0) in vec3 quadPos;       // per-vertex
layout(location = 1) in vec3 inStarPos;     // per-instance
layout(location = 2) in float inStarSize;   // per-instance
layout(location = 3) in float inStarBrightness; // per-instance

layout(location = 0) out float vBrightness;

void main() {
    vec3 pos = inStarPos + quadPos * inStarSize;
    gl_Position = vec4(pos, 1.0);
    vBrightness = inStarBrightness;
}
