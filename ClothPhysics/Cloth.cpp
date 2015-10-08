#include "Cloth.h"
#include <iostream>

Cloth::Cloth(unsigned int width, unsigned int height) : m_width(width), m_height(height)
{
	float spK = 1.0f; //spring constant
	float dpK = 1.0f; //damp constant
	//init positions of the cloth particles
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			m_pPos.push_back(glm::vec3(x, y, 0));
			m_prevPPos.push_back(glm::vec3(x, y, 0));
		}
	}
	//adding the springs

	//structural horizontal
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width - 1; x++)
		{
			AddSpring(width*y + x, width*y + x + 1, spK, dpK, SpringType::STRUCT);
		}
	}
	//structural vertical
	for (unsigned int y = 0; y < height - 1; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			AddSpring(width*y + x, width*(y + 1) + x, spK, dpK, SpringType::STRUCT);
		}
	}

	//Shearing Springs
	for (unsigned int y = 0; y < height - 1; y++)
	{
		for (unsigned int x = 0; x < width - 1; x++)
		{
			AddSpring(width*y + x, width*(y + 1) + x + 1, spK, dpK, SpringType::SHEAR);
			AddSpring(width*(y + 1) + x, width*y  + x + 1, spK, dpK, SpringType::SHEAR);
		}
	}

	//Bend Springs horizontal
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width - 2; x++)
		{
			AddSpring(width*y + x, width*y + x + 2, spK, dpK, SpringType::BEND);
		}
	}
	//Bend Springs Vertical
	for (unsigned int y = 0; y < height - 2; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			AddSpring(width*y + x, width*(y + 2) + x, spK, dpK, SpringType::BEND);
		}
	}

}

Cloth::~Cloth()
{
	m_springs.clear();
	m_pPos.clear();
	m_prevPPos.clear();
}

void Cloth::AddSpring(int pos_a, int pos_b, float spring_const, float damp_const, SpringType type)
{
	Spring temp;
	temp.pos1 = pos_a;
	temp.pos2 = pos_b;
	glm::vec3 delta = m_pPos[pos_a] - m_pPos[pos_b];
	temp.rest_length = glm::length(delta);
	temp.spring_constant = spring_const;
	temp.damp = damp_const;
	temp.type = type;
	m_springs.push_back(temp);
}

void Cloth::print()
{
	std::cout << m_springs.size() << std::endl;
}