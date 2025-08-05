#version 430 core

out vec4 FragColor;

in vec2 tex_coord;

uniform sampler2D texture1;

uniform vec2 uv_desloc;

void main() {
    // texture color
    vec4 texColor = texture(texture1, tex_coord+uv_desloc);

    FragColor = texColor;
}