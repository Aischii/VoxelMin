#version 330 core

in vec2 vUV;
uniform float uTime;
uniform float uAspect;

out vec4 FragColor;

void main() {
    // Radial vignette calculation
    vec2 centered = (vUV - 0.5) * vec2(uAspect, 1.0);
    float dist = length(centered);

    // Subtle animated caustic ripples
    float wave1 = sin(vUV.x * 28.0 + uTime * 2.0 + sin(vUV.y * 18.0 + uTime * 1.4)) * 0.5 + 0.5;
    float wave2 = cos(vUV.y * 24.0 - uTime * 1.6 + cos(vUV.x * 20.0 + uTime * 1.1)) * 0.5 + 0.5;
    float shimmer = (wave1 * 0.55 + wave2 * 0.45) * 0.08;

    // Deep oceanic blue center to dark vignetted perimeter
    vec4 centerColor = vec4(0.04, 0.22, 0.62, 0.38 + shimmer);
    vec4 edgeColor = vec4(0.01, 0.08, 0.30, 0.76);

    float vignette = smoothstep(0.28, 0.95, dist);
    vec4 color = mix(centerColor, edgeColor, vignette);

    FragColor = color;
}
