#version 450

layout(set = 0, binding = 0) uniform sampler2D fontAtlas;

layout(push_constant) uniform customColor {
  vec4 color;
}cc;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 glyph = texture(fontAtlas, fragUV);
//vec4 color = vec4(fragUV,0.0f,1.0f);
//    if (color.a < 0.1) discard;
//    outColor = color;

    // Option 1: Use glyph as alpha for your color
    outColor = vec4(fragColor.rgb, fragColor.a * glyph);
    // Option 2: Output white with glyph as alpha
//     outColor = vec4(1.0, 1.0, 1.0, glyph);
    // Option 3: Color only where glyph > threshold (for binary font)
    // outColor = textColor * step(0.5, glyph);

}
