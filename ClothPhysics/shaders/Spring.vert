/*
Demo code accompanying the Chapter: "Realtime Deformation using Transform Feedback" by
Muhammad Mobeen Movania and Lin Feng. This is the shader code implementing the Verlet 
integration on the GPU. 

Author: Muhammad Mobeen Movania
Last Modified: 9 September 2011.
*/
#version 400
precision highp float;

layout( location = 0 )  in vec4 position_mass;
layout( location = 1 )  in vec4 prev_position;

layout( location = 2 )  in ivec4 spring_struct; //(right, down, left, up)
layout( location = 3 )  in ivec4 spring_shear;  //(right down, left down, left up, right up)
layout( location = 4 )  in ivec4 spring_bend;   //(right, down, left, up)
 
uniform mat4 MVP;
uniform samplerBuffer tex_position_mass;
uniform samplerBuffer tex_prev_position_mass;
uniform vec2  inv_cloth_size;			//size of a single patch in world space
uniform vec2  step;						//delta texture size
uniform int texsize_x;					//size of position texture
uniform int texsize_y; 
uniform float dt, ksStr, ksShr, ksBnd, 
				 kdStr, kdShr, kdBnd, DEFAULT_DAMPING;
uniform float rest_struct;
uniform float rest_shear;
uniform float rest_bend;
  
uniform vec3 gravity;
 
out vec4 to_gs_position_mass;
out	vec4 to_gs_prev_position;
out int to_gs_vertexID;
out vec3 to_gs_vertexNormal;
 
// Resolve constraint in this space
uniform vec3 ball_position;
uniform float radius;

void sphereCollision(inout vec3 x, vec3 center, float r)
{
	vec3 delta = x - center;
	float dist = length(delta);
	if (dist < r) {
		x = center + delta*(r / dist);
	}
} 
 
 vec3 calcSpringForce(vec3 p1,vec3 p1_last,vec3 p1_vel, vec3 p2,vec3 p2_last,float rest_length,float ks,float kd)
 {
	
	vec3 v2 = (p2- p2_last)/dt;
	vec3 deltaP = p1 - p2;	
	vec3 deltaV = p1_vel - v2;	 
	float dist = length(deltaP);
				
	float  leftTerm = -ks * (dist-rest_length);
	float  rightTerm = kd * (dot(deltaV, deltaP)/dist);		
	vec3 springForce = (leftTerm + rightTerm)* normalize(deltaP);
	return springForce;
 }
 
 vec3 getNormal(vec3 p1, vec3 p2, vec3 p3)
 {
	vec3 edge1 = p2-p1;
	vec3 edge2 = p3-p1;
	return cross(edge1, edge2);
 }
 /*
 *  - - *  - - *
 |   /  | 1 /  |
 |  / 0 |  / 2 |
 *  - - *  - - *
 | 5 /  | 3 /  |
 |  / 4 |  /   |
 *  - - *  - - *
 */
 //this function assumes no holes
 vec3 calculateNormal(vec3 p1)
 {
	vec3 p2;
	vec3 p3;
	vec3 edge1;
	vec3 edge2;
	vec3 normal = vec3(0,0,0);
	//triangle 0
	int index2 = spring_struct[1]; //above
	int index1 = spring_struct[2]; //left
	if(index1 != -1 && index2 != -1)
	{
		p2 = texelFetch(tex_position_mass, index1).xyz;
		p3 = texelFetch(tex_position_mass, index2).xyz;
		normal += getNormal(p1,p2,p3);
	}
	
	//triangle 1
	index1 = spring_struct[1]; //above
	index2 = spring_shear[0];  //top right
	if(index1 != -1 && index2 != -1)
	{
		p2 = texelFetch(tex_position_mass, index1).xyz;
		p3 = texelFetch(tex_position_mass, index2).xyz;
		normal += getNormal(p1,p2,p3);
	}
	
	//triangle 2
	index1 = spring_shear[0]; //top right
	index2 = spring_struct[0]; //right
	if(index1 != -1 && index2 != -1)
	{
		p2 = texelFetch(tex_position_mass, index1).xyz;
		p3 = texelFetch(tex_position_mass, index2).xyz;
		normal += getNormal(p1,p2,p3);
	}
	
	//triangle 3
	index1 = spring_struct[0]; //right
	index2 = spring_struct[3]; //bottom
	if(index1 != -1 && index2 != -1)
	{
		p2 = texelFetch(tex_position_mass, index1).xyz;
		p3 = texelFetch(tex_position_mass, index2).xyz;
		normal += getNormal(p1,p2,p3);
	}
	
	//triangle 4
	index1 = spring_struct[3]; //bottom
	index2 = spring_shear[2]; //bottom left
	if(index1 != -1 && index2 != -1)
	{
		p2 = texelFetch(tex_position_mass, index1).xyz;
		p3 = texelFetch(tex_position_mass, index2).xyz;
		normal += getNormal(p1,p2,p3);
	}
	
	//triangle 5
	index1 = spring_shear[2]; //bottom left
	index2 = spring_struct[2]; //left
	if(index1 != -1 && index2 != -1)
	{
		p2 = texelFetch(tex_position_mass, index1).xyz;
		p3 = texelFetch(tex_position_mass, index2).xyz;
		normal += getNormal(p1,p2,p3);
	}
	return normalize(normal);
}
  
void main(void) 
{  
	float m = position_mass.w;
	vec3 pos = position_mass.xyz;
    vec3 pos_old = prev_position.xyz;	
	vec3 vel = (pos - pos_old) / dt;
	float ks=0, kd=0;
	int index = gl_VertexID;
	int ix = index % texsize_x;
	int iy = index / texsize_x;
	
    vec3 F = gravity*m + (DEFAULT_DAMPING*vel);
	for	(int i = 0; i < 4;i++)
	{
		int index_neigh = spring_struct[i];
		if (index_neigh != -1)
		{
			vec3 p2 = texelFetch(tex_position_mass, index_neigh).xyz;
			vec3 p2_last = texelFetch(tex_prev_position_mass, index_neigh).xyz;
			F+= calcSpringForce(pos,pos_old,vel, p2,p2_last,rest_struct,ksStr,kdStr);
		}
	}

	for(int i = 0; i < 4; i ++)
	{
		int index_neigh = spring_shear[i];
		if (index_neigh != -1)
		{
			vec3 p2 = texelFetch(tex_position_mass, index_neigh).xyz;
			vec3 p2_last = texelFetch(tex_prev_position_mass, index_neigh).xyz;
			F+= calcSpringForce(pos,pos_old,vel, p2,p2_last,rest_shear,ksShr,kdShr);
		}
	}
	for( int i = 0; i < 4; i++)
	{
		int index_neigh = spring_bend[i];
		if (index_neigh != -1)
		{
			vec3 p2 = texelFetch(tex_position_mass, index_neigh).xyz;
			vec3 p2_last = texelFetch(tex_prev_position_mass, index_neigh).xyz;
			F+= calcSpringForce(pos,pos_old,vel,p2,p2_last,rest_bend,ksBnd,kdBnd);
		}
	}
	sphereCollision(pos,ball_position,radius);
    vec3 acc = vec3(0);
	if(m!=0)
	   acc = F/m; 

	
	vec3 tmp = pos; 
	pos = pos * 2.0 - pos_old + acc* dt * dt;
	pos_old = tmp;
	to_gs_position_mass = vec4(pos, m);	
	to_gs_prev_position = vec4(pos_old,m);	
	to_gs_vertexID = gl_VertexID;
	to_gs_vertexNormal = calculateNormal(pos);
	gl_Position = MVP*vec4(pos, 1);
}