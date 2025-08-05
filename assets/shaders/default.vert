#version 430 core

layout (location=0) in vec3 pos;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 uv;

out vec3 in_color;
out vec2 in_uv;

uniform mat4 proj;
uniform mat4 model;
uniform mat4 view;

void main(){
	gl_Position = proj * view * model * vec4(pos, 1.0);
	gl_PointSize = 20.0;
	in_color = normal;
	in_uv = uv;
}
