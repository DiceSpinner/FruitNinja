#version 330 core
in vec2 uv;
in vec4 colorMod;
out vec4 FragColor;
 
uniform sampler2D image;
uniform vec4 textColor;

void main()
{
	vec4 color = colorMod * texture(image, uv);
	if(color.a <= 0) { discard; }
	FragColor = color;
}