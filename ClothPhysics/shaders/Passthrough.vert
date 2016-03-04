/*
Demo code accompanying the Chapter: "Realtime Deformation using Transform Feedback" by
Muhammad Mobeen Movania and Lin Feng. This is the shader code for passthrough vertex processing. 

Author: Muhammad Mobeen Movania
Last Modified: 9 September 2011.
*/
#version 400

in vec4 position_mass; 
uniform mat4 MVP;

void main(void) 
{  
	gl_Position = MVP*vec4(position_mass.xyz, 1.0);		
}