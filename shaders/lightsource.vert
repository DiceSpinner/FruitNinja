#version 330 core
layout (location = 0) in vec3 modelPosition;

uniform mat4 modelTransform;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * modelTransform * vec4(modelPosition, 1.0);
}