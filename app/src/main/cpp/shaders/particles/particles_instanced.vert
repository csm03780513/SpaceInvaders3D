#version 450

// Per-vertex (quad)
layout(location = 0) in vec2 quadPos;

// Per-instance
layout(location = 1) in vec2 inCenter;
layout(location = 2) in float inSize;
layout(location = 3) in vec4 inColor;

// If you want rotation:
// layout(location = 4) in float inRotation;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main() {
    fragUV = quadPos + 0.5; // [0,1] UVs
    fragColor = inColor;

    // Optional: Rotation
    // float c = cos(inRotation);
    // float s = sin(inRotation);
    // mat2 rot = mat2(c, -s, s, c);
    // vec2 pos = inCenter + (rot * quadPos) * inSize;

    vec2 pos = inCenter + quadPos * inSize;
    gl_Position = vec4(pos, 0.0, 1.0);
}

