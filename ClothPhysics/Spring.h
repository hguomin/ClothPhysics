#pragma once
#ifndef SPRING_H
#define SPRING_H
#include <memory>
#include "glm\glm.hpp"
#include "Particle.h"
enum class SpringType { SHEAR, BEND, STRUCT };

class Spring
{
public:
	Spring(std::shared_ptr<Particle> part1, std::shared_ptr<Particle> part2, SpringType Spring_type, float spring_constant = 1.0f, float damp_constant = 0.0f)
		: p1(part1), p2(part2), type(Spring_type), m_springConst(spring_constant), m_dampConst(damp_constant)
	{
		rest_length = glm::length(p1->getPosition() - p2->getPosition());
		invmass1 = 1 / part1->getMass();
		invmass2 = 1 / part2->getMass();
	};
	~Spring() {};
	void SatisfyConstraints()
	{
		glm::vec3 delta = p2->getPosition() - p1->getPosition();
		float delta_length= glm::length(delta);
		float diff = (delta_length - rest_length) / (delta_length*(invmass1 + invmass2));
		p1->addPosition(invmass1*delta*diff);
		p2->addPosition(-invmass2*delta*diff);
	}

	void InternalForces()
	{
		//spring force
		glm::vec3 delta = p2->getPosition() - p1->getPosition();
		float delta_length = glm::length(delta);
		glm::vec3 spring_force = m_springConst*(delta_length - rest_length)*delta / delta_length;

		p1->addForce(spring_force);
		p2->addForce(-spring_force);
		//damper force
		glm::vec3 damp_force = m_dampConst*(p2->getVelocity() - p1->getVelocity());
		p1->addForce(damp_force);
		p2->addForce(-damp_force);
	}
private:
	std::shared_ptr<Particle> p1;
	std::shared_ptr<Particle> p2;
	SpringType type;
	float m_springConst;
	float m_dampConst;
	float rest_length;
	float invmass1;
	float invmass2;
};

#endif