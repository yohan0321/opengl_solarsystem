#version 330 core

in vec2 TexCoords;
uniform sampler2D diffuseMap;

out vec4 FragColor;

void main()
{
    // Simple texturing—no lighting
    FragColor = texture(diffuseMap, TexCoords);
}
