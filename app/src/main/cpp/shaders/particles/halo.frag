#version 450

layout (location = 0) in vec2 fragUV;
layout (location = 1) in vec4 fragColor;
layout (location = 2) in float vTime;
layout (location = 3) in float vEffectType;

layout (location = 0) out vec4 outColor;

void main() {
    // Example: if effectType == 1.0, render as glowing halo ring
    if (vEffectType == 1.0) {
        vec2 uv = fragUV * 2.0 - 1.0;
        float dist = length(uv);
        float inner = smoothstep(0.0, 0.6, dist);
        float outer = smoothstep(0.45, 0.5, dist);
        float ring = inner - outer;
        float pulse = 0.8 + 0.2 * sin(vTime * 3.0);
        float alpha = ring * fragColor.a * pulse;
        //vec3 color = mix(vec3(1.0,1.0,1.0), fragColor.rgb, 0.8);

        // --- Rainbow ---
        float hue = mod(vTime * 0.03, 1.0);
        float s = 1.0;
        float v = 1.0;
        vec3 k = vec3(1.0, 2.0/3.0, 1.0/3.0);
        vec3 p = abs(fract(hue + k) * 6.0 - 3.0);
        vec3 rgb = v * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), s);

        outColor = vec4(rgb, alpha);
//        outColor = vec4(color, alpha);

    } else {
        // Default: just render the colored quad
        outColor = fragColor;
    }
}
