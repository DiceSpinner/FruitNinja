#version 330 core
in vec4 fragColor;
out vec4 FragColor;
 
uniform sampler2D image;

void main()
{
	if(fragColor.a <= 0) { discard; }
	FragColor = fragColor;
}