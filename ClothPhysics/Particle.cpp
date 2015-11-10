#include "Particle.h"

void Particle::timeStep(float dt, float dampening)
{
	if (m_movable)
	{
		glm::vec3 temp = m_position;
		glm::vec3 velocity = m_position - m_lastPosition;
		m_position += velocity*(1 - dampening) + m_acceleration*dt*dt;
		m_lastPosition = temp;
		setAcceleration(glm::vec3(0)); //already performed the acceleration now so reset it
	}
}