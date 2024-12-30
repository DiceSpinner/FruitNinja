#version 330 core
layout (location = 0) in vec3 modelPosition;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 aNormal;

uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;
out vec2 uv;
out vec3 normal;
out vec4 position;

void main()
{
	uv = texCoord;
	normal = mat3(transpose(inverse(transform))) * aNormal;
	position = transform * vec4(modelPosition, 1);
	vec4 pos = projection * view * transform * vec4(modelPosition, 1.0);
	gl_Position = pos;
}