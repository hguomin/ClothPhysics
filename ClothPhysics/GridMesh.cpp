#include "GridMesh.h"
#include "Half-Edge(notMine)\trimesh.h"
#include <iostream>
#include <utility>

GridMesh::GridMesh()
{

}

GridMesh::GridMesh(unsigned int height, unsigned int width) : m_height(height), m_width(width)
{
	InitGridMesh(height, width);
	print();
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
	SplitVertex(0, 0);
	/*
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
	*/
}

void GridMesh::UpdateNormals()
{

}

void GridMesh::UpdateTextureCoords()
{
	//get the total size that is take the position of 
}

void GridMesh::SplitVertex(unsigned int v_X, unsigned int v_Y)
{
	const unsigned int index = v_X + v_Y*m_width;
	std::vector<trimesh::index_t> neigh_faces;
	//get the neighbour faces saved in neigh_faces
	m_triMesh.vertex_face_neighbors(index, neigh_faces);

	//trying to find the centers of each face saving the face information
	std::vector<std::pair<glm::vec3,trimesh::index_t>> face_centers;
	for (unsigned int face = 0; face < neigh_faces.size(); face++)
	{
		//extract the vertices for the current face
		std::vector<trimesh::index_t> vertices;
		trimesh::index_t face_index = neigh_faces.at(face);
		m_triMesh.vertices_for_face(face_index, vertices);
		//finding the index positions of the vertices
		trimesh::index_t p1_index = vertices[0];
		trimesh::index_t p2_index = vertices[1];
		trimesh::index_t p3_index = vertices[2];
		//getting the world coordinates for the vertices
		glm::vec3 p1 = m_model.positions[p1_index];
		glm::vec3 p2 = m_model.positions[p2_index];
		glm::vec3 p3 = m_model.positions[p3_index];
		//getting averaged middle and save it
		glm::vec3 center = (p1 + p2 + p3)*0.33f;
		face_centers.push_back(std::make_pair(center,face_index));
	}

	//get what side the center triangle point is on
	glm::vec3 plane_normal = glm::vec3(0, 1, 0);
	glm::vec3 plane_point = glm::vec3(1, 1, 0); //in the exact middle
	std::vector<trimesh::index_t> above_plane;
	std::vector<trimesh::index_t> below_plane;
	for (unsigned int i = 0; i < face_centers.size(); i++)
	{
		float projection = glm::dot(plane_normal, plane_point - face_centers.at(i).first);
		if (projection > 0)
		{
			//above plane
			above_plane.push_back(face_centers.at(i).second);
		}
		else if (projection < 0)
		{
			//below plane
			below_plane.push_back(face_centers.at(i).second);
		}
		else
		{
			//on plane
			below_plane.push_back(face_centers.at(i).second);
		}
	}





	/*TODO: */
	//create new particle at the same point as the splitting point
	glm::vec3 old_pos = m_model.positions.at(index);
	glm::vec3 old_normal = m_model.normals.at(index);
	glm::vec2 old_texCoord = m_model.texCoords.at(index);
	
	
	//new particle will have edges from above the splitting plane
	//old particle wwill have edges from belo the splitting plane
	//(this means that we have to have someway of updating the indices list nicely.)

	//update the trianglemesh, particles, normal mesh

	std::vector<trimesh::trimesh_t::halfedge_t> half_edge;
	for (unsigned int i = 0; i < neigh_faces.size(); i++)
	{
		half_edge = m_triMesh.halfedge_for_face(neigh_faces.at(i));

		for (unsigned int j = 0; j < half_edge.size(); j++)
		{
			std::cout << "Next Half_edge: " << half_edge.at(j).next_he << " " << half_edge.at(j).opposite_he  << std::endl;
		}
	}
	
	/*
	std::vector< trimesh::index_t > neighs;
	const unsigned int index = v_X + v_Y*m_width;
	m_triMesh.vertex_face_neighbors(index, neighs);
	for (unsigned int  j = 0; j < neighs.size(); j++)
	{
		std::vector<trimesh::index_t > face_ver;
		m_triMesh.vertices_for_face(neighs.at(j), face_ver);
		std::cout << "Indecies for face: " << neighs.at(j) << std::endl;
		for (unsigned int i = 0; i < face_ver.size(); i++)
		{
			std::cout << face_ver.at(i) << " ";
		}
		std::cout << std::endl;
	}
	*/
}