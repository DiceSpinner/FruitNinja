#version 330 core
in vec2 uv;
in float isText;
out vec4 FragColor;
 
uniform sampler2D image;
uniform sampler2D textAtlas;
uniform vec4 textColor;

void main()
{
	if(isText > 0){
		FragColor = textColor * texture(textAtlas, uv).r;
	}else{
		FragColor = texture(image, uv);
	}
}