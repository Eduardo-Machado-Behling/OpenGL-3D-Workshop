#version 450 core
out vec4 FragColor;

uniform vec3 fillColor;
uniform vec3 borderColor;
uniform bool renderBorder;

void main()
{
    if (renderBorder)
    {
        FragColor = vec4(borderColor, 1.0);
    }
    else
    {
        FragColor = vec4(fillColor, 1.0);
    }
}
