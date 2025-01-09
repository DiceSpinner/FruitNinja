#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

uniform mat4 transform;
uniform mat4 projection;
out vec2 uv;
out float isText;

void main()
{
	 uv = tex;
	 isText = pos.z;
	 gl_Position = projection * transform * vec4(pos.xy, 0, 1);
}