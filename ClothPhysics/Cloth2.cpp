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
				0.0f,
				-height*((float)y / (float)particles_height));
			getParticleAt(x, y)->setPosition(temp);
			getParticleAt(x, y)->setLastPosition(temp);
		}
	}
	//create constraints
	for (unsigned int  y = 0; y < particles_height; y++)
	{
		for (unsigned int  x = 0; x < particles_width; x++)
		{
			if (x < (particles_width - 1)) //check this
			{
				//particle to the right
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 1, y), SpringType::STRUCT);
			}
				
			if (y < (m_particles_height - 1))
			{
				//particle below
				makeConstraint(getParticleAt(x, y), getParticleAt(x , y + 1), SpringType::STRUCT);
			}
			if (x < (particles_width -1) && y < (particles_height -1))
			{
				//making the cross
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 1 , y + 1), SpringType::SHEAR);
				makeConstraint(getParticleAt(x + 1, y), getParticleAt(x, y + 1), SpringType::SHEAR);
			}
		}
	}

	//bending particles
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
	getParticleAt(m_particles_width - 1, 0)->makeUnmovable();
	
	GridMesh::UpdatePositions(ExtractPositions());
}


Cloth2::~Cloth2()
{
	std::cout << "Cloth2 Destroyed" << std::endl;
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
		temp.push_back((*it)->getPosition());
	}
	return temp;
}

void Cloth2::TimeStep(float dt)
{
	ForceConstraints();
	for (auto it = m_particles.cbegin(); it != m_particles.cend(); it++)
	{
		(*it)->timeStep(dt, 0.1f); //this could go wrong
	}
	
}

void Cloth2::ForceConstraints()
{
	for (unsigned int i = 0; i < 3; i++) //number of times we want to iterate over the constraints
	{
		for (auto it = m_constraints.cbegin(); it != m_constraints.cend(); it++)
		{
			(*it)->SatisfyConstraints();
			//(*it)->InternalForces();
		}
	}
}

void Cloth2::AddForce(glm::vec3 &force)
{
	for (auto it = m_particles.cbegin(); it < m_particles.cend(); it++)
	{
		(*it)->addForce(force);
	}
}

void Cloth2::Update(float dt, glm::vec3 wind)
{
	AddForce(glm::vec3(0, GRAVITY, 0));
	Wind(wind);
	TimeStep(dt);

	BallCollision(glm::vec3(5,-5.0f,-5),3.0f);

	GridMesh::UpdatePositions(ExtractPositions());
	//GridMesh::SplitVertex(1, 1);
}
void Cloth2::SplitVert(unsigned int x, unsigned int y, const glm::vec3 cut_normal)
{
	const unsigned int index = x + m_particles_width*y;
	auto old_particle = m_particles.at(index);

	if (GridMesh::SplitVertex(x, y, old_particle->getPosition(), cut_normal))
	{
		//update old particle mass
		old_particle->setMass(old_particle->getMass()*0.5f);
		//copy content of old_particle and create new_particle
		auto new_particle = std::make_shared<Particle>(*old_particle);

		m_particles.push_back(new_particle);
		CutConstraints(old_particle, new_particle, cut_normal);
		GridMesh::m_model.positions.push_back(new_particle->getPosition());
		GridMesh::UpdateIndices();
	}
}

bool Cloth2::isBend(const std::shared_ptr<Spring> spr)
{
	if (spr->getType() == SpringType::BEND)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Cloth2::BelowPlane(const glm::vec3& test_point, const glm::vec3& point_on_plane, const glm::vec3& plane_normal)
{
	float projection = glm::dot(plane_normal, point_on_plane - test_point);
	if (projection <= 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Cloth2::CutConstraints(std::shared_ptr<Particle> old_particle, std::shared_ptr<Particle> new_particle, const glm::vec3 cut_plane_normal)
{
	//typedef cause this is used quite a bit
	typedef std::shared_ptr<Spring> spr;
	std::vector<spr> part1;
	std::vector<spr> part2;
	
	//going backwards because we don't want to miss anything when we erease the bend
	for (unsigned int i = m_constraints.size() -1 ; i > 0; i--)
	{
		spr constraint = m_constraints.at(i);
		if (constraint->p1 == old_particle)
		{
			if (isBend(constraint))
			{
				m_constraints.erase(m_constraints.begin() + i);
			}
			else if (BelowPlane(constraint->p2->getPosition(), old_particle->getPosition(),cut_plane_normal))
			{
				part1.push_back(constraint);
			}
		}
		else if (constraint->p2 == old_particle)
		{
			if (isBend(constraint))
			{
				m_constraints.erase(m_constraints.begin() + i);
			}
			else if (BelowPlane(constraint->p1->getPosition(), old_particle->getPosition(), cut_plane_normal))
			{
				part2.push_back(constraint);
			}
		}
	}

	for each (spr constraint in part1)
	{
		constraint->setParticle_1(new_particle);
	}
	for each (spr constraint in part2)
	{
		constraint->setParticle_2(new_particle);
	}
}

bool Cloth2::ParticleAbovePlane(const std::shared_ptr<Particle> particle, const glm::vec3 plane, const glm::vec3 on_plane)
{
		float projection = glm::dot(plane, on_plane - particle->getPosition());
		if (projection > 0)
		{
			//above plane
			return true;
		}
		else if (projection < 0)
		{
			//below plane
			return false;
		}
		else
		{
			//on plane
			return false;
		}
}

void Cloth2::print()
{
	std::vector<glm::vec3> temp = ExtractPositions();

	glm::vec3 i = temp[8];
		std::cout << i.x << " " << i.y << " " << i.z << "\n";
	std::cout << std::endl;
	//GridMesh::print();
}

glm::vec3 Cloth2::CalculateTriangleNormal(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, std::shared_ptr<Particle> p3)
{
	glm::vec3 pos1 = *p1->GetPosition();
	glm::vec3 pos2 = *p2->GetPosition();
	glm::vec3 pos3 = *p3->GetPosition();

	glm::vec3 v1 = pos2 - pos1;
	glm::vec3 v2 = pos3 - pos1;

	glm::vec3 ret = glm::cross(v1, v2);

	return ret;
}

void Cloth2::AddWind(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, std::shared_ptr<Particle> p3, glm::vec3 direction)
{
	glm::vec3 normal = CalculateTriangleNormal(p1, p2, p3);
	glm::vec3 d = glm::normalize(normal);
	glm::vec3 force = normal*glm::dot(d, direction);
	p1->addForce(force);
	p1->addNormal(d);
	
	p2->addForce(force);
	p2->addNormal(d);
	
	p3->addForce(force);
	p3->addNormal(d);
}

void Cloth2::Wind(glm::vec3 direction)
{
	for (unsigned int x = 0; x < m_particles_width -1; x++)
	{
		for (unsigned int y = 0; y < m_particles_height -1; y++)
		{
			AddWind(getParticleAt(x, y), getParticleAt(x + 1, y), getParticleAt(x, y + 1),direction);
			AddWind(getParticleAt(x + 1, y), getParticleAt(x, y + 1), getParticleAt(x + 1, y + 1), direction);
		}
	}
}

void Cloth2::BallCollision(glm::vec3 position, float radius)
{
	for each (std::shared_ptr<Particle> particle in m_particles)
	{
		glm::vec3 forceVector = particle->getPosition() - position;
		if (glm::length(forceVector) < radius)
		{
			forceVector = (forceVector / glm::length(forceVector))* radius;
			particle->setPosition(position + forceVector);
		}
	}
}