#include "Cloth2.h"
#include <iostream>

Cloth2::Cloth2(float width, float height, unsigned int particles_width, unsigned int particles_height)
	: GridMesh(particles_height, particles_width), m_width(width), m_height(height), m_particles_width(particles_width), m_particles_height(particles_height)
{
	for (unsigned int x = 0; x < particles_width; x++)
	{
		for (unsigned int y = 0; y < particles_height; y++)
		{
			m_particles.push_back(std::make_shared<Particle>());
		}
	}

	for (unsigned int  x = 0; x < particles_width; x++)
	{
		for (unsigned int  y = 0; y < particles_height; y++)
		{
			glm::vec3 temp = glm::vec3(
				width*((float)x / (float)particles_width),
				-height*((float)y / (float)particles_height),
				0.0f);
			getParticleAt(x, y)->setPosition(temp);
		}
	}
	//create constraints
	for (unsigned int  x = 0; x < particles_width; x++)
	{
		for (unsigned int  y = 0; y < particles_height; y++)
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

	for (unsigned int x = 0; x < width; x++)
	{
		for (unsigned int y = 0; y < height; y++)
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
	
	for (unsigned int i = 0; i < 3; i++)
	{
		getParticleAt(i,0)->makeUnmovable();

		getParticleAt(m_particles_width - i, 0)->makeUnmovable();
	}
	for (unsigned int i = 0; i < m_particles_width; i++)
	{
		getParticleAt(i, 2)->addPosition(glm::vec3(0, 5, 10));
	}
	
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

	for (unsigned int i = 0; i < 10; i++) //number of times we want to iterate over the constraints
	{
		for (auto it = m_constraints.cbegin(); it != m_constraints.cend(); it++)
		{
			(*it)->SatisfyConstraints();
		}
	}

	for (auto it = m_particles.cbegin(); it != m_particles.cend(); it++)
	{
		(*it)->timeStep(0.1f, 0.8f); //this could go wrong
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
	AddForce(glm::vec3(0, -9.81, 0));
	TimeStep();
	GridMesh::UpdatePositions(ExtractPositions());
	GridMesh::UpdateModel(GL_DYNAMIC_DRAW);
	GridMesh::Draw();
}

void Cloth2::print()
{
	std::vector<glm::vec3> temp = ExtractPositions();

	for each (glm::vec3 i in temp)
	{
		std::cout << i.x << " " << i.y << " " << i.z << "\n";
	}
	std::cout << std::endl;
}