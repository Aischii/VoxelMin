#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 uVP;
uniform mat4 uModel;

void main() {
    vec4 clip = uVP * uModel * vec4(aPos, 1.0);
    clip.z -= 0.0002 * clip.w;
    gl_Position = clip;
}
