#pragma once
#ifndef CLOTH2_H
#define CLOTH2_H

#include <vector>
#include <memory>

#include "Spring.h"
#include "Particle.h"
#include "GridMesh.h"

#define GRAVITY -0.0981f

class Cloth_CPU : public GridMesh
{
public:
	Cloth_CPU(float width, float height, unsigned int particles_width, unsigned int particles_height);
	~Cloth_CPU();

	void Update(float dt, glm::vec3 wind, unsigned int numIterations);

	void print();
	void CalculatePerVertexNormals();

	void UpdateTextureCoordinates();

	void Upload();

	glm::vec3 ball_position;
	float ball_size;

	void setStartPosition();

private:
	float m_width;
	float m_height;
	unsigned int m_particles_width;
	unsigned int m_particles_height;
	void makeConstraint(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, SpringType type);
	std::vector<std::shared_ptr<Particle>> m_particles;
	std::vector<std::shared_ptr<Spring>> m_constraints;


	void Wind(glm::vec3 direction);
	std::vector<glm::vec3> ExtractPositions();
	std::vector<glm::vec3> ExtractNormals();
	glm::vec3 CalculateTriangleNormal(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, std::shared_ptr<Particle> p3);
	void AddWind(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, std::shared_ptr<Particle> p3, glm::vec3 direction);
	std::shared_ptr<Particle> getParticleAt(unsigned int x, unsigned int y);
	void TimeStep(float dt, unsigned int numIterations);
	void AddForce(glm::vec3& force);

	void ForceConstraints(unsigned int numIterations);

	

	void BallCollision();
};

#endif //CLOTH2_H