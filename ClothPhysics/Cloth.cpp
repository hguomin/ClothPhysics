#include "Cloth.h"
#include <iostream>

Cloth::Cloth(unsigned int width, unsigned int height, float weight) : GridMesh(height,width)
{
	m_particle_weight = 0.001f; // weight / (width*height);
	m_gravity = glm::vec3(0.0f, -9.81f, 0.0f);
	float	KsStruct = 50.75f, KdStruct = -0.25f;
	float	KsShear = 50.75f, KdShear = -0.25f;
	float	KsBend = 50.95f, KdBend = -0.25f;
	InitGridMesh(height, width);
	//set up positions

	for (unsigned int i = 0; i < height*width; i++)
	{
		m_position.push_back(GetPositionOf(i));
		m_last_position.push_back(GetPositionOf(i));
		m_F.push_back(glm::vec3(0));
	}
	
	
	//adding the springs
	//structural horizontal
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width - 1; x++)
		{
			AddSpring(width*y + x, width*y + x + 1, KsStruct, KdStruct, SpringType::STRUCT);
		}
	}
	//structural vertical
	for (unsigned int y = 0; y < height - 1; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			AddSpring(width*y + x, width*(y + 1) + x, KsStruct, KdStruct, SpringType::STRUCT);
		}
	}

	//Shearing Springs
	for (unsigned int y = 0; y < height - 1; y++)
	{
		for (unsigned int x = 0; x < width - 1; x++)
		{
			AddSpring(width*y + x, width*(y + 1) + x + 1, KsShear, KdShear, SpringType::SHEAR);
			AddSpring(width*(y + 1) + x, width*y  + x + 1, KsShear, KdShear, SpringType::SHEAR);
		}
	}

	//Bend Springs horizontal
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width - 2; x++)
		{
			AddSpring(width*y + x, width*y + x + 2, KsBend, KdBend, SpringType::BEND);
		}
	}
	//Bend Springs Vertical
	for (unsigned int y = 0; y < height - 2; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			AddSpring(width*y + x, width*(y + 2) + x, KsBend, KdBend, SpringType::BEND);
		}
	}

}

Cloth::~Cloth()
{
	m_springs.clear();
}

void Cloth::AddSpring(int pos_a, int pos_b, float spring_const, float damp_const, SpringType type)
{
	Spring temp;
	temp.pos1 = pos_a;
	temp.pos2 = pos_b;
	glm::vec3 delta = GetPositionOf(pos_a) -GetPositionOf(pos_b);
	temp.rest_length = glm::length(delta);
	temp.spring_constant = spring_const;
	temp.damp = damp_const;
	temp.type = type;
	m_springs.push_back(temp);
}

void Cloth::ComputeForce(float dt)
{
	for (unsigned int i = 0; i < m_F.size(); i++)
	{
		m_F[i] = glm::vec3(0);
		glm::vec3 V = GetVelocity(m_position[i], m_last_position[i], dt);
		if (i!= 0 && i != GetWidth()) //lock the top corners
		{
			m_F[i] += m_gravity;
			m_F[i] += -0.0125f*V; //HERE WE CAN HAVE PROBLEMS WITH NO DEFAULT DAMP SIZE
		}
		
	}
	//calculate forces
	for (unsigned int i = 0; i < m_springs.size(); i++)
	{
		Spring spring = m_springs[i];
		glm::vec3 p1 = m_position.at(spring.pos1);
		glm::vec3 p1_last = m_last_position.at(spring.pos1);

		glm::vec3 p2 = m_position.at(spring.pos2);
		glm::vec3 p2_last = m_last_position.at(spring.pos2);

		glm::vec3 v1 = GetVelocity(p1, p1_last, dt);
		glm::vec3 v2 = GetVelocity(p2, p2_last, dt);

		glm::vec3 deltaP = p1 - p2;
		glm::vec3 deltaV = v1 - v2;
		float dist = glm::length(deltaP);

		float leftTerm = -spring.spring_constant *(dist - spring.rest_length);
		float rightTerm = spring.damp * (glm::dot(deltaV, deltaP) / dist);

		glm::vec3 springForce = (leftTerm + rightTerm)*glm::normalize(deltaP);

		if (spring.pos1 != 0 && spring.pos1 != (GetWidth()))
		{
			m_F[spring.pos1] += springForce;
		}
		if (spring.pos2 != 0 && spring.pos2 != (GetHeight()))
		{
			m_F[spring.pos2] -= springForce;
		}
	}
}

void Cloth::Draw()
{
	ComputeForce(0.1f);
	ApplyMovement(0.1f);
	UpdatePositions(m_position);
	GridMesh::Draw();
}

glm::vec3 Cloth::GetVelocity(glm::vec3 p1, glm::vec3 p2, float dt, IntegrationType integType)
{
	if (integType == IntegrationType::VERLET)
	{
		return (p1 - p2) / dt;
	}
	else
	{
		return glm::vec3(0);
	}
}

void Cloth::ApplyMovement(float dt)
{
	float a = dt * m_particle_weight * m_particle_weight;

	for (unsigned int i = 0; i < m_position.size(); i++)
	{
		glm::vec3 temp = m_position[i];
		
		m_position[i] = m_position[i] + (m_position[i] - m_last_position[i]) + a*m_F[i];

		m_last_position[i] = temp;

		if (m_position[i].y < -300)
		{
			m_position[i].y = -300;
		}
	}
}