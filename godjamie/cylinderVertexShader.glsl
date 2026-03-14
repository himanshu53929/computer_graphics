#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normalCoordinates;
layout(location = 2) in vec3 texCoordinates;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 theColor;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0);
	theColor = vec4(1.0, 0.2f, 0.9f, 1.0f);
}