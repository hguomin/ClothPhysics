#include "Cloth2.h"
#include <iostream>

Cloth2::Cloth2(float width, float height, unsigned int particles_width, unsigned int particles_height)
	: GridMesh(particles_height, particles_width), m_width(width), m_height(height), m_particles_width(particles_width), m_particles_height(particles_height)
{
	for (unsigned int y = 0; y < particles_height; y++)
	{
		for (unsigned int x = 0; x < particles_width; x++)
		{
			m_particles.push_back(std::make_shared<Particle>());
		}
	}

	for (unsigned int  y = 0; y < particles_height; y++)
	{
		for (unsigned int  x = 0; x < particles_width; x++)
		{
			glm::vec3 temp = glm::vec3(
				width*((float)x / (float)particles_width),
				-height*((float)y / (float)particles_height),
				0.0f);
			getParticleAt(x, y)->setPosition(temp);
		}
	}
	//create constraints
	for (unsigned int  y = 0; y < particles_height; y++)
	{
		for (unsigned int  x = 0; x < particles_width; x++)
		{
			if (x < (particles_width - 1))
			{
				//particle to the right
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 1, y), SpringType::STRUCT);
			}
				
			if (y < (m_particles_height - 1))
			{
				//particle below
				makeConstraint(getParticleAt(x, y), getParticleAt(x , y+1), SpringType::STRUCT);
			}
			if (x < (particles_width -1) && y < (particles_height -1))
			{
				//making the cross
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 1 , y + 1), SpringType::SHEAR);
				makeConstraint(getParticleAt(x + 1, y), getParticleAt(x, y + 1), SpringType::SHEAR);
			}
		}
	}

	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			if (x < particles_width -2)
			{
				//to the right
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 2 , y), SpringType::BEND);
			}
			if (y < particles_height -2)
			{
				//below
				makeConstraint(getParticleAt(x , y), getParticleAt(x , y + 2), SpringType::BEND);
			}
			if (x < (particles_width -2) && y < (particles_height -2))
			{
				//the cross
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 2, y + 2), SpringType::BEND);
				makeConstraint(getParticleAt(x + 2 , y),getParticleAt(x, y + 2), SpringType::BEND);
			}
		}
	}

	//making the top three on each side fixed
	getParticleAt(0, 0)->makeUnmovable();
	
	//getParticleAt(0, m_particles_width - 1)->makeUnmovable();
	
	for (unsigned int i = 0; i < m_width; i++)
	{
		//getParticleAt(m_height -1 , i)->setPosition(glm::vec3(-5,0,-20));
	}
	
	//getParticleAt(1, 1)->setPosition(glm::vec3(1, -1, 1));
	
	GridMesh::UpdatePositions(ExtractPositions());
}


Cloth2::~Cloth2()
{
}

void Cloth2::makeConstraint(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, SpringType type)
{
	m_constraints.push_back(std::make_shared<Spring>(p1, p2, type));
}

std::shared_ptr<Particle> Cloth2::getParticleAt(unsigned int x, unsigned int y)
{
	return m_particles.at(x + m_particles_width*y);
}

std::vector<glm::vec3> Cloth2::ExtractPositions()
{
	std::vector<glm::vec3> temp;

	for (auto it = m_particles.cbegin(); it!=m_particles.cend(); it++)
	{
		temp.push_back(*(*it)->getPosition());
	}
	return temp;
}

void Cloth2::TimeStep()
{

	for (unsigned int i = 0; i < 20; i++) //number of times we want to iterate over the constraints
	{
		for (auto it = m_constraints.cbegin(); it != m_constraints.cend(); it++)
		{
			(*it)->SatisfyConstraints();
		}
	}

	for (auto it = m_particles.cbegin(); it != m_particles.cend(); it++)
	{
		(*it)->timeStep(0.1f, 1.0f); //this could go wrong
	}
}

void Cloth2::AddForce(glm::vec3 &force)
{
	for (auto it = m_particles.cbegin(); it < m_particles.cend(); it++)
	{
		(*it)->addForce(force);
	}
}

void Cloth2::Update()
{
	AddForce(glm::vec3(0, -9.81f, 0));
	//Wind(glm::vec3(0, 0, -1));
	TimeStep();
	GridMesh::UpdatePositions(ExtractPositions());
	GridMesh::UpdateModel(GL_DYNAMIC_DRAW);
	GridMesh::Draw();
}

void Cloth2::print()
{
	std::vector<glm::vec3> temp = ExtractPositions();

	glm::vec3 i = *temp.cend();
		std::cout << i.x << " " << i.y << " " << i.z << "\n";
	std::cout << std::endl;
	GridMesh::print();
}

glm::vec3 Cloth2::CalculateTriangleNormal(Particle* p1, Particle* p2, Particle* p3)
{
	glm::vec3 pos1 = *p1->GetPosition();
	glm::vec3 pos2 = *p2->GetPosition();
	glm::vec3 pos3 = *p3->GetPosition();

	glm::vec3 v1 = pos2 - pos1;
	glm::vec3 v2 = pos3 - pos1;

	return glm::cross(v1, v2);
}

void Cloth2::AddWind(Particle* p1, Particle* p2, Particle* p3, glm::vec3 direction)
{
	glm::vec3 normal = CalculateTriangleNormal(p1, p2, p3);
	glm::vec3 d = glm::normalize(normal);
	glm::vec3 force = normal*glm::dot(d, direction);
	p1->addForce(force);
	p2->addForce(force);
	p3->addForce(force);
}

void Cloth2::Wind(glm::vec3 direction)
{
	for (unsigned int x = 0; x < m_particles_width -1; x++)
	{
		for (unsigned int y = 0; y < m_particles_height -1; y++)
		{
			AddWind(getParticleAt(x, y).get(), getParticleAt(x + 1, y).get(), getParticleAt(x, y + 1).get(),direction);
			AddWind(getParticleAt(x + 1, y).get(), getParticleAt(x, y + 1).get(), getParticleAt(x + 1, y + 1).get(), direction);
		}
	}
}