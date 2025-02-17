#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 worldPos;
layout (location = 2) in vec3 localScale;
layout (location = 3) in vec4 colorModifier;
layout (location = 4) in vec2 upDirection;

uniform vec3 scale;
uniform vec3 cameraPos;
uniform mat4 view;
uniform mat4 projection;

out vec2 uv;
out vec4 colorMod;

void main()
{
	 uv = pos.xy + vec2(0.5, 0.5);
	 colorMod = colorModifier;

	 vec3 sum = localScale * scale * pos;
	 vec3 forward = cameraPos - worldPos;
	 forward.y = 0;
	 forward = normalize(forward);
	 vec3 right = cross(forward, normalize(vec3(upDirection, 0)));
	 vec3 up = cross(right, forward);
	 vec3 finalPos = worldPos + sum.y * up + sum.x * right + sum.z * forward;

	 gl_Position = projection * view * vec4(finalPos, 1);
}