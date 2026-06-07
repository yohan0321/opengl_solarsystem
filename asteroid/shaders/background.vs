#version 330 core

// Vertex attributes: 2D position and texture coordinates
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

// Output texture coordinates to fragment shader
out vec2 TexCoords;

void main() {
    // Pass texture coordinates
    TexCoords = aTexCoords;

    // Expand 2D position -> 3D clip-space position
    gl_Position = vec4(aPos, 0.0, 1.0);
}
