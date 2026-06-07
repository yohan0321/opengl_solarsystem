#version 330 core

// Vertex position input
layout(location = 0) in vec3 aPos;

// Transformation matrices
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    // Compute final clip-space position
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
