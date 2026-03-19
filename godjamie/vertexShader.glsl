#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 textCord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec2  TextCord;
out vec3 Normal;
out vec3 FragPos;

void main()
{
	FragPos = vec3(model * vec4(position, 1.0f));
	Normal = normalMatrix * normal;
	gl_Position = projection * view * model * vec4(position, 1.0);
	TextCord = textCord;
}