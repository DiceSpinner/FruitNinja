#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

uniform mat4 transform;
out vec2 uv;
out int isText;

void main()
{
	 uv = tex;
	 isText = int(pos.z);
	 gl_Position = transform * vec4(pos.xy, 0, 1);
}