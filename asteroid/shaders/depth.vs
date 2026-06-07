#version 330 core

// Vertex position input
layout(location = 0) in vec3 aPos;

// Transformation matrices
uniform mat4 model;
uniform mat4 shadowMatrix;

void main() {
    // Transform vertex to shadow projection-view space
    gl_Position = shadowMatrix * model * vec4(aPos, 1.0);
}
