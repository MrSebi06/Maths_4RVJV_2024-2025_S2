#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 VertexColor;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;
uniform bool useLighting;
uniform bool useTexture;
uniform sampler2D texture1;

void main() {
    vec3 baseColor = VertexColor * objectColor;

    // Si on utilise une texture, multiplier par la couleur de la texture
    if (useTexture) {
        vec3 texColor = texture(texture1, TexCoord).rgb;
        baseColor *= texColor;
    }

    if (useLighting) {
        float ambientStrength = 0.15;
        vec3 ambient = ambientStrength * lightColor;
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        float specularStrength = 0.3;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
        vec3 specular = specularStrength * spec * lightColor;
        vec3 result = (ambient + diffuse + specular) * baseColor;
        FragColor = vec4(result, 1.0);
    } else {
        FragColor = vec4(baseColor, 1.0);
    }
}