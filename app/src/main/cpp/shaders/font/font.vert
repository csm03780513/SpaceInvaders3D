#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outFragColor;

void main() {

    gl_Position = vec4(inPos.xy,0.0, 1.0);
    //    gl_Position = vec4(inPos, 0.0, 1.0);
    outUV = inUV;
    outFragColor = inColor;


}