#version 330 core
in vec2 uv;
in float isText;
out vec4 FragColor;
 
uniform sampler2D image;
uniform sampler2D textAtlas;

void main()
{
	if(isText > 0){
		FragColor = texture(textAtlas, uv);
	}else{
		FragColor = texture(image, uv);
	}
}