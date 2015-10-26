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
	Spring(std::shared_ptr<Particle> part1, std::shared_ptr<Particle> part2, SpringType Spring_type) : p1(part1), p2(part2), type(Spring_type)
	{
		rest_length = glm::length(*p1->getPosition() - *p2->getPosition());
	};
	~Spring() {};
	void SatisfyConstraints()
	{
		glm::vec3 p1_p2 = *p2->getPosition() - *p1->getPosition();
		float current_length= glm::length(p1_p2);
		glm::vec3 correction = p1_p2*(1 - rest_length / current_length);
		glm::vec3 halfCorrection = 0.5f*correction;
		p1.get()->addPosition(halfCorrection);
		p2.get()->addPosition(-halfCorrection);
		//reset the forces and acceleration working on the particles
	}
private:
	std::shared_ptr<Particle> p1;
	std::shared_ptr<Particle> p2;
	SpringType type;
	float rest_length;
	float spring_constant;
	
};

#endif