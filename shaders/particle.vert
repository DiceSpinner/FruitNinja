#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 offset;

uniform vec3 worldPos;
uniform vec3 cameraPos;
uniform mat4 view;
uniform mat4 projection;

out vec2 uv;

void main()
{
	 uv = pos.xy + vec2(0.5, 0.5);

	 vec3 sum = worldPos + pos + offset;
	 vec3 forward = cameraPos - worldPos;
	 forward.y = 0;
	 forward = normalize(forward);
	 vec3 right = cross(vec3(0, 1, 0), forward);
	 vec3 finalPos = vec3(0, sum.y, 0) + sum.x * right + sum.z * forward;

	 gl_Position = projection * view * vec4(finalPos, 1);
}