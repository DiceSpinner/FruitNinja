#version 330 core
layout (location = 0) in vec2 tex;

uniform mat4 transform;
out vec2 uv;

void main()
{
	uv = tex;
	gl_Position = transform * vec4(uv, 0, 1);
}