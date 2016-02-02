#include "Cloth_GPU.h"



Cloth_GPU::Cloth_GPU()
{
}

Cloth_GPU::~Cloth_GPU()
{
	m_indices.clear();
	m_forces.clear();
	m_last_position.clear();
	m_position.clear();

	glDeleteVertexArrays(1, &m_vArrayoCloth);
	glDeleteVertexArrays(2, m_vArrayoRender);
	glDeleteVertexArrays(2, m_vArrayoUpdate);

	glDeleteBuffers(1, &m_vBufferoCloth);
	glDeleteBuffers(1, &m_vBufferoIndices);

	glDeleteBuffers(2, m_vBuffero_Pos);
	glDeleteBuffers(2, m_vBuffero_PrevPos);

	glDeleteTransformFeedbacks(1, &m_transformFeedback);

	glDeleteTextures(2, m_texPos);
	glDeleteTextures(2, m_texPrevPos);
}

void Cloth_GPU::InitShaders(float timeStep, glm::vec3& gravity, unsigned int texture_size_x, unsigned int texture_size_y,
	unsigned int sizeX, unsigned int sizeY, unsigned int particle_width, unsigned int particle_height,
	float KsStruct, float KsShear, float KsBend, float KdStruct, float KdShear, float KdBend, float DEFAULT_DAMPING)
{
	m_massSpringShader.Use();
		glUniform1f(m_massSpringShader("dt"), timeStep);
		glUniform3fv(m_massSpringShader("gravity"), 1, &gravity.x);
		glUniform1i(m_massSpringShader("tex_position_mass"), 0);
		glUniform1i(m_massSpringShader("tex_pre_position_mass"), 1);
		glUniform1i(m_massSpringShader("texsize_x"), texture_size_x);
		glUniform1i(m_massSpringShader("texsize_y"), texture_size_y);
		glUniform2f(m_massSpringShader("inv_cloth_size"), float(sizeX) / particle_width, float(sizeY) / particle_height);
		glUniform2f(m_massSpringShader("step"), 1.0f / (texture_size_x - 1.0f), 1.0f / (texture_size_y - 1.0f));
		glUniform1f(m_massSpringShader("ksStr"), KsStruct);
		glUniform1f(m_massSpringShader("ksShr"), KsShear);
		glUniform1f(m_massSpringShader("ksBnd"), KsBend);
		glUniform1f(m_massSpringShader("kdStr"), KdStruct / 1000.0f);
		glUniform1f(m_massSpringShader("kdShr"), KdShear / 1000.0f);
		glUniform1f(m_massSpringShader("kdBnd"), KdBend / 1000.0f);
		glUniform1f(m_massSpringShader("DEFAULT_DAMPING"), DEFAULT_DAMPING);
	m_massSpringShader.UnUse();
}

Cloth_GPU::Cloth_GPU(unsigned int width, unsigned int height, unsigned int particle_width, unsigned int particle_height) :
	m_width(width), m_height(height),  m_particles_width(particle_width), m_particles_height(particle_height)
{
	const unsigned int total_points = (particle_height + 1)*(particle_width + 1); //+1 cause of 0 pos
	m_total_particles = total_points;
	m_indices.resize(particle_height*particle_width * 2 * 3); //why 2 * 3?
	m_position.resize(total_points);
	m_last_position.resize(total_points);
	m_forces.resize(total_points);
	//generating positions in space
	setStartPosition();
	//generate indices
	setIndices();
	//create time querry
	glGenQueries(1, &t_query);
	
}

void Cloth_GPU::setStartPosition()
{
	for (unsigned int y = 0; y < m_particles_height; y++)
	{
		for (unsigned int x = 0; x < m_particles_width; x++)
		{
			const unsigned int index = y*m_particles_width + x;
			glm::vec4 temp = glm::vec4(
				m_width*((float)x / (float)m_particles_width),
				0.0f,
				-m_height*((float)y / (float)m_particles_height),
				1);
			m_position[index] = temp;
			m_last_position[index] = temp;
		}
	}
}

void Cloth_GPU::setIndices()
{
	//fill in indices
	GLushort* id = &m_indices[0];
	for (int i = 0; i < m_particles_height; i++) {
		for (int j = 0; j < m_particles_width; j++) {
			int i0 = i * (m_particles_width + 1) + j;
			int i1 = i0 + 1;
			int i2 = i0 + (m_particles_width + 1);
			int i3 = i2 + 1;
			if ((j + i) % 2) {
				*id++ = i0; *id++ = i2; *id++ = i1;
				*id++ = i1; *id++ = i2; *id++ = i3;
			}
			else {
				*id++ = i0; *id++ = i2; *id++ = i3;
				*id++ = i0; *id++ = i3; *id++ = i1;
			}
		}
	}
}

void Cloth_GPU::generateArraysAndBuffers()
{
	glGenVertexArrays(1, &m_vArrayoCloth);
	glGenBuffers(1, &m_vBufferoCloth);
	glGenBuffers(1, &m_vBufferoIndices);

	glBindVertexArray(m_vArrayoCloth);
	glBindBuffer(GL_ARRAY_BUFFER, m_vBufferoCloth);
	//pass data to the gpu
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * m_position.size(), &m_position[0].x, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	assert(glGetError() == GL_NO_ERROR); //check for errors

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vBufferoIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*m_indices.size(), &m_indices[0], GL_STATIC_DRAW);
	glBindVertexArray(0);

	//now generating the ping-pong and render buffers
	glGenVertexArrays(2, m_vArrayoUpdate);
	glGenVertexArrays(2, m_vArrayoRender);
	glGenBuffers(2, m_vBuffero_Pos);
	glGenBuffers(2, m_vBuffero_PrevPos);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindVertexArray(m_vArrayoUpdate[i]);
		glBindBuffer(GL_ARRAY_BUFFER, m_vBuffero_Pos[i]);
		glBufferData(GL_ARRAY_BUFFER, m_position.size() * sizeof(glm::vec4), &(m_position[0].x), GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, m_vBuffero_PrevPos[i]);
		glBufferData(GL_ARRAY_BUFFER, m_last_position.size() * sizeof(glm::vec4), &(m_last_position[0].x), GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	}
	// Render vao
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindVertexArray(m_vArrayoRender[i]);
		glBindBuffer(GL_ARRAY_BUFFER, m_vBuffero_Pos[i]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vBufferoIndices);
		if (i == 0)
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size()*sizeof(GLushort), &m_indices[0], GL_STATIC_DRAW);
		}
	}

	//setting up so we map to textures for ease of use
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_BUFFER, m_texPos[i]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vBuffero_Pos[i]);
		glBindTexture(GL_TEXTURE_BUFFER, m_texPrevPos[i]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vBuffero_PrevPos[i]);
	}

	//setting up transform feedback
	glGenTransformFeedbacks(1, &m_transformFeedback);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transformFeedback);
	const char* varying_names[] = { "out_position_mass", "out_prev_position" };
	glTransformFeedbackVaryings(m_massSpringShader.getProgram(), 2, varying_names, GL_SEPARATE_ATTRIBS);
	glLinkProgram(m_massSpringShader.getProgram());
}

void Cloth_GPU::Draw(const Transform & transform, const Camera & camera)
{
	m_massSpringShader.Use();
	m_massSpringShader.UpdateValues(transform, camera);
	for (unsigned int i = 0; i < NUM_ITER; i++)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, m_texPos[m_write]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, m_texPrevPos[m_write]);
		glBindVertexArray(m_vArrayoUpdate[m_write]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_vBuffero_Pos[m_read]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_vBuffero_PrevPos[m_read]);
		glEnable(GL_RASTERIZER_DISCARD); //no rasterization
		//get the times
		glBeginQuery(GL_TIME_ELAPSED, t_query);
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, m_total_particles);
		glEndTransformFeedback();
		glEndQuery(GL_TIME_ELAPSED);
		glFlush();
		glDisable(GL_RASTERIZER_DISCARD); //enable rasterization again
		std::swap(m_read, m_write);
	}
	GLuint64 elapsed_time = 0.0f;
	glGetQueryObjectui64v(t_query, GL_QUERY_RESULT, &elapsed_time);
	float delta_time = float(elapsed_time) / 1000000.0f;
	m_massSpringShader.UnUse();

	//now time to Render
	glBindVertexArray(m_vArrayoRender[m_write]);
	glDisable(GL_DEPTH_TEST); //why am I doing this?!
	m_renderShader.Use();
	m_renderShader.UpdateValues(transform, camera);
	//draw the geometry
	glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_SHORT, 0);
	m_renderShader.UnUse();
	glEnable(GL_DEPTH_TEST); //why am I doing this?!
	
	glBindVertexArray(0);
}
