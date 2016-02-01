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
	Cloth_GPU(unsigned int width, unsigned int height, unsigned int particl_width, unsigned int particle_height);
	~Cloth_GPU();

	void Draw();

private:
	/*------HELPER FUNCTIONS--------*/
	void setStartPosition();
	void setIndices();
	void generateArraysAndBuffers();
	/*------Particle Storage-------*/
	std::vector<glm::vec4> m_position;
	std::vector<glm::vec4> m_last_position;
	std::vector<glm::vec4> m_forces;
	/*------Rendering stuff-------*/
	std::vector<GLushort> m_indices;
	GLuint m_vArrayoCloth;
	GLuint m_vBufferoCloth;
	GLuint m_vBufferoIndices;
	GLuint m_vArrayoUpdate[2];
	GLuint m_vArrayoRender[2];
	GLuint m_vBuffero_Pos[2];
	GLuint m_vBuffero_PrevPos[2];
	//texture positions
	GLuint m_texPos[2];
	GLuint m_texPrevPos[2];
	//Transform feedback
	GLuint m_transformFeedback;
	unsigned int NUM_ITER = 10;
	GLuint m_read = 0;
	GLuint m_write = 1;
	//time querry
	GLuint t_query;
	/*------Grid stuff-------------*/
	float m_width;
	float m_height;
	unsigned int m_particles_width;
	unsigned int m_particles_height;
	unsigned int m_total_particles;
};

#endif //CLOTH_GPU_H