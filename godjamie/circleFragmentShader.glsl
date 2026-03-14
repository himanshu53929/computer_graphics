#version 330 core

out vec4 daColor;

in vec4 theColor;
uniform float someColor;


void main()
{
	vec3 color = vec3(someColor);
	daColor = vec4(color, 1.0f);
}