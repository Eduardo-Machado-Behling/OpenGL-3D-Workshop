#version 450 core
out vec4 FragColor;

uniform vec3 planeColor;

void main()
{
    // Use the uniform for the final color, with full alpha
    FragColor = vec4(planeColor, 1.0);
}
