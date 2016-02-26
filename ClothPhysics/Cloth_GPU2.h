#pragma once

#include "glm\glm.hpp"
#include <vector>
#include <array>
#include "Basic_Shader.h"
class Cloth_GPU2
{
public:
	Cloth_GPU2();
	~Cloth_GPU2();

	void Draw(const Transform& transform,const Camera& camera );

private:
	void setupPositions();
	void setupIndices();
	void setupShaders();
	void createVBO();
	void setupTransformFeedback();
	Basic_Shader massSpringShader;
	Basic_Shader particleShader;
	Basic_Shader renderShader;


	const int width = 1024, height = 1024;
	const int numX = 63, numY = 63;
	const int total_points = (numX + 1)*(numY + 1);
	const int sizeX = 4, sizeY = 4;
	const float hsize = sizeX / 2.0f;
	const int NUM_ITER = 4;
	bool bDisplayMasses = true;


	std::vector<glm::vec4> X;		//current positions
	std::vector<glm::vec4> X_last;	//previous positions
	std::vector<glm::vec3> F;
	std::vector<GLushort> indices;

	const float DEFAULT_DAMPING = -0.0125f;
	float	KsStruct = 50.75f, KdStruct = -0.25f;
	float	KsShear = 50.75f, KdShear = -0.25f;
	float	KsBend = 50.95f, KdBend = -0.25f;
	glm::vec3 gravity = glm::vec3(0.0f, -0.00981f, 0.0f);
	float mass = 1.0f;

	float timeStep = 1.0f / 60.0f;

	int texture_size_x = numX + 1;
	int texture_size_y = numY + 1;
	
	GLfloat pointSize = 30;

	int readID = 0, writeID = 1;

	GLuint vboID_Pos[2];
	GLuint vboID_PrePos[2];

	GLuint vaoUpdateID[2], vaoRenderID[2], vboIndices;
	GLuint texPosID[2];
	GLuint texPrePosID[2];

	GLuint tfID;

	std::array<GLfloat, 4> vRed;
	std::array<GLfloat, 4> vBeige;
	std::array<GLfloat, 4> vWhite;
	std::array<GLfloat, 4> vGray;
};
