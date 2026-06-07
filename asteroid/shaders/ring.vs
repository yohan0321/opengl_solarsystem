// ring.vs
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 3) in mat4 instanceMatrix;   // local scale+rotation only

uniform mat4 view;
uniform mat4 projection;
uniform vec3 planetCenter;  // world position of the planet

out vec2 TexCoords;

void main()
{
    // build world‐space position by: local→(instance)→planetCenter
    vec4 localPos = instanceMatrix * vec4(aPos,1);
    vec4 worldPos = vec4(localPos.xyz + planetCenter, 1.0);
    TexCoords = aTex;
    gl_Position = projection * view * worldPos;
}
