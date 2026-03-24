#version 330 core

out vec4 daColor;


uniform vec3 sourceColor;

void main(){
	daColor = vec4(sourceColor, 1.0f);
}