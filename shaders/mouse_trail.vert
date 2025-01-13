#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

uniform mat4 projection;
out vec2 uv;
out float isArrow;

void main()
{
	 uv = tex;
	 isArrow = pos.z;
	 gl_Position = projection * vec4(pos.xyz, 1);
}