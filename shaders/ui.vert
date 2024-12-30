#version 330 core
layout (location = 1) in vec2 texCoord;

uniform mat4 transform;
out vec2 uv;

void main()
{
	uv = texCoord;
	gl_Position = transform * vec4(uv, 0, 1);
}