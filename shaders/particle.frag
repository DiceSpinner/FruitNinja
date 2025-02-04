#version 330 core
in vec2 uv;
out vec4 FragColor;
 
uniform sampler2D image;
uniform vec4 textColor;

void main()
{
	FragColor = texture(image, uv);
}