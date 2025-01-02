#version 330 core
layout (location = 0) in vec2 tex;
layout (location = 1) in vec3 position;

uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;
out vec2 uv;

void main()
{
	 uv = tex;
	gl_Position = projection * view * transform * vec4(position, 1);
}