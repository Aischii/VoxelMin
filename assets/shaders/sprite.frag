#version 330 core
in vec2 vUV;
uniform sampler2D uTexture;
uniform vec4 uTint;
out vec4 FragColor;

void main() {
    vec4 texel = texture(uTexture, vUV);
    if (texel.a < 0.1) discard;
    FragColor = texel * uTint;
}
