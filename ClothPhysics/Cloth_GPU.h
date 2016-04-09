#pragma once

#include "glm\glm.hpp"
#include <vector>
#include <array>
#include "Basic_Shader.h"
#include "Phong_Shader.h"
#include "trimesh.h"
class Cloth_GPU
{
public:
	typedef std::vector<trimesh::index_t> vec;
	Cloth_GPU();
	~Cloth_GPU();

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
	void UpdatePositionsFromGPU();
	void UpdateGPUAfterCut();
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

	const int points_x = 75;
	const int points_y = 75;
	int current_points = points_x*points_y;
	int num_indices;
	const int maximum_split_points = 3 * current_points;
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

	glm::vec2 inv_cloth_size = glm::vec2(float(sizeX) / (points_x-1), float(sizeY) / (points_y-1));
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

	int texture_size_x = points_x;
	int texture_size_y = points_y;
	
	GLfloat pointSize = 30;

	int readID = 0, writeID = 1;

	GLuint vboID_Pos[2];
	GLuint vboID_PrePos[2];
	GLuint vboID_Struct, vboID_Shear, vboID_Bend;

	GLuint vboID_Normal;
	GLuint vboID_TexCoord;

	GLuint vaoUpdateID[2], vaoRenderID[2], vboID_Indices;
	
	GLuint texPosID[2];
	GLuint texPrePosID[2];

	bool isPointAbovePlane(glm::vec3 p1, glm::vec3 pointOnPlane, glm::vec3 planeNormal);
	enum SPRING
	{
		STRUCT,
		SHEAR,
		BEND
	};
	void FixSprings(vec& faces_above, vec& faces_below, int split_index);
	void FixStructSprings(const vec& vertices_below, int split_index);
	void FixShearSprings(const vec& vertices_below, int split_index);
	void FixBendSprings(const vec& vertices_below, int split_index);
	glm::ivec4 SplitSpring(std::vector<glm::ivec4>& springs, trimesh::index_t split_index, vec indexes_to_remove_from_original);
	unsigned int getReverseDirection(unsigned int direction);
	vec getCommonVertices(vec faces_above, vec faces_below);
	vec getVertices(vec faces);
	void fillTriangles(std::vector< trimesh::triangle_t>& triang);
};

