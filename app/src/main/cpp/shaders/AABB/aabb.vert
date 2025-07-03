// debug_line.vert
#version 450
layout(location = 0) in vec3 inPos;   // NDC (-1..1)
layout(location = 1) in vec3 inColor; // RGB

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPos.xy, 0.0, 1.0);
    fragColor = inColor;
}
