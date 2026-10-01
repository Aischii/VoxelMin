#version 330 core

in vec4 vColor;
out vec4 FragColor;

void main() {
    if (vColor.a < 0.02) {
        discard;
    }
    FragColor = vColor;
}
