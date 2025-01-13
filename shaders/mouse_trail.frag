#version 330 core
in vec2 uv;
in float isArrow;
out vec4 FragColor;
 
uniform sampler2D image;
uniform sampler2D arrow;

void main()
{
	if(isArrow < -0.6){
		FragColor = texture(arrow, uv);
	}else{
		FragColor = texture(image, uv);
	}
}