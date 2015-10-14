#pragma once
#ifndef CLOTH_H
#define CLOTH_H

#include <vector>
#include "glm\glm.hpp"
#include "GridMesh.h"

enum class IntegrationType {VERLET};
enum class SpringType { SHEAR, BEND, STRUCT };

struct Spring
{
	int pos1;
	int pos2;
	float rest_length;
	float spring_constant;
	float damp;
	SpringType type;
};

class Cloth : public GridMesh
{
public:
	Cloth(unsigned int width, unsigned int height, float weight);
	~Cloth();

	void AddSpring(int pos_a, int pos_b, float force_constat, float damp_constant, SpringType type);

	void Draw();

	glm::vec3 GetVelocity(glm::vec3 p1, glm::vec3 p2, float dt, IntegrationType integType = IntegrationType::VERLET);
	
	void ApplyMovement(float dt);
	void ComputeForce(float dt);

private:

	glm::vec3 m_gravity;
	float m_particle_weight;

	std::vector<glm::vec3> m_position;
	std::vector<glm::vec3> m_last_position;
	std::vector<glm::vec3> m_F;
	std::vector<Spring> m_springs;

};

#endif //CLOTH_H