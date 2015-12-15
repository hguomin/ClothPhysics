#pragma once
#ifndef GRIDMESH_H
#define GRIDMESH_H
#include "Mesh.h"

class GridMesh : public Mesh
{
public:
	GridMesh();
	GridMesh(unsigned int height, unsigned int width);
	~GridMesh();

	void Draw();

	glm::vec3 GetPositionOf(unsigned int x, unsigned int y);
	glm::vec3 GetPositionOf(unsigned int a);

	std::vector<glm::vec3> GetPositions();
	void UpdatePositions(std::vector<glm::vec3> &position);
	void UpdateNormals(std::vector<glm::vec3> &normals);

	unsigned int GetHeight(){ return m_height; }
	unsigned int GetWidth() { return m_width; }

	void print();

protected:
	void InitGridMesh(unsigned int height, unsigned int width);

private:
	unsigned int m_height;
	unsigned int m_width;
};

#endif //GRIDMESH_H