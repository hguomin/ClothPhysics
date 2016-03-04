#version 400 core

layout (points) in;
layout (points) out;
layout (max_vertices = 1) out;

in vec4 to_gs_position_mass[];
in	vec4 to_gs_prev_position[];
in int to_gs_vertexID[];
in vec3 to_gs_vertexNormal[];

uniform samplerBuffer tex_position;

out vec4 out_position_mass;
out vec4 out_prev_position;
//out int out_vertexID;
out vec3 out_vertexNormal;

void main(void)
{
	for(int i = 0; i < gl_in.length(); i++)
	{
		out_position_mass = to_gs_position_mass[i];
		out_prev_position = to_gs_prev_position[i];
		gl_Position = gl_in[i].gl_Position;
		//out_vertexID = int(texelFetch(tex_position, to_gs_vertexID[i]).z);;//gl_PrimitiveID;
		out_vertexNormal = to_gs_vertexNormal[i];
		EmitVertex();
	}

	EndPrimitive();
    
}