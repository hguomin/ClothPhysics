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
	SplitVertex(1, 1);
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

	/*going to use this quite a bit so doing some typedefs*/
	typedef trimesh::trimesh_t::halfedge_t halfedge;

	/*Changes to be implemented*/
	std::vector<halfedge> changes_to_save;

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
	std::vector<trimesh::index_t> faces_above_plane;
	std::vector<trimesh::index_t> faces_below_plane;
	for (unsigned int i = 0; i < face_centers.size(); i++)
	{
		float projection = glm::dot(plane_normal, plane_point - face_centers.at(i).first);
		if (projection > 0)
		{
			//above plane
			faces_above_plane.push_back(face_centers.at(i).second);
		}
		else if (projection < 0)
		{
			//below plane
			faces_below_plane.push_back(face_centers.at(i).second);
		}
		else
		{
			//on plane
			faces_below_plane.push_back(face_centers.at(i).second);
		}
	}

	/*TODO: */
	//create new particle at the same point as the splitting point
	
	std::vector<trimesh::index_t> half_edges_above_index;
	std::vector<trimesh::index_t> half_edges_below_index;

	//first contains above, second contains below
	//std::vector<std::pair<halfedge, halfedge>> splitting_edges;

	//find the edges that seperate the faces above and the faces below
	for (unsigned int i = 0; i < faces_above_plane.size(); i++)
	{
		//so all edges for the triangles above is saved in half_edge_above
		auto temp = m_triMesh.halfedge_for_face(faces_above_plane.at(i));
		half_edges_above_index.insert(half_edges_above_index.end(), temp.begin(), temp.end());
	}
	for (unsigned int below = 0; below < faces_below_plane.size(); below++)
	{
		//so all edges for the triangles below is saved in half_edge_below
		auto temp = m_triMesh.halfedge_for_face(faces_below_plane.at(below));
		half_edges_below_index.insert(half_edges_below_index.end(), temp.begin(), temp.end());
	}
	
	//compare the edges for one triangle above and one triangle below. 
	//If the edge is the same then we have a pair. And that will be our splitting edge
	//std::vector<std::pair<halfedge, halfedge>> splitting_edges;
	for each (trimesh::index_t above_ind in half_edges_above_index)
	{
		halfedge above = m_triMesh.halfedge(above_ind);
		for each (trimesh::index_t below_ind in half_edges_below_index)
		{
			halfedge below = m_triMesh.halfedge(below_ind);
			if (above.edge == below.edge)
			{
			
				above.ghost_he = above.opposite_he;
				below.ghost_he = below.opposite_he;

				above.opposite_he = -1;
				below.opposite_he = -1;
				m_triMesh.save_he(above);
				m_triMesh.save_he(below);
			}
			else if ((above.ghost_he != -1) && (below.ghost_he != -1) && (above.ghost_he == below.ghost_he))
			{
				
				above.ghost_he = above.opposite_he;
				below.ghost_he = below.opposite_he;

				above.opposite_he = -1;
				below.opposite_he = -1;
				m_triMesh.save_he(above);
				m_triMesh.save_he(below);
			}
		}
	}
	
	//now update the halfedges below so that they point to/from the new vertice
	for (unsigned int i = 0; i < half_edges_below_index.size(); i++)
	{
		auto half_edge = m_triMesh.get_he_at_heindex(half_edges_below_index.at(i));
		if (half_edge.to_vertex == index)
		{
			half_edge.to_vertex = -15; //size +1
			m_triMesh.save_he(half_edge);
		}
		else if (half_edge.from_vertex == index)
		{
			half_edge.from_vertex = -15; //size +1
			m_triMesh.save_he(half_edge);
		}
	}
	std::cout << "hej" << std::endl;
	/*TRYING STUFF HERE*/
}