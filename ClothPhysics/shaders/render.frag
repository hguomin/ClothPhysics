#version 410 core

layout(location = 0) out vec4 vFragColor;

uniform sampler2D sampler;

//input from vertex shader
smooth in vec2 uv;

void main()
{
	vFragColor = texture2D(sampler, uv.st);
}