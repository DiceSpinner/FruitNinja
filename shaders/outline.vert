#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;

out vec2 uv;

void main()
{
	 uv = tex;
	 gl_Position = projection * view * transform * vec4(pos, 1);
}