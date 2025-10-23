#version 450

layout(set = 0, binding = 0) uniform sampler2D fontAtlas;

layout(push_constant) uniform customColor {
  vec4 color;
}cc;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in float fragFade;

layout(location = 0) out vec4 outColor;

void main() {
    float glyphAlpha = texture(fontAtlas, fragUV).r;
    float alpha = fragColor.a * glyphAlpha * fragFade;
    if (alpha <= 0.01) {
        discard;
    }
    outColor = vec4(fragColor.rgb, alpha);
}
