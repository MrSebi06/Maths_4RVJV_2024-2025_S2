#version 120

attribute vec2 a_Position;
attribute vec3 a_Color;

varying vec4 v_Color;

void main() {
    gl_Position = vec4(a_Position, 0.0, 1.0);
    v_Color = vec4(a_Color, 1.0);
}