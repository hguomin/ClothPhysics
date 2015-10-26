#pragma once
#ifndef PARTICLE_H
#define PARTICLE_H
#include "glm\glm.hpp"
#include "Vertex.h"

class Particle :  public Vertex
{
public:
	Particle() :  Vertex(glm::vec3(0),glm::vec2(0)), m_mass(1.0f), m_acceleration(0), m_lastPosition(glm::vec3(0)), m_velocity(0), m_movable(true){};
	~Particle() {};

	void timeStep(float dt, float dampening);

	void addForce(glm::vec3 &force) { m_acceleration += force/m_mass; }
	void addPosition(glm::vec3 & pos) { if (m_movable) { m_position += pos; }}

	void setMass(float mass) { m_mass = mass; };
	void setAcceleration(glm::vec3 acceleration) { m_acceleration = acceleration; }
	void setVelocity(glm::vec3 velocity) { m_velocity = velocity; };
	void setPosition(glm::vec3 position) { m_position = position; };
	void setLastPosition(glm::vec3 position) { m_lastPosition = position; }

	float getMass() { return m_mass; }
	glm::vec3* getVelocity() { return &m_velocity; }
	glm::vec3* getAcceleration() { return &m_acceleration; }
	glm::vec3* getPosition() { return Vertex::GetPosition(); }
	glm::vec3* getLastPosition() { return &m_lastPosition; }

	bool Movable() const { return m_movable; }
	void makeUnmovable() { m_movable = false; }
	void makeMovable() { m_movable = true; }
private:
	float m_mass;
	glm::vec3 m_acceleration;
	glm::vec3 m_lastPosition;
	glm::vec3 m_velocity;
	bool m_movable = true;
	
};

#endif