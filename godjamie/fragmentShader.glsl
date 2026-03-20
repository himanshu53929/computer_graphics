#version 330 core

out vec4 daColor;

struct Material{
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};

struct Light{
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

in vec3 FragPos;
in vec2 TextCord;
in vec3 Normal;

uniform Material material;
uniform Light light;

void main()
{
	// ambient lightining
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, TextCord));

	// diffuse lightining
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(light.position - FragPos);
	float diff = max(dot(lightDir, norm), 0.0f);
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TextCord));

	vec3 result = ambient + diffuse;
	daColor = vec4(result, 1.0f);
}