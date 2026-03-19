#version 330 core

out vec4 daColor;

uniform sampler2D textureArrow;

in vec2 texCoords;


void main()
{
	daColor = texture(textureArrow, texCoords);
}