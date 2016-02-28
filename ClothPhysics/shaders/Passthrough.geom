#version 400 core

layout (points) in;
layout (points) out;
layout (max_vertices = 1) out;

in vec4 to_gs_position_mass[];
in	vec4 to_gs_prev_position[];
in int to_gs_geom[];

out vec4 out_position_mass;
out vec4 out_prev_position;
out int out_geom;

void main(void)
{
	for(int i = 0; i < 1; i++)
	{
		out_position_mass = to_gs_position_mass[i];
		out_prev_position = to_gs_prev_position[i];
		out_geom = to_gs_geom[i];
		gl_Position = gl_in[i].gl_Position;
		EmitVertex();
	}
    EndPrimitive();
}