#version 330 core
out vec4 FragColor;

uniform vec4 overlayColor;

void main() {
    FragColor = vec4(overlayColor.rgb * vec3(1.0, 0.7, 1.0), overlayColor.a);
}