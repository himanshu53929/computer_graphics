#version 330 core

out vec4 daColor;

in vec3 theColor;
in vec2 TextCord;

uniform sampler2D ourTexture;

void main()
{
	daColor = texture(ourTexture, TextCord);
}