#version 330 core
layout (location = 0) in vec2 tex;

uniform mat4 transform;
out vec2 uv;

void main()
{
	vec2 temp = (tex - vec2(0.5, 0.5)) * 2;
	 uv = tex;
	gl_Position = transform * vec4(temp, 0, 1);
}