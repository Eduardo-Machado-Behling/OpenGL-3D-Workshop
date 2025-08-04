#version 430 core

in vec3 in_color;
in vec3 in_pos;

out vec4 color;

uniform float yClip = 1.0;

void main(){
	if(in_pos.y > yClip){
		discard;
	}

	color = vec4(in_color, 1.0);
}
