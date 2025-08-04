#version 430 core

layout (location=0) in vec3 pos;
layout (location=1) in vec3 color;

out vec3 in_color;

uniform mat4 proj;
uniform mat4 model;

void main(){
	gl_Position = proj * model * vec4(pos, 1.0);
	gl_PointSize = 20.0;
	in_color = color;
}
