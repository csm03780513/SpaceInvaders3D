#version 450



layout(set = 0, binding = 0) uniform sampler2D textures[5];
layout(location = 0) in vec2 fragUV;
layout(location = 1) in flat uint inTexturePos;
layout(location = 2) in float inHpRatio;

layout(location = 0) out vec4 outColor;



void main() {
    // Replace the output temporarily to see if you get the alpha channel
//    outColor = vec4(texture(tex, fragUV).a, texture(tex, fragUV).a, texture(tex, fragUV).a, 1.0);
    // Or output all channels as a color
//    outColor = texture(tex.rgb, fragUV);
    if (fragUV.x > inHpRatio) discard;
    vec4 texColor = texture(textures[inTexturePos], fragUV);
    // See the alpha channel as red intensity
//    outColor = vec4(texColor.r, 0, 0, 1.0);
    // or just output alpha as color for debugging
//    outColor = vec4(fragUV,0.0,1.0);
    outColor = texColor;
//    outColor = vec4(1,0,1,1);

}
