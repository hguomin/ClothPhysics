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
private:
	std::shared_ptr<Particle> p1;
	std::shared_ptr<Particle> p2;
	SpringType type;
	float rest_length;
	float spring_constant;
	float invmass1;
	float invmass2;
};

#endif