#version 330 core
in vec2 uv;
in int isText;
out vec4 FragColor;
 
uniform sampler2D image;
uniform sampler2D textAtlas;

void main()
{
	if(isText){
		FragColor = texture(textAtlas, uv);
	}else{
		FragColor = texture(image, uv);
	}
}