#include "Cloth2.h"
#include <iostream>
#include <deque>

Cloth2::Cloth2(float width, float height, unsigned int particles_width, unsigned int particles_height)
	: GridMesh(particles_height, particles_width), m_width(width), m_height(height), m_particles_width(particles_width), m_particles_height(particles_height)
{
	for (unsigned int y = 0; y < particles_height; y++)
	{
		for (unsigned int x = 0; x < particles_width; x++)
		{
			m_particles.push_back(std::make_shared<Particle>());
		}
	}

	setStartPosition();
	//create constraints
	for (unsigned int  y = 0; y < particles_height; y++)
	{
		for (unsigned int  x = 0; x < particles_width; x++)
		{
			if (x < (particles_width - 1)) //check this
			{
				//particle to the right
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 1, y), SpringType::STRUCT);
			}
				
			if (y < (m_particles_height - 1))
			{
				//particle below
				makeConstraint(getParticleAt(x, y), getParticleAt(x , y + 1), SpringType::STRUCT);
			}
			if (x < (particles_width -1) && y < (particles_height -1))
			{
				//making the cross
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 1 , y + 1), SpringType::SHEAR);
				makeConstraint(getParticleAt(x + 1, y), getParticleAt(x, y + 1), SpringType::SHEAR);
			}
		}
	}

	//bending particles
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			if (x < particles_width -2)
			{
				//to the right
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 2 , y), SpringType::BEND);
			}
			if (y < particles_height -2)
			{
				//below
				makeConstraint(getParticleAt(x , y), getParticleAt(x , y + 2), SpringType::BEND);
			}
			if (x < (particles_width -2) && y < (particles_height -2))
			{
				//the cross
				makeConstraint(getParticleAt(x, y), getParticleAt(x + 2, y + 2), SpringType::BEND);
				makeConstraint(getParticleAt(x + 2 , y),getParticleAt(x, y + 2), SpringType::BEND);
			}
		}
	}

	//making the top three on each side fixed
	getParticleAt(0, 0)->makeUnmovable();
	getParticleAt(1, 0)->makeUnmovable();

	getParticleAt(m_particles_width - 1, 0)->makeUnmovable();
	getParticleAt(m_particles_width - 2, 0)->makeUnmovable();
	
	ball_position = glm::vec3(2, -3.0f, -2);
	ball_size = 1.0f;

	GridMesh::UpdatePositions(ExtractPositions());
}


Cloth2::~Cloth2()
{
}

void Cloth2::setStartPosition()
{
	for (unsigned int y = 0; y < m_particles_height; y++)
	{
		for (unsigned int x = 0; x < m_particles_width; x++)
		{
			glm::vec3 temp = glm::vec3(
				m_width*((float)x / (float)m_particles_width),
				0.0f,
				-m_height*((float)y / (float)m_particles_height));
			getParticleAt(x, y)->setPosition(temp);
			getParticleAt(x, y)->setLastPosition(temp);
		}
	}
}

void Cloth2::makeConstraint(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, SpringType type)
{
	m_constraints.push_back(std::make_shared<Spring>(p1, p2, type));
}

std::shared_ptr<Particle> Cloth2::getParticleAt(unsigned int x, unsigned int y)
{
	return m_particles.at(x + m_particles_width*y);
}

std::vector<glm::vec3> Cloth2::ExtractPositions()
{
	std::vector<glm::vec3> temp;

	for (auto it = m_particles.cbegin(); it!=m_particles.cend(); it++)
	{
		temp.push_back((*it)->getPosition());
	}
	return temp;
}

std::vector<glm::vec3> Cloth2::ExtractNormals()
{
	std::vector<glm::vec3> temp;

	for (auto it = m_particles.cbegin(); it != m_particles.cend(); it++)
	{
		temp.push_back((*it)->getNormal());
	}
	return temp;
}

void Cloth2::TimeStep(float dt, unsigned int numIterations)
{
	ForceConstraints(numIterations);
	for (auto it = m_particles.cbegin(); it != m_particles.cend(); it++)
	{
		(*it)->timeStep(dt, 0.1f); //this could go wrong
	}
	
}

void Cloth2::ForceConstraints(unsigned int numIterations)
{
	for (unsigned int i = 0; i < numIterations; i++) //number of times we want to iterate over the constraints
	{
		for (auto it = m_constraints.cbegin(); it != m_constraints.cend(); it++)
		{
			(*it)->SatisfyConstraints();
		}
	}
}

void Cloth2::AddForce(glm::vec3 &force)
{
	for (auto it = m_particles.cbegin(); it < m_particles.cend(); it++)
	{
		(*it)->addForce(force);
	}
}

void Cloth2::Update(float dt, glm::vec3 wind, unsigned int numIterations)
{
	AddForce(glm::vec3(0, GRAVITY, 0));
	Wind(wind);
	TimeStep(dt, numIterations);

	BallCollision();
	//UpdateTextureCoordinates();
	CalculatePerVertexNormals();
	GridMesh::UpdatePositions(ExtractPositions());
	GridMesh::UpdateNormals(ExtractNormals());
}

void Cloth2::print()
{
	
}

glm::vec3 Cloth2::CalculateTriangleNormal(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, std::shared_ptr<Particle> p3)
{
	glm::vec3 pos1 = *p1->GetPosition();
	glm::vec3 pos2 = *p2->GetPosition();
	glm::vec3 pos3 = *p3->GetPosition();

	glm::vec3 v1 = pos2 - pos1;
	glm::vec3 v2 = pos3 - pos1;

	glm::vec3 ret = glm::cross(v1, v2);

	return ret;
}

void Cloth2::AddWind(std::shared_ptr<Particle> p1, std::shared_ptr<Particle> p2, std::shared_ptr<Particle> p3, glm::vec3 direction)
{
	glm::vec3 normal = CalculateTriangleNormal(p1, p2, p3);
	glm::vec3 d = glm::normalize(normal);
	glm::vec3 force = normal*glm::dot(d, direction);
	p1->addForce(force);
	p2->addForce(force);
	p3->addForce(force);
}

void Cloth2::Wind(glm::vec3 direction)
{
	for (unsigned int x = 0; x < m_particles_width -1; x++)
	{
		for (unsigned int y = 0; y < m_particles_height -1; y++)
		{
			AddWind(getParticleAt(x, y), getParticleAt(x + 1, y), getParticleAt(x, y + 1),direction);
			AddWind(getParticleAt(x + 1, y), getParticleAt(x, y + 1), getParticleAt(x + 1, y + 1), direction);
		}
	}
}

void Cloth2::BallCollision()
{
	for each (std::shared_ptr<Particle> particle in m_particles)
	{
		glm::vec3 forceVector = particle->getPosition() - ball_position;
		if (glm::length(forceVector) < ball_size)
		{
			forceVector = (forceVector / glm::length(forceVector))* ball_size;
			particle->setPosition(ball_position + forceVector);
		}
	}
}

