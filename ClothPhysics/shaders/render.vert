#version 410 core

layout (location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

//output to fragment shader
smooth out vec2 uv;
void main()
{
	mat4 MVP = projection * view * model;
	gl_Position = MVP * vec4(position*0.03, 1.0);

	uv = texCoord;
}
