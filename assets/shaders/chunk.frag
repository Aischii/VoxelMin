#version 330 core

in vec3 vNormal;
in vec2 vUV;
in vec2 vTileMin;
in vec2 vTileSize;
in vec3 vWorldPos;
in float vAO;
in float vLight;
in float vTorchLight;

uniform sampler2D uAtlas;
uniform vec3 uCamPos;
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform float uSunlight;
uniform float uIsBackrooms;
uniform float uTime;

out vec4 FragColor;

void main() {
    vec2 localUV = fract(vUV);
    vec2 uv = vTileMin + localUV * vTileSize;
    vec2 ddx = dFdx(vUV) * vTileSize;
    vec2 ddy = dFdy(vUV) * vTileSize;
    vec4 texel = textureGrad(uAtlas, uv, ddx, ddy);

    if (texel.a < 0.5) {
        discard;
    }

    // Directional face shading
    float shade;
    if (vNormal.y > 0.5)          shade = 1.00;
    else if (vNormal.y < -0.5)    shade = 0.55;
    else if (abs(vNormal.z) > 0.5) shade = 0.85;
    else                           shade = 0.75;

    // Smooth Ambient Occlusion factor
    float aoClamped = max(vAO, 0.0);
    float aoFactor = 0.35 + 0.65 * aoClamped;

    // Smooth lighting response curve (standard non-linear perceptual power curve)
    float smoothSun = pow(clamp(vLight, 0.0, 1.0), 1.25);
    float smoothTorch = pow(clamp(vTorchLight, 0.0, 1.0), 1.25);

    // Sunlight: modulated dynamically by sun angle / time of day.
    // At night (uSunlight == 0), sunlight gives pitch black night with only 0.025 faint moonlight on sky-exposed surfaces.
    // In deep caves (vLight == 0), skyFactor is 0.0 (absolute pitch black darkness without torches).
    float moonlight = 0.025;
    float skyFactor = smoothSun * (moonlight + (1.0 - moonlight) * uSunlight);

    // Block / Torch light: constant independent of time of day
    float torchFactor = smoothTorch * 0.95;

    // Max blend between daylight/moonlight and torch light
    float totalLight = max(skyFactor, torchFactor) * aoFactor * shade;
    vec3 color = texel.rgb * totalLight;

    // Warm golden glow boost for torch illumination
    if (vTorchLight > 0.05) {
        color += texel.rgb * vec3(0.20, 0.10, 0.02) * smoothTorch * aoFactor;
    }

    // Red damage indicator when entity is hurt (indicated by negative vAO)
    if (vAO < 0.0) {
        color = mix(color, vec3(1.0, 0.18, 0.18), 0.72);
    }

    float dist = length(vWorldPos - uCamPos);
    float fog = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    color = mix(color, uFogColor, fog);

    // Backrooms Liminal Horror Analog Noise & Voltage Flicker
    if (uIsBackrooms > 0.5) {
        // 1. High-frequency analog film grain
        float grain = fract(sin(dot(gl_FragCoord.xy + vec2(uTime * 140.0, uTime * 83.0), vec2(12.9898, 78.233))) * 43758.5453);
        color += (grain - 0.5) * 0.055;

        // 2. Subtle CRT scanline modulation
        float scanline = sin(gl_FragCoord.y * 1.8 + uTime * 12.0) * 0.025;
        color -= scanline;

        // 3. Fluorescent ballast micro-voltage flicker
        float flicker = 1.0 - 0.05 * step(0.965, fract(sin(floor(uTime * 20.0) * 23.41) * 4375.85));
        color *= flicker;

        // 4. Damp yellowish-green liminal grading
        color = mix(color, color * vec3(1.08, 1.04, 0.82), 0.40);
    }

    FragColor = vec4(color, texel.a);
}
