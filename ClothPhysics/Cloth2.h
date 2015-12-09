#pragma once
#ifndef CLOTH2_H
#define CLOTH2_H

#include <vector>
#include <memory>

#include "Spring.h"
#include "Particle.h"
#include "GridMesh.h"

#define GRAVITY -0.0981f

class Cloth2 : public GridMesh
{
public:
	Cloth2(float width, float height, unsigned int particles_width, unsigned int particles_height);
	~Cloth2();

	void Update(float dt, glm::vec3 wind);

	void SplitVert(unsigned int x, unsigned int y, const glm::vec3 cut_normal = glm::vec3(0, 1, 0));

	void print();
private:
	float m_width;
	float m_height;
	unsigned int m_particles_width;
	unsigned int m_particles_height;
	void makeConstraint(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, SpringType type);
	std::vector<std::shared_ptr<Particle>> m_particles;
	std::vector<std::shared_ptr<Spring>> m_constraints;
	void CutConstraints(std::shared_ptr<Particle> old_particle, std::shared_ptr<Particle> new_particle, const glm::vec3 cut_plane_normal);
	bool ParticleAbovePlane(const std::shared_ptr<Particle> particle, const glm::vec3 plane, const glm::vec3 on_plane);
	bool isBend(const std::shared_ptr<Spring> spr);
	bool BelowPlane(const glm::vec3& test_point, const glm::vec3& point_on_plane, const glm::vec3& plane_normal);

	void Wind(glm::vec3 direction);
	std::vector<glm::vec3> ExtractPositions();
	glm::vec3 CalculateTriangleNormal(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, std::shared_ptr<Particle> p3);
	void AddWind(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, std::shared_ptr<Particle> p3, glm::vec3 direction);
	std::shared_ptr<Particle> getParticleAt(unsigned int x, unsigned int y);
	void TimeStep(float dt);
	void AddForce(glm::vec3& force);

	void ForceConstraints();

	void BallCollision(glm::vec3 position, float radius);
};

#endif //CLOTH2_H