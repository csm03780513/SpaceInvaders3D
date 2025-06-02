#version 450

layout(set = 0, binding = 1) uniform sampler2D tex;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;

void main() {
//    outColor = vec4(fragColor);
//    outColor = vec4(inUV,1.0f,1.0f);
//      outColor = texture(tex, inUV) * fragColor;

      vec4 texColor = texture(tex, inUV);
      outColor = texColor;

}

