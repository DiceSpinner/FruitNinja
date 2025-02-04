#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 offset;

uniform vec3 modelPos;
uniform mat4 view;
uniform mat4 projection;

out vec2 uv;

void main()
{
	 uv = pos.xy + vec2(0.5, 0.5);
	 gl_Position = projection * (vec4(pos + modelPos + offset, 0) + view * vec4(0, 0, 0, 1));
}