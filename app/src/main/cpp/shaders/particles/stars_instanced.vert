#version 450

// Star quad: pos = [-0.5,-0.5] to [0.5,0.5], instanced
layout(location = 0) in vec3 quadPos;       // per-vertex
layout(location = 1) in vec3 inStarPos;     // per-instance
layout(location = 2) in float inStarSize;   // per-instance
layout(location = 3) in float inStarBrightness; // per-instance

layout(location = 0) out float vStarDist;
layout(location = 1) out float vBrightness;
layout(location = 2) out vec3 outQuadPos;

void main() {
//    vec3 pos = inStarPos + quadPos * inStarSize;
//    gl_Position = vec4(pos, 1.0);
//    vBrightness = inStarBrightness;

    vec3 starCenter = inStarPos;
    float size = inStarSize;
    vec3 worldPos = starCenter + quadPos * size;
    gl_Position = vec4(worldPos, 1.0);

    vStarDist = length(quadPos.xy); // 0 at center, ~0.7 at corner
    vBrightness = inStarBrightness;
    outQuadPos = quadPos;
}