void Cloth2::CalculatePerVertexNormals()
{
	//fixing  the top left corner
	{
		const unsigned int index = 0; // 0 + 0 * m_particles_width;
		unsigned int east = index + 1;
		unsigned int south = index + m_particles_width;
		const glm::vec3 center_pos = m_particles.at(index)->getPosition();
		glm::vec3 right = center_pos - m_particles.at(east)->getPosition();
		glm::vec3 down = center_pos - m_particles.at(south)->getPosition();
		glm::vec3 plane_normal = glm::normalize(glm::cross(right, down));
		m_particles.at(index)->SetNormal(plane_normal);
	}
	//fixing the top right corner
	{
		const unsigned int index = m_particles_width - 1; // (m_particles_width - 1) + 0*m_particles_width
		unsigned int west = index - 1;
		unsigned int south_west = (index - 1) + m_particles_width;
		unsigned int south = index + m_particles_width;

		const glm::vec3 center_pos = m_particles.at(index)->getPosition();
		glm::vec3 left = center_pos - m_particles.at(west)->getPosition();
		glm::vec3 down_left = center_pos - m_particles.at(south_west)->getPosition();
		glm::vec3 down = center_pos - m_particles.at(south)->getPosition();
		//the triangle normals
		glm::vec3 down_left_normal = glm::normalize(glm::cross(down_left, left));
		glm::vec3 down_middle_normal = glm::normalize(glm::cross(down, down_left));
		glm::vec3 sumNormal = (down_left_normal + down_middle_normal) / 2.0f;

		m_particles.at(index)->SetNormal(sumNormal);
	}
	//fixing the bottom left corner
	{
		const unsigned int index = (m_particles_height - 1)*m_particles_width; // 0 + (m_particles_height -1)*m_particles_width
		unsigned int north = index - m_particles_width;
		unsigned int north_east = (index + 1) - m_particles_width;
		unsigned int east = index + 1;

		const glm::vec3 center_pos = m_particles.at(index)->getPosition();
		//getting the vectors so we can calculate the normals
		glm::vec3 up = center_pos - m_particles.at(north)->getPosition();
		glm::vec3 up_right = center_pos - m_particles.at(north_east)->getPosition();
		glm::vec3 right = center_pos - m_particles.at(east)->getPosition();

		//calculating the normals
		glm::vec3 top_middle_normal = glm::normalize(glm::cross(up, up_right));
		glm::vec3 top_right_normal = glm::normalize(glm::cross(up_right, right));

		glm::vec3 sumNormal = (top_middle_normal + top_right_normal) / 2.0f;
		m_particles.at(index)->SetNormal(sumNormal);
	}
	//fixing the bottom right corner
	{
		const unsigned int index = m_particles_height*m_particles_width - 1; // (m_particles_width - 1) + (m_particles_height-1)*m_particles_width
		unsigned int west = index - 1;
		unsigned int north = index - m_particles_width;
		const glm::vec3 center_pos = m_particles.at(index)->getPosition();
		//getting the vectors so we can calculate the normals
		glm::vec3 left = center_pos - m_particles.at(west)->getPosition();
		glm::vec3 up = center_pos - m_particles.at(north)->getPosition();
		//calculating the normals
		glm::vec3 top_left_normal = glm::normalize(glm::cross(left, up));
		m_particles.at(index)->SetNormal(top_left_normal);
	}

	//fixing the top border
	for (unsigned int i = 1; i < m_particles_width - 1; i++)
	{
		const unsigned int index = i; //i + 0*m_particles_width
		unsigned int west = index - 1;
		unsigned int south_west = (index - 1) + m_particles_width;
		unsigned int south = index + m_particles_width;
		unsigned int east = index + 1;

		const glm::vec3 center_pos = m_particles.at(index)->getPosition();

		glm::vec3 left = center_pos - m_particles.at(west)->getPosition();
		glm::vec3 right = center_pos - m_particles.at(east)->getPosition();
		glm::vec3 down = center_pos - m_particles.at(south)->getPosition();
		glm::vec3 down_left = center_pos - m_particles.at(south_west)->getPosition();

		glm::vec3 down_right_normal = glm::normalize(glm::cross(right, down));
		glm::vec3 down_middle_normal = glm::normalize(glm::cross(down, down_left));
		glm::vec3 down_left_normal = glm::normalize(glm::cross(down_left, left));

		glm::vec3 sumNormal = (	down_right_normal + down_left_normal + down_middle_normal) / 3.0f;
		m_particles.at(index)->SetNormal(sumNormal);
	}

	//fixing the left border
	for (unsigned int j = 1; j < m_particles_height - 1; j++)
	{
		const unsigned int index = j*m_particles_width; //0 + j*m_particles_width
		unsigned int north = index - m_particles_width;
		unsigned int north_east = (index + 1) - m_particles_width;
		unsigned int east = index + 1;
		unsigned int south = index + m_particles_width;

		const glm::vec3 center_pos = m_particles.at(index)->getPosition();

		glm::vec3 up = center_pos - m_particles.at(north)->getPosition();
		glm::vec3 up_right = center_pos - m_particles.at(north_east)->getPosition();
		glm::vec3 right = center_pos - m_particles.at(east)->getPosition();
		glm::vec3 down = center_pos - m_particles.at(south)->getPosition();

		//calculating the normals
		glm::vec3 top_middle_normal = glm::normalize(glm::cross(up, up_right));
		glm::vec3 top_right_normal = glm::normalize(glm::cross(up_right, right));

		glm::vec3 down_right_normal = glm::normalize(glm::cross(right, down));

		//normalize the normals and add them together
		glm::vec3 sumNormal = (top_middle_normal + top_right_normal +
			down_right_normal) / 3.0f;
		m_particles.at(index)->SetNormal(sumNormal);
	}

	//right border
	for (unsigned int j = 1; j < m_particles_height - 1; j++)
	{
		const unsigned int index = (m_particles_width-1) + j*m_particles_width;
		unsigned int north = index - m_particles_width;
		unsigned int west = index - 1;
		unsigned int south_west = (index - 1) + m_particles_width;
		unsigned int south = index + m_particles_width;

		const glm::vec3 center_pos = m_particles.at(index)->getPosition();
		//getting the vectors so we can calculate the normals
		glm::vec3 left = center_pos - m_particles.at(west)->getPosition();
		glm::vec3 up = center_pos - m_particles.at(north)->getPosition();
		glm::vec3 down = center_pos - m_particles.at(south)->getPosition();
		glm::vec3 down_left = center_pos - m_particles.at(south_west)->getPosition();

		//calculating the normals
		glm::vec3 top_left_normal = glm::normalize(glm::cross(left, up));
		glm::vec3 down_middle_normal = glm::normalize(glm::cross(down, down_left));
		glm::vec3 down_left_normal = glm::normalize(glm::cross(down_left, left));
		//normalize the normals and add them together
		glm::vec3 sumNormal = (top_left_normal + down_left_normal + down_middle_normal) / 3.0f;
		m_particles.at(index)->SetNormal(sumNormal);
	}

	//bottom border
	for (unsigned int i = 1; i < m_particles_width - 1; i++)
	{
		const unsigned int index = i + (m_particles_height-1)*m_particles_width;
		unsigned int north = index - m_particles_width;
		unsigned int north_east = (index + 1) - m_particles_width;
		unsigned int west = index - 1;
		unsigned int east = index + 1;

		const glm::vec3 center_pos = m_particles.at(index)->getPosition();
		//getting the vectors so we can calculate the normals
		glm::vec3 left = center_pos - m_particles.at(west)->getPosition();
		glm::vec3 up = center_pos - m_particles.at(north)->getPosition();
		glm::vec3 up_right = center_pos - m_particles.at(north_east)->getPosition();
		glm::vec3 right = center_pos - m_particles.at(east)->getPosition();

		//calculating the normals
		glm::vec3 top_left_normal = glm::normalize(glm::cross(left, up));
		glm::vec3 top_middle_normal = glm::normalize(glm::cross(up, up_right));
		glm::vec3 top_right_normal = glm::normalize(glm::cross(up_right, right));

		//normalize the normals and add them together
		glm::vec3 sumNormal = (top_left_normal + top_middle_normal + top_right_normal) / 3.0f;
		m_particles.at(index)->SetNormal(sumNormal);
	}

	//start by ignoring the borders because borders are annoying
	for (unsigned int i = 1; i < m_particles_height - 1; i++)
	{
		for (unsigned int j = 1; j < m_particles_width - 1; j++)
		{
			const unsigned int index = j + i*m_particles_width;
			unsigned int north = index - m_particles_width;
			unsigned int north_east = (index + 1) - m_particles_width;
			unsigned int west = index - 1;
			unsigned int south_west = (index - 1) + m_particles_width;
			unsigned int south = index + m_particles_width;
			unsigned int east = index + 1;

			const glm::vec3 center_pos = m_particles.at(index)->getPosition();
			//getting the vectors so we can calculate the normals
			glm::vec3 left = center_pos - m_particles.at(west)->getPosition();
			glm::vec3 up = center_pos - m_particles.at(north)->getPosition();
			glm::vec3 up_right = center_pos - m_particles.at(north_east)->getPosition();
			glm::vec3 right = center_pos - m_particles.at(east)->getPosition();
			glm::vec3 down = center_pos - m_particles.at(south)->getPosition();
			glm::vec3 down_left = center_pos - m_particles.at(south_west)->getPosition();

			//calculating the normals
			glm::vec3 top_left_normal = glm::normalize(glm::cross(left, up));
			glm::vec3 top_middle_normal = glm::normalize(glm::cross(up, up_right));
			glm::vec3 top_right_normal = glm::normalize(glm::cross(up_right, right));

			glm::vec3 down_right_normal = glm::normalize(glm::cross(right, down));
			glm::vec3 down_middle_normal = glm::normalize(glm::cross(down, down_left));
			glm::vec3 down_left_normal = glm::normalize(glm::cross(down_left, left));

			//normalize the normals and add them together
			glm::vec3 sumNormal = (top_left_normal + top_middle_normal + top_right_normal +
				down_right_normal + down_left_normal + down_middle_normal) / 6.0f;
			m_particles.at(index)->SetNormal(sumNormal);
		}
	}
	
	
	std::deque<glm::vec3> temp;
	//Trying something here
	//what if we also take into concideration nearby particle normals
	for (unsigned int i = 1; i < m_particles_height - 1; i++)
	{
		for (unsigned int j = 1; j < m_particles_width - 1; j++)
		{
			const unsigned int index = j + i*m_particles_width;
			unsigned int north = index - m_particles_width;
			unsigned int north_east = (index + 1) - m_particles_width;
			unsigned int north_west = (index - 1) - m_particles_width;
			unsigned int west = index - 1;
			unsigned int south_west = (index - 1) + m_particles_width;
			unsigned int south_east = (index + 1) + m_particles_width;
			unsigned int south = index + m_particles_width;
			unsigned int east = index + 1;

			//calculating the normals
			glm::vec3 current_normal = m_particles.at(index)->getNormal();
			glm::vec3 top_normal = m_particles.at(north)->getNormal();
			glm::vec3 left_normal = m_particles.at(west)->getNormal();
			glm::vec3 down_normal = m_particles.at(south)->getNormal();
			glm::vec3 right_normal = m_particles.at(east)->getNormal();
			glm::vec3 top_left_normal = m_particles.at(north_west)->getNormal();
			glm::vec3 top_right_normal = m_particles.at(north_east)->getNormal();
			glm::vec3 down_left_normal = m_particles.at(south_west)->getNormal();
			glm::vec3 down_right_normal = m_particles.at(south_east)->getNormal();

			glm::vec3 average = (current_normal + top_normal + left_normal + down_normal + right_normal +
				top_left_normal + top_right_normal + down_left_normal + down_right_normal) / 9.0f;
			glm::normalize(average);
			temp.push_back(average);
		}
	}
	for (unsigned int i = 1; i < m_particles_height - 1; i++)
	{
		for (unsigned int j = 1; j < m_particles_width - 1; j++)
		{
			const unsigned int index = j + i*m_particles_width;

			m_particles.at(index)->SetNormal(temp.front());
			temp.pop_front();
		}
	}
	
}

void Cloth2::UpdateTextureCoordinates()
{
	//using a central difference approach we can find the acceleration of a particle and use the previous position and move the texture coordinate in that
	//direction
	//ignoring borders cause they are annoying
	for (unsigned int i = 1; i < m_particles_height -2; i++)
	{
		for (unsigned int j = 1; j < m_particles_width - 2; j++)
		{
			const unsigned int index = j + i*m_particles_width;
			const unsigned int west = index - 1;
			const unsigned int east = index + 1;
			const unsigned int north = index - m_particles_width;
			const unsigned int south = index + m_particles_width;
			glm::vec2 currentTexCoord = *m_particles.at(index)->GetTexCoord();
			glm::vec2 accX = *m_particles.at(east)->GetTexCoord() - *m_particles.at(west)->GetTexCoord();
			accX = accX / 2.0f;

			glm::vec2 accY = *m_particles.at(north)->GetTexCoord() - *m_particles.at(south)->GetTexCoord();
			accY = accY / 2.0f;
			
			glm::vec2 new_TexCoord = (accX + accY) / 2.0f;
			m_particles.at(index)->SetTexCoord(currentTexCoord - new_TexCoord);
		}
	}
}

void Cloth2::Upload()
{
	GridMesh::UploadToGPU();
}