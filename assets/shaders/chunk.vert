#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec2 aTileMin;
layout(location = 4) in vec2 aTileSize;
layout(location = 5) in float aAO;
layout(location = 6) in float aLight;

uniform mat4 uVP;

out vec3 vNormal;
out vec2 vUV;
out vec2 vTileMin;
out vec2 vTileSize;
out vec3 vWorldPos;
out float vAO;
out float vLight;

void main() {
    vNormal = aNormal;
    vUV = aUV;
    vTileMin = aTileMin;
    vTileSize = aTileSize;
    vWorldPos = aPos;
    vAO = aAO;
    vLight = aLight;
    gl_Position = uVP * vec4(aPos, 1.0);
}
