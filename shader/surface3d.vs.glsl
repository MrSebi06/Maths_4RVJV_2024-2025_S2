#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 VertexColor;

void main() {
    FragPos = vec3(model * vec4(aPosition, 1.0));
    Normal = normalMatrix * aNormal;
    TexCoord = aTexCoord;
    VertexColor = aColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}