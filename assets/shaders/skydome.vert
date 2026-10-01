#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 uVP;
uniform vec3 uCamPos;

out vec3 vDir;

void main() {
    // The dome is world-aligned and centred on the camera, so the local offset
    // equals the world direction from the camera to the sky point. Gradients
    // must use the world direction: a view-space direction would rotate the
    // gradient with the camera and lock it to the player's view.
    vDir = aPos;
    gl_Position = uVP * vec4(uCamPos + aPos, 1.0);
}
