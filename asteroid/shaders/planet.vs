#version 330 core

// Vertex input layout
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

// Outputs to fragment shader
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

// Transformation matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Convert position to world space
    FragPos = vec3(model * vec4(aPos, 1.0));

    // Convert normal to world space (correct for non-uniform scale)
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Pass texture coordinates
    TexCoords = aTexCoords;

    // Output clip-space position
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
