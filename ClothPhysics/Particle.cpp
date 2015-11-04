#include "Particle.h"

void Particle::timeStep(float dt, float dampening)
{
	if (m_movable)
	{
		glm::vec3 temp = m_position;
		m_position += (m_position - m_lastPosition)*(1 - dampening) + m_acceleration*dt*dt;
		m_lastPosition = temp;
		m_acceleration = glm::vec3(0); //already performed the acceleration now so reset it
	}
}