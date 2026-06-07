#version 330 core

// Output final fragment color
out vec4 FragColor;

// Inputs from vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Uniforms
uniform sampler2D texture_diffuse;
uniform samplerCube shadowMap;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float far_plane;
uniform bool isSun;

// Computes shadow factor from point light using cube map (no PCF)
float ShadowCalculationPoint(vec3 fragPos) {
    vec3 LtoF = fragPos - lightPos;
    float currentDepth = length(LtoF);
    float closestDepth = texture(shadowMap, LtoF).r * far_plane;
    float bias = 0.05;
    return step(closestDepth + bias, currentDepth);
}

void main() {
    // Sun emits texture color directly
    if (isSun) {
        vec3 emissive = texture(texture_diffuse, TexCoords).rgb;
        FragColor = vec4(emissive, 1.0);
        return;
    }

    // Lighting calculations
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    float diff = max(dot(N, L), 0.0);

    if (diff <= 0.0) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 diffuseColor = texture(texture_diffuse, TexCoords).rgb;
    vec3 diffuse = diff * diffuseColor;

    vec3 V = normalize(viewPos - FragPos);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), 32.0);
    vec3 specular = vec3(0.5 * spec);

    // Apply shadow
    float shadow = ShadowCalculationPoint(FragPos);
    vec3 resultColor = mix(diffuse + specular, vec3(0.0), shadow);

    FragColor = vec4(resultColor, 1.0);
}
