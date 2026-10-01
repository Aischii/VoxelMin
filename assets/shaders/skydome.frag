#version 330 core

in vec3 vDir;

uniform vec3 uZenith;
uniform vec3 uHorizon;
uniform vec3 uGround;

out vec4 FragColor;

void main() {
    float h = normalize(vDir).y;
    vec3 col;
    if (h >= 0.0) {
        col = mix(uHorizon, uZenith, pow(clamp(h, 0.0, 1.0), 0.55));
    } else {
        col = mix(uHorizon, uGround, pow(clamp(-h, 0.0, 1.0), 0.5));
    }
    FragColor = vec4(col, 1.0);
}
