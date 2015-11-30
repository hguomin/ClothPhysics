#include "GridMesh.h"
#include "Half-Edge(notMine)\trimesh.h"
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
	std::vector<trimesh::edge_t> edges;
	std::vector<trimesh::triangle_t> triangles;

	//generate vertex positions
	for (unsigned int j = 0; j < height; j++)
	{
		for (unsigned int i = 0; i < width; i++)
		{
			model.positions.push_back(glm::vec3(i, j, 0));
			model.texCoords.push_back(glm::vec2((float)i / (float)(width - 1), (float)j / (float)(height - 1)));
			model.normals.push_back(glm::vec3(0, 1, 0));
		}
	}
	unsigned int count = 0;
	for (unsigned int j = 0; j < m_height - 1; j++)
	{
		for (unsigned int i = 0; i < m_width - 1; i++)
		{
			const unsigned int index = i + j*m_width;
			const unsigned int step_right = 1;
			const unsigned int step_down = m_width;
			trimesh::triangle_t temp;

			model.indices.push_back(index);
			temp.v[0] = index;
			model.indices.push_back(index + step_right);
			temp.v[1] = index + step_right;
			model.indices.push_back(index + step_down);
			temp.v[2] = index + step_down;
			triangles.push_back(temp);
			count++;

			model.indices.push_back(index + step_down);
			temp.v[0] = index + step_down;
			model.indices.push_back(index + step_right);
			temp.v[1] = index + step_right;
			model.indices.push_back(index + step_down + step_right);
			temp.v[2] = index + step_down + step_right;

			triangles.push_back(temp);
			count++;
		}
	}
	const unsigned int kNumVertices = width*height;
	//create the edge list for our trimesh
	trimesh::unordered_edges_from_triangles(triangles.size(), &triangles[0], edges);
	//build the trimesh
	m_triMesh.build(kNumVertices, triangles.size(), &triangles[0], edges.size(), &edges[0]);
	Mesh::InitMesh(model);
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
	Mesh::Draw(GL_TRIANGLES);
	glEnable(GL_DEPTH_TEST);
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

std::vector<glm::vec3> GridMesh::GetPositions()
{
	return m_model.positions;
}

void GridMesh::UpdatePositions(std::vector<glm::vec3> &pos)
{
	assert(pos.size() == m_model.positions.size());
	m_model.positions = pos;
	UpdateTextureCoords();
}

void GridMesh::print()
{
	glm::vec3 temp;
	for (unsigned int i = 0; i < m_height; i++)
	{
		for (unsigned int j = 0; j < m_width; j++)
		{
			temp = GetPositionOf(j, i);
			std::cout << "For (" << j << "," << i << ") Pos: x: " << temp.x << ", y: " << temp.y << ", z: " << temp.z << "\n";
		}
	}
	std::cout << std::endl;
}

void GridMesh::UpdateNormals()
{

}

void GridMesh::UpdateTextureCoords()
{
	//get the total size that is take the position of 
}