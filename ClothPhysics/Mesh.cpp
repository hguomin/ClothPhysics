#include "Mesh.h"
#include <vector>
#include <string>
#include <iostream>


Mesh::Mesh(Vertex* vertices, unsigned int numVertices, unsigned int* indices, unsigned int numIndices, GLenum DRAWTYPE) : m_DRAWTYPE(DRAWTYPE)
{
	IndexedModel m_model = CreateIndexedModel(vertices, numVertices, indices, numIndices);

	InitMesh(m_model);
}

Mesh::Mesh(const std::string& fileName, GLenum DRAWTYPE) : m_DRAWTYPE(DRAWTYPE)
{
	m_model = OBJModel(fileName).ToIndexedModel();
	InitMesh(m_model);
}


Mesh::~Mesh()
{
	glDeleteVertexArrays(1, &m_vertexArrayObject);
}


void Mesh::Draw()
{
	glBindVertexArray(m_vertexArrayObject);

	glDrawElements(GL_TRIANGLES, m_drawCount, GL_UNSIGNED_INT, 0);
	//glDrawArrays(GL_TRIANGLES, 0, m_drawCount);

	glBindVertexArray(0);
}

void Mesh::StandardDraw()
{
	glBindVertexArray(m_vertexArrayObject);

	glDrawElements(GL_TRIANGLES, m_drawCount, GL_UNSIGNED_INT, 0);
	//glDrawArrays(GL_TRIANGLES, 0, m_drawCount);

	glBindVertexArray(0);
}

void Mesh::UploadToGPU()
{
	glBindVertexArray(m_vertexArrayObject);
	//Position
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[POSITION_VB]);
	glBufferData(GL_ARRAY_BUFFER, m_model.positions.size() * sizeof(m_model.positions[0]), &m_model.positions[0], m_DRAWTYPE); //sizeof(vertices[0]) gives size of vertex

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//Texture
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[TEXCOORD_VB]);
	glBufferData(GL_ARRAY_BUFFER, m_model.texCoords.size() * sizeof(m_model.texCoords[0]), &m_model.texCoords[0], m_DRAWTYPE); //sizeof(texCoords[0]) gives size of vertex

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	//Normal
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexArrayBuffers[NORMAL_VB]);
	glBufferData(GL_ARRAY_BUFFER, m_model.normals.size() * sizeof(m_model.normals[0]), &m_model.normals[0], m_DRAWTYPE); //sizeof(vertices[0]) gives size of vertex

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//Indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vertexArrayBuffers[INDEX_VB]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_model.indices.size() * sizeof(m_model.indices[0]), &m_model.indices[0], m_DRAWTYPE); //sizeof(vertecies[0]) gives size of vertex

}

void Mesh::InitMesh(const IndexedModel& model, bool updateModel, GLenum DRAWTYPE)
{
	m_model = model;
	m_drawCount = model.indices.size(); //makes sure that that we have the correct draw count
	m_DRAWTYPE = DRAWTYPE;

	if (!updateModel)
	{
		GenerateBufferAndVertexArray();
	}

	UploadToGPU();
}

IndexedModel Mesh::CreateIndexedModel(Vertex* vertices, unsigned int numVertices, unsigned int* indices, unsigned int numIndices)
{
	IndexedModel model;

	for (unsigned int i = 0; i < numVertices; i++)
	{
		model.positions.push_back(*vertices[i].GetPosition());
		model.texCoords.push_back(*vertices[i].GetTexCoord());
		model.normals.push_back(*vertices[i].GetNormal());
	}
	for (unsigned int i = 0; i < numVertices; i++)
	{
		model.indices.push_back(indices[i]);
	}
	return model;
}

void Mesh::UpdateModel(GLenum DRAWTYPE)
{
	InitMesh(m_model,true);
}

void Mesh::GenerateBufferAndVertexArray()
{
	glGenVertexArrays(1, &m_vertexArrayObject);
	glGenBuffers(NUM_BUFFERS, m_vertexArrayBuffers);
}