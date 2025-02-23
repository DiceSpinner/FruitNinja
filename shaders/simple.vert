#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;

uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;

out vec4 fragColor;

void main()
{
	 fragColor = color;
	 gl_Position = projection * view * transform * vec4(pos, 1);
}