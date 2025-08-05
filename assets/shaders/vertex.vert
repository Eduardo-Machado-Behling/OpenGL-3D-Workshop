#version 430 core

layout (location=0) in vec3 pos;
layout (location=1) in vec2 uv;

out vec3 in_pos;
out vec2 tex_coord;

void main() {
	gl_Position = vec4(pos, 1.0);
	gl_PointSize = 10.0;

	in_pos = pos;
	tex_coord = uv;
}