#pragma once
#ifndef CLOTH_H
#define CLOTH_H

#include <vector>
#include "glm\glm.hpp"
#include "GridMesh.h"

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
	Cloth(unsigned int width, unsigned int height);
	~Cloth();

	void AddSpring(int pos_a, int pos_b, float force_constat, float damp_constant, SpringType type);

	void print();

private:
	unsigned int m_width;
	unsigned int m_height;

	std::vector<Spring> m_springs;

};

#endif //CLOTH_H