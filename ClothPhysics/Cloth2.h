#pragma once
#ifndef CLOTH2_H
#define CLOTH2_H

#include <vector>
#include <memory>

#include "Spring.h"
#include "Particle.h"
#include "GridMesh.h"

class Cloth2 : public GridMesh
{
public:
	Cloth2(float width, float height, unsigned int particles_width, unsigned int particles_height);
	~Cloth2();

	std::shared_ptr<Particle> getParticleAt(unsigned int x, unsigned int y);
	void TimeStep();
	void AddForce(glm::vec3& force);

	void Update();

	void Wind(glm::vec3 direction);

	void print();
private:
	float m_width;
	float m_height;
	unsigned int m_particles_width;
	unsigned int m_particles_height;
	void makeConstraint(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, SpringType type);
	std::vector<std::shared_ptr<Particle>> m_particles;
	std::vector<std::shared_ptr<Spring>> m_constraints;

	std::vector<glm::vec3> ExtractPositions();
	glm::vec3 CalculateTriangleNormal(Particle* p1, Particle* p2, Particle* p3);
	void AddWind(Particle* p1, Particle* p2, Particle* p3, glm::vec3 direction);
};

#endif //CLOTH2_H