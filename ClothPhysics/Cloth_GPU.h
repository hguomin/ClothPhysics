#pragma once
#ifndef CLOTH_GPU_H
#define CLOTH_GPU_H
#include <vector>
#include <glm\glm.hpp>
#include "GL\glew.h"

class Cloth_GPU
{
public:
	Cloth_GPU();
	~Cloth_GPU();

	void Draw();

private:
	enum BUFFER_TYPE
	{
		POSITiON_A,
		POSITION_B,
		VELOCITY_A,
		VELOCITY_B,
		CONNECTION
	};

	void loadShaders();

	unsigned int m_points_width;
	unsigned int m_points_height;
	unsigned int m_points_total;
	unsigned int m_connections_total;

	//------Shaders and stuff
	GLuint m_vArrayO[2];
	GLuint m_vBufferO[5];
	GLuint m_index_buffer;
	GLuint m_pos_texBufferO[2];
	GLuint m_update_program;
	GLuint m_render_program;
	GLuint m_C_loc;
	GLuint m_iteration_index;

	int iterations_per_frame;
};

#endif //CLOTH_GPU_H