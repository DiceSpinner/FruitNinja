#version 330 core
uniform vec4 color;

out vec4 FragColor;

void main()
{
	if(color.a <= 0) { discard; }
	FragColor = color;
}