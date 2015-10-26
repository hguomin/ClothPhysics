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
		}
	}
	//create indices
	for (unsigned int j = 0; j < m_height - 1; j++)
	{
		for (unsigned int i = 0; i < m_width - 1; i++)
		{
			model.indices.push_back(j*m_height + i);
			model.indices.push_back(j*m_height + i + 1);
			model.indices.push_back((j + 1)*m_height + i);

			model.indices.push_back(j*m_height + i + 1);
			model.indices.push_back((j + 1)*m_height + i);
			model.indices.push_back((j + 1)*m_height + i + 1);
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
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	Mesh::UpdateModel(GL_DYNAMIC_DRAW);
	Mesh::Draw();
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}

glm::vec3 GridMesh::GetPositionOf(unsigned int x, unsigned int y)
{
	return m_model.positions.at(y*m_width + x);
}

glm::vec3 GridMesh::GetPositionOf(unsigned int a)
{
	return m_model.positions.at(a);
}

std::vector<glm::vec3> GridMesh::GetPositions()
{
	return m_model.positions;
}

void GridMesh::UpdatePositions(std::vector<glm::vec3> &pos)
{
	assert(pos.size() == m_model.positions.size());
	m_model.positions = pos;
}

void GridMesh::print()
{
	std::cout << m_model.positions.end()->x << " " << m_model.positions.end()->y << " " << m_model.positions.end()->z << std::endl;
}