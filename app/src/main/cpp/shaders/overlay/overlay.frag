#version 450

layout(push_constant) uniform overlayPush {
    vec4 color;
} oc;

layout(set = 0, binding = 0) uniform sampler2D tex;
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;



void main() {
    // Replace the output temporarily to see if you get the alpha channel
//    outColor = vec4(texture(tex, fragUV).a, texture(tex, fragUV).a, texture(tex, fragUV).a, 1.0);
    // Or output all channels as a color
//    outColor = texture(tex.rgb, fragUV);
    vec4 texColor = texture(tex, fragUV);
    // See the alpha channel as red intensity
//    outColor = vec4(texColor.r, 0, 0, 1.0);
    // or just output alpha as color for debugging
//    outColor = vec4(fragUV,0.0,1.0);
    outColor = texColor * oc.color;
//    outColor = vec4(1,0,1,1);

}
