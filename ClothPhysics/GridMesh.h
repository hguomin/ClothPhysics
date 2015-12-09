#pragma once
#ifndef GRIDMESH_H
#define GRIDMESH_H
#include "Mesh.h"
#include <string>

class GridMesh : public Mesh
{
public:
	GridMesh();
	GridMesh(unsigned int height, unsigned int width);
	~GridMesh();

	void Draw();

	glm::vec3 GetPositionOf(unsigned int x, unsigned int y);
	glm::vec3 GetPositionOf(unsigned int a);

	void UpdateNormals();

	std::vector<glm::vec3> GetPositions();
	void UpdatePositions(std::vector<glm::vec3> &position);

	unsigned int GetHeight(){ return m_height; }
	unsigned int GetWidth() { return m_width; }
	bool SplitVertex(unsigned int v_X, unsigned int v_Y, const glm::vec3 vertex_pos, const glm::vec3 cut_normal);

	void print();

protected:
	void InitGridMesh(unsigned int height, unsigned int width);
	void UpdateIndices();

private:
	void CalculateNormals();
	void UpdateTextureCoords();
	unsigned int m_height;
	unsigned int m_width;

	
	
	trimesh::trimesh_t m_triMesh;

	bool debug = true;
	void debugMsg(const std::string& msg, int count = -1);
};

#endif //GRIDMESH_H