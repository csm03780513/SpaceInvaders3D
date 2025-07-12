#version 450

layout (location = 0) in vec2 fragUV;
layout (location = 1) in vec4 fragColor;
layout (location = 2) in float vTime;
layout (location = 3) in float vEffectType;

layout (location = 0) out vec4 outColor;

void main() {
    // Example: if effectType == 1.0, render as glowing halo ring
    if (vEffectType == 1.0) {
        //        vec2 uv = fragUV * 2.0 - 1.0; // [-1,1]
        //        float dist = length(uv);
        //        float ring = smoothstep(0.85, 1.0, dist) - smoothstep(1.0, 1.15, dist);
        //        float pulse = 0.8 + 0.2 * sin(vTime * 6.0);
        //        float alpha = ring * fragColor.a * pulse;
        //        vec3 color = mix(vec3(1.0,1.0,1.0), fragColor.rgb, 0.8);
        //        outColor = vec4(color, alpha);

        vec2 uv = fragUV * 2.0 - 1.0;
        float dist = length(uv);

        float outer = smoothstep(0.22, 1.00, dist);
        float inner = smoothstep(0.10, 0.70, dist);
        float band = outer - inner;
        outColor = vec4(0.2, 0.8, 1.0, band); // Bluish ring


        //        if (dist < 0.65) discard; // Optional: hollow center for true ring
    } else {
        // Default: just render the colored quad
        outColor = fragColor;
    }
}
