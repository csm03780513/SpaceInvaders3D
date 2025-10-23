#version 450

layout (push_constant) uniform FontPush {
    vec2 pos;
    float currentTime;
    float startTime;
    float lifetime;
    float riseSpeed;
    float startScale;
    float endScale;
    float fadeStart;
} fp;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outFragColor;
layout (location = 2) out float outFade;

void main() {
    float t = 0.0;
    if (fp.lifetime > 0.0) {
        t = clamp((fp.currentTime - fp.startTime) / fp.lifetime, 0.0, 1.0);
    }

    float scaleFactor = 1.0;
    if (fp.startScale > 0.0) {
        float targetScale = mix(fp.startScale, fp.endScale, t);
        scaleFactor = targetScale / fp.startScale;
    }
    vec2 scaledPos = inPos.xy * scaleFactor;
    float rise = fp.riseSpeed * t;
    vec2 worldPos = scaledPos + fp.pos + vec2(0.0, rise);

    gl_Position = vec4(worldPos, 0.0, 1.0);

    float fade = 1.0;
    if (fp.lifetime > 0.0) {
        float fadeSpan = max(0.0001, 1.0 - fp.fadeStart);
        float fadeProgress = clamp((t - fp.fadeStart) / fadeSpan, 0.0, 1.0);
        fade = 1.0 - fadeProgress;
    }

    outUV = inUV;
    outFragColor = inColor;
    outFade = fade;
}
