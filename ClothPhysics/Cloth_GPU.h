#pragma once
#ifndef CLOTH_GPU_H
#define CLOTH_GPU_H
#include <vector>
#include <glm\glm.hpp>
#include "GL\glew.h"
#include "Basic_Shader.h"

class Cloth_GPU
{
public:
	Cloth_GPU();
	~Cloth_GPU();

	void Draw(const Transform & transform, const Camera & camera);

private:
	enum BUFFER_TYPE
	{
		POSITION_A,
		POSITION_B,
		VELOCITY_A,
		VELOCITY_B,
		STRETCH_CONNECTION,
		SHEAR_CONNECTION,
		BEND_CONNECTION
		
	};

	void loadShaders();

	unsigned int m_points_width;
	float m_width;
	unsigned int m_points_height;
	float m_height;
	unsigned int m_points_total;
	unsigned int m_connections_total;

	//------Shaders and stuff
	GLuint m_vArrayO[2];
	GLuint m_vBufferO[5];
	GLuint m_index_buffer;
	GLuint m_pos_texBufferO[2];
	GLuint m_update_program;
	GLuint m_C_loc;
	GLuint m_iteration_index;
	Basic_Shader m_renderShader;

	int iterations_per_frame;
};

#endif //CLOTH_GPU_H