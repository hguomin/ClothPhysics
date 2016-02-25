#version 400 core

// This input vector contains the vertex position in xyz, and the
// mass of the vertex in w
layout (location = 0) in vec4 position_mass;
// This is our stretch connection vector
layout (location = 2) in ivec4 stretch_connection;
// This is our shear connection vector
layout (location = 3) in ivec4 shear_connection;
// This is our bend connection vector
layout (location = 4) in ivec4 bend_connection;


// This is a TBO that will be bound to the same buffer as the
// position_mass input attribute
uniform samplerBuffer tex_position;

// The outputs of the vertex shader are the same as the inputs
out vec4 tf_position_mass;
out vec4 tf_last_position;

// A uniform to hold the timestep. The application can update this.
uniform float t = 0.07;

// The global spring constant
uniform float k = 7.1;

// Gravity
const vec3 gravity = vec3(0.0, -0.08, 0.0);

// Global damping constant
uniform float c = 2.8;

// Spring resting length
uniform float shear_length;
uniform float stretch_length;
uniform float bend_length;

vec3 Constraint(vec3 pos, int index, float rest)
{
	vec3 q = texelFetch(tex_position, index).xyz; //this is other position
	float curr_length = length(q-pos);
	float err_length = curr_length - rest;
	vec3 err_dir = normalize(q-pos);
	return err_dir*err_length*0.5;
}

void main(void)
{

    vec3 p = position_mass.xyz;    // p is our current position
	float m = position_mass.w;
	vec3 diff = vec3(0);
	
	if(m> 0)
	{
	for (int i = 0; i < 4; i++) {
        if (stretch_connection[i] != -1) {
            //diff -= Constraint(p,stretch_connection[i], stretch_length);
        }
		/*
		if (shear_connection[i] != -1)
		{
			diff += Constraint(p,shear_connection[i], shear_length);
		}
		if	(bend_connection[i] != -1)
		{
			diff += Constraint(p,bend_connection[i], bend_length);
		}
		*/
    }
	}
    // Write the outputs
    tf_position_mass = vec4(p +diff, m);
	tf_last_position = vec4(p,m);
}
