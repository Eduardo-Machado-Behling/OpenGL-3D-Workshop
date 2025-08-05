#version 430 core

in vec3 in_color;
in vec2 in_uv;

out vec4 color;
uniform sampler2D texture_diffuse1;

uniform int type;


void main(){
	if (type == 0){
		color = texture(texture_diffuse1, in_uv);
	} else {
		color = vec4(in_color, 1.0);
	}
}
