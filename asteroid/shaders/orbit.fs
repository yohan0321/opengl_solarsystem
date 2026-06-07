#version 330 core

// Final output fragment color
out vec4 FragColor;

// Line color set from uniform
uniform vec3 lineColor;

void main() {
    // Output solid line color
    FragColor = vec4(lineColor, 1.0);
}
