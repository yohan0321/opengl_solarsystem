#version 330 core

// Texture coordinates from vertex shader
in vec2 TexCoords;

// Final fragment color output
out vec4 FragColor;

// 2D background texture
uniform sampler2D bgTex;

void main() {
    // Sample color from background texture
    FragColor = texture(bgTex, TexCoords);
}
