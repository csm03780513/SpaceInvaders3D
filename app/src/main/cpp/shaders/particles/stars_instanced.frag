#version 450

layout(location = 0) in float vStarDist;
layout(location = 1) in float vBrightness;
layout(location = 2) in vec3 quadPos;
layout(location = 0) out vec4 outColor;

void main() {
//    outColor = vec4(vec3(vBrightness), 1.0);

//    float alpha = smoothstep(0.0, 0.5, vStarDist); // soft round
//    outColor = vec4(vec3(vBrightness), alpha);

//outColor = vec4(vStarDist, vStarDist, vStarDist, 1.0);
float dist = length(quadPos.xy);           // Use only X/Y for distance!
float alpha = smoothstep(0.4, 0.0, dist);  // 1 at center, 0 at edge (soft round)
outColor = vec4(vec3(vBrightness), alpha);     // Or use brightness if you like
}
