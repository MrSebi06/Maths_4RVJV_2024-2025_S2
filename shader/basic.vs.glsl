#version 330 core
layout (location = 0) in vec2 aPosition;

uniform mat4 projection;  // Add this line

void main() {
    gl_Position = projection * vec4(aPosition, 0.0, 1.0);  // Use projection
    gl_PointSize = 30.0;
}