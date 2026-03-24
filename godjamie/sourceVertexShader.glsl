#version 330 core

layout (location = 0) in vec3 positions;
layout (location = 1) in vec3 Color;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;


void main(){
	gl_Position = projection * view * model * vec4(positions, 1.0f);

}