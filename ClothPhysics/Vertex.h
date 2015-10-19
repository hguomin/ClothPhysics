#pragma once
#ifndef VERTEX_H
#define VERTEX_H
#include "glm\glm.hpp"
class Vertex
{
public:
	Vertex(const glm::vec3& pos, const glm::vec2& texCoord, const glm::vec3& normal = glm::vec3(0, 0, 0))
	{
		this->m_position = pos;
		this->m_texCoord = texCoord;
		this->m_normal = normal;
	}

	glm::vec3* GetPosition() { return &m_position; }
	glm::vec2* GetTexCoord() { return &m_texCoord; }
	glm::vec3* GetNormal() { return &m_normal; }
	
protected:
	
	void setPos(glm::vec3 &new_pos) { m_position = new_pos; }
	void setNormal(glm::vec3 &new_normal) { m_normal = new_normal; }
	void setTexCoord(glm::vec2 &new_texCoord) { m_texCoord = new_texCoord; }
	glm::vec3 m_position;
	glm::vec2 m_texCoord;
	glm::vec3 m_normal;
private:
	
};

#endif //VERTEX_H