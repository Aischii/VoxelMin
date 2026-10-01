#version 330 core

in vec3 vNormal;
in vec2 vUV;
in vec2 vTileMin;
in vec2 vTileSize;
in vec3 vWorldPos;
in float vAO;
in float vLight;

uniform sampler2D uAtlas;
uniform vec3 uCamPos;
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

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

    // Combine sky light + vertex ambient occlusion + directional shading smoothly
    float lightFloor = 0.35;
    float skyFactor = lightFloor + (1.0 - lightFloor) * vLight;
    float aoClamped = max(vAO, 0.0);
    float aoFactor = 0.35 + 0.65 * aoClamped;
    float lightLevel = skyFactor * aoFactor * shade;
    vec3 color = texel.rgb * lightLevel;

    // Red damage indicator when entity is hurt (indicated by negative vAO)
    if (vAO < 0.0) {
        color = mix(color, vec3(1.0, 0.18, 0.18), 0.72);
    }

    float dist = length(vWorldPos - uCamPos);
    float fog = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    color = mix(color, uFogColor, fog);

    FragColor = vec4(color, texel.a);
}
