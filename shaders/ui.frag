#version 330 core
in vec2 uv;
in float isText;
out vec4 FragColor;
 
uniform sampler2D image;
uniform sampler2D textAtlas;
uniform vec4 textColor;
uniform vec4 imageColor;

void main()
{
	if(isText > 0){
		FragColor = textColor * texture(textAtlas, uv).r;
	}else{
		FragColor = imageColor * texture(image, uv);
	}
}