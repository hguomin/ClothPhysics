#pragma once

#include "glm\glm.hpp"
#include <vector>
#include <array>
#include "Basic_Shader.h"
#include "Phong_Shader.h"
#include "trimesh.h"
class Cloth_GPU2
{
public:
	Cloth_GPU2();
	~Cloth_GPU2();

	void Draw(const Transform& transform,const Camera& camera );
	
	void Split(unsigned int index, glm::vec3 planeNormal);
private:
	void setupPositions();
	void setupIndices();
	void setupShaders();
	void setupSprings();
	void setupHEMesh();
	glm::ivec2 getNextNeighbor(int n);
	void createVBO();
	void setupTransformFeedback();

	void Simulate(glm::mat4 MVP);
	void massSpringShader_UploadData(glm::mat4 MVP);


	Basic_Shader massSpringShader;
	Basic_Shader splitShader;
	Phong_Shader renderShader;

	enum
	{
		RIGHT = 0,
		UP = 1,
		LEFT = 2,
		DOWN = 3,
		UP_RIGHT = 0,
		UP_LEFT = 1,
		DOWN_LEFT = 2,
		DOWN_RIGHT = 3,
	};

	const int width = 1024, height = 1024;
	const int numX = 2, numY = 2;
	int current_points = (numX + 1)*(numY + 1);
	int num_indices;
	const int max_points = 3 * current_points;
	const int sizeX = 4, sizeY = 4;
	const float hsize = sizeX / 2.0f;
	const int NUM_ITER = 4;
	bool bDisplayMasses = true;


	std::vector<glm::vec4> X;		//current positions
	std::vector<glm::vec4> X_last;	//previous positions
	std::vector<glm::vec3> F;
	std::vector<glm::vec2> Tex_coord;
	std::vector<GLushort> indices;
	std::vector<glm::ivec4> struct_springs;
	std::vector<glm::ivec4> shear_springs;
	std::vector<glm::ivec4> bend_springs;

	trimesh::trimesh_t m_he_mesh;

	glm::vec2 inv_cloth_size = glm::vec2(float(sizeX) / numX, float(sizeY) / numY);
	glm::vec3 gravity = glm::vec3(0.0f, -0.00981f, 0.0f);
	float mass = 1.0f;
	const float DEFAULT_DAMPING = -0.0125f;
	struct springData_t
	{
		float	KsStruct, KdStruct;
		float	KsShear, KdShear;
		float	KsBend, KdBend;
		float rest_struct;
		float rest_shear;
		float rest_bend;
	} springData;

	bool splitAdded = true;

	float timeStep = 1.0f / 60.0f;

	int texture_size_x = numX + 1;
	int texture_size_y = numY + 1;
	
	GLfloat pointSize = 30;

	int readID = 0, writeID = 1;

	GLuint vboID_Pos[2];
	GLuint vboID_PrePos[2];
	GLuint vboID_Struct, vboID_Shear, vboID_Bend;

	GLuint vboID_Normal;
	GLuint vboID_TexCoord;

	GLuint vaoUpdateID[2], vaoRenderID[2], vboIndices;
	
	GLuint texPosID[2];
	GLuint texPrePosID[2];

	bool isPointAbovePlane(glm::vec3 p1, glm::vec3 pointOnPlane, glm::vec3 planeNormal);
	enum SPRING
	{
		STRUCT,
		SHEAR,
		BEND
	};
	void FixSprings(std::vector<glm::ivec4>& springs, glm::ivec4& new_spring, trimesh::index_t face_above, trimesh::index_t face_below, int index, int new_index, int direction, SPRING springType);

	void fillTriangles(std::vector< trimesh::triangle_t>& triang);
};

