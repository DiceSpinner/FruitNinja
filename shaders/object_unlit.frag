#version 330 core
in vec2 uv;
in vec3 normal;
in vec4 position;
out vec4 FragColor;

struct Material {
	int dArraySize;
    sampler2D diffuse[4];
	vec4 diffuseColor;

	int sArraySize;
    sampler2D specular[4];
	vec4 specularColor;
    float shininess;

	int aArraySize;
    sampler2D ambient[4];
	vec4 ambientColor;
}; 
struct Light {
	vec3 position;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};
 
uniform Material material;
uniform Light light;
uniform vec3 cameraPosition;

void main()
{
	vec4 diffuse1;
	vec4 ambient1;
	float shininess = material.shininess;
	if(material.dArraySize > 0){
		diffuse1 = texture(material.diffuse[0], uv);
	}
	else{
		diffuse1 = material.diffuseColor;
	}

	if(material.aArraySize > 0){
		ambient1 = texture(material.ambient[0], uv);	
	}else{
		ambient1 = material.ambientColor;
	}

	vec4 ambient = ambient1 * light.ambient;
	vec4 diffuse = diffuse1 * light.diffuse;

	FragColor = ambient + diffuse;
}