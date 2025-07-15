#version 450

layout (set = 0, binding = 1) uniform sampler2D textures[5];

layout (location = 0) in vec4 fragColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in float inFlashAmount;
layout (location = 3) in flat uint inTexturePos;
layout (location = 4) in float inTime;
layout (location = 5) in flat uint inCanPulse;

layout (location = 0) out vec4 outColor;

void main() {

    float alpha = 1.0f;
    if (inCanPulse == 1) {
        alpha = 0.8 + 0.2 * sin(inTime * 9.0);
    }

    vec4 texColor = texture(textures[inTexturePos], inUV);
    vec3 color = mix(texColor.rgb, vec3(1.0), clamp(inFlashAmount, 0.0f, 1.0f));
    outColor = vec4(color, texColor.a * alpha);
}

