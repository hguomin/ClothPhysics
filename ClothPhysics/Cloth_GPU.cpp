#include "Cloth_GPU.h"
#include "Shader.h"

Cloth_GPU::Cloth_GPU()
{
	m_points_height = 75;
	m_points_width = 75;
	m_width = 8;
	m_height = 8;
	m_points_total = (m_points_height*m_points_width);
	m_connections_total = ((m_points_width - 1)* m_points_height + (m_points_height - 1)*m_points_width);
	iterations_per_frame = 4;

	loadShaders();

	glm::vec4* initial_positions = new glm::vec4[m_points_total];
	glm::vec3* initial_velocities = new glm::vec3[m_points_total];
	std::vector<glm::ivec4> stretch_connection_vectors(m_points_total);
	std::vector<glm::ivec4> shear_connection_vectors(m_points_total);
	std::vector<glm::ivec4> bend_connection_vectors(m_points_total);

	bool stretch = false;
	float stretch_length = -1;
	bool shear = false;
	float shear_length = -1;
	bool bend = false;
	float bend_length = -1;

	unsigned int n = 0;
	for (unsigned int j = 0; j < m_points_height; j++)
	{
		float fj = float(j) / float(m_points_height);
		for (unsigned int i = 0; i < m_points_width; i++)
		{
			float fi = float(i) / float(m_points_width);
			glm::vec4 temp = glm::vec4(
				(fi - 0.5f)*m_width,
				0,
				-(fj - 0.5f)*m_height,
				1.0f);
			initial_positions[n] = temp;
			initial_velocities[n] = glm::vec3(0.0f);
			stretch_connection_vectors[n] = glm::ivec4(-1);
			shear_connection_vectors[n] = glm::ivec4(-1);
			bend_connection_vectors[n] = glm::ivec4(-1);

			if (j != (m_points_height - 1))
			{
				if (i != 0) //index left
				{
					stretch_connection_vectors[n][0] = n - 1;
					stretch = true;
				}
				if (j != 0) //index above
				{
					stretch_connection_vectors[n][1] = n - m_points_width;
					stretch = true;
				}
				if (i != (m_points_width -1)) //index right
				{
					stretch_connection_vectors[n][2] = n + 1;
					stretch = true;
				}
				if (j != (m_points_height -1)) //index left
				{
					stretch_connection_vectors[n][3] = n + m_points_width;
					stretch = true;
				}
			}
			//shear. A bit redundent but easier to read
				if (j != 0 && i != 0) //index above left
				{
					shear_connection_vectors[n][0] = n - 1 - m_points_width;
					shear = true;
				}
				if (j != 0 && i != (m_points_width - 1)) //index above right
				{
					shear_connection_vectors[n][1] = n + 1 - m_points_width;
					shear = true;
				}
				if (j !=(m_points_height -1) && i != 0) //index below left
				{
					shear_connection_vectors[n][2] = n - 1 + m_points_width;
					shear = true;
				}
				if (j != (m_points_height -1) && i !=(m_points_width -1)) //index below right
				{
					shear_connection_vectors[n][3] = n + 1 + m_points_width;
					shear = true;
				}
			//bend
				if (i > 1) //index left
				{
					bend_connection_vectors[n][0] = n - 2;
					bend = true;
				}
				if (i < (m_points_width - 2)) //index right
				{
					bend_connection_vectors[n][1] = n + 2;
					bend = true;
				}
				if (j > 1) //index above
				{
					bend_connection_vectors[n][2] = n - 2 * m_points_width;
					bend = true;
				}
				if (j < (m_points_width -2)) //index below
				{
					bend_connection_vectors[n][3] = n + 2 * m_points_width;
					bend = true;
				}
			n++;
		}
	}

	//calculating resting length
	if (stretch)
	{
		stretch_length = glm::length(glm::vec3(initial_positions[0]) - glm::vec3(initial_positions[1]));
	}
	if (shear)
	{
		shear_length = glm::length(glm::vec3(initial_positions[0]) - glm::vec3(initial_positions[m_points_width + 1]));
	}
	if (bend)
	{
		bend_length = glm::length(glm::vec3(initial_positions[0]) - glm::vec3(initial_positions[2 * m_points_width]));
	}
	//add the rest lengths to the update shader as uniforms
	glUniform1f(glGetUniformLocation(m_update_program, "stretch_length"), stretch_length);
	glUniform1f(glGetUniformLocation(m_update_program, "shear_length"), shear_length);
	glUniform1f(glGetUniformLocation(m_update_program, "bend_length"), bend_length);

	glGenVertexArrays(2, m_vArrayO);
	glGenBuffers(7, m_vBufferO);

	for (unsigned int i = 0; i < 2; i++)
	{
		glBindVertexArray(m_vArrayO[i]);

		glBindBuffer(GL_ARRAY_BUFFER, m_vBufferO[POSITION_A + i]);
		glBufferData(GL_ARRAY_BUFFER, m_points_total*sizeof(glm::vec4), initial_positions, GL_DYNAMIC_COPY);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, m_vBufferO[VELOCITY_A + i]);
		glBufferData(GL_ARRAY_BUFFER, m_points_total*sizeof(glm::vec3), initial_velocities, GL_DYNAMIC_COPY);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, m_vBufferO[STRETCH_CONNECTION]);
		glBufferData(GL_ARRAY_BUFFER, m_points_total*sizeof(glm::ivec4), &stretch_connection_vectors[0], GL_STATIC_DRAW);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, m_vBufferO[SHEAR_CONNECTION]);
		glBufferData(GL_ARRAY_BUFFER, m_points_total*sizeof(glm::ivec4), &shear_connection_vectors[0], GL_STATIC_DRAW);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(3);

		glBindBuffer(GL_ARRAY_BUFFER, m_vBufferO[BEND_CONNECTION]);
		glBufferData(GL_ARRAY_BUFFER, m_points_total*sizeof(glm::ivec4), &bend_connection_vectors[0], GL_STATIC_DRAW);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(4);
	}

	delete [] initial_positions;
	delete [] initial_velocities;
	stretch_connection_vectors.clear();
	bend_connection_vectors.clear();
	shear_connection_vectors.clear();

	glGenTextures(2, m_pos_texBufferO);
	glBindTexture(GL_TEXTURE_BUFFER, m_pos_texBufferO[0]);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vBufferO[POSITION_A]);
	glBindTexture(GL_TEXTURE_BUFFER, m_pos_texBufferO[1]);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_vBufferO[POSITION_B]);

	int lines = (m_points_width - 1) * m_points_height + (m_points_height - 1)* m_points_width;

	glGenBuffers(1, &m_index_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lines * 2 * sizeof(int), NULL, GL_STATIC_DRAW);

	int* e = (int *)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, lines * 2 * sizeof(int), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

	for (unsigned int  j = 0; j < m_points_height; j++)
	{
		for (unsigned int i = 0; i < m_points_width - 1; i++)
		{
			*e++ = i + j*m_points_width;
			*e++ = 1 + i + j*m_points_width;
		}
	}

	for (unsigned int i = 0; i < m_points_width; i++)
	{
		for (unsigned int j = 0; j < m_points_height - 1; j++)
		{
			*e++ = i + j*m_points_width;
			*e++ = m_points_width + i + j*m_points_width;
		}
	}

	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

Cloth_GPU::~Cloth_GPU()
{
	glDeleteProgram(m_update_program);
	glDeleteBuffers(7, m_vBufferO);
	glDeleteVertexArrays(2, m_vArrayO);
	m_renderShader.DeleteShaderProgram();
}

void Cloth_GPU::Draw(const Transform & transform, const Camera & camera)
{
	glUseProgram(m_update_program);

	glEnable(GL_RASTERIZER_DISCARD);

	for (unsigned int i = iterations_per_frame; i != 0; --i)
	{
		unsigned int temp = m_iteration_index & 1;
		glBindVertexArray(m_vArrayO[m_iteration_index & 1]);
		glBindTexture(GL_TEXTURE_BUFFER, m_pos_texBufferO[m_iteration_index & 1]);
		m_iteration_index++;
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_vBufferO[POSITION_A + (m_iteration_index & 1)]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_vBufferO[VELOCITY_A + (m_iteration_index & 1)]);
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, m_points_total);
		glEndTransformFeedback();
	}

	glDisable(GL_RASTERIZER_DISCARD);

	m_renderShader.Use();
	m_renderShader.UpdateValues(transform, camera);

	glPointSize(4.0f);
	glDrawArrays(GL_POINTS, 0, m_points_total);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
	glDrawElements(GL_LINES, m_connections_total * 2, GL_UNSIGNED_INT, NULL);
	m_renderShader.UnUse();
}

void Cloth_GPU::loadShaders()
{
	GLuint vs;

	vs = Shader::LoadFromFileAndReturn(GL_VERTEX_SHADER, "./shaders/update.vert");
	m_update_program = glCreateProgram();
	glAttachShader(m_update_program, vs);

	static const char* tf_varyings[] =
	{
		"tf_position_mass",
		"tf_velocity"
	};
	glTransformFeedbackVaryings(m_update_program, 2, tf_varyings, GL_SEPARATE_ATTRIBS);

	glLinkProgram(m_update_program);

	Shader::PrintError(m_update_program);

	glDeleteShader(vs);

	m_renderShader = Basic_Shader("./shaders/render");
}
