#include "GridMesh.h"
#include <iostream>

GridMesh::GridMesh()
{

}

GridMesh::GridMesh(unsigned int height, unsigned int width) : m_height(height), m_width(width)
{
	InitGridMesh(height, width);
}

void GridMesh::InitGridMesh(unsigned int height, unsigned int width)
{
	IndexedModel model;

	for (unsigned int j = 0; j < height; j++)
	{
		for (unsigned int i = 0; i < width; i++)
		{
			model.positions.push_back(glm::vec3(i, j, 0));
			model.texCoords.push_back(glm::vec2((float)i / (float)width, (float)j / (float)height));
			model.normals.push_back(glm::vec3(0, 1, 0));

			//indices
			model.indices.push_back(j*(width)+i);
			model.indices.push_back(j*(width)+i + 1);
			model.indices.push_back((j + 1)*width + i);

			model.indices.push_back(j*(width)+i + 1);
			model.indices.push_back((j + 1)*width + i + 1);
			model.indices.push_back((j + 1)*width + i);
		}
	}

	InitMesh(model);
	m_model = model;
}


GridMesh::~GridMesh()
{
}

void GridMesh::Draw()
{
	glDisable(GL_CULL_FACE);
	m_model.positions.at(0).x += 0.001;
	UpdateModel(GL_DYNAMIC_DRAW);
	StandardDraw();
	glEnable(GL_CULL_FACE);
}

glm::vec3 GridMesh::GetPositionOf(unsigned int x, unsigned int y)
{
	return m_model.positions.at(y*m_width + x);
}

glm::vec3 GridMesh::GetPositionOf(unsigned int a)
{
	return m_model.positions.at(a);
}