#include "Basic_Shader.h"

Basic_Shader::~Basic_Shader()
{
}

void Basic_Shader::Update(const Transform & transform, const Camera & camera)
{
	glUniformMatrix4fv(m_uniforms[PROJECTION_U], 1, GL_FALSE, &camera.GetProjectionMatrix()[0][0]);
	glUniformMatrix4fv(m_uniforms[VIEW_U], 1, GL_FALSE, &camera.GetViewMatrix()[0][0]);
	glUniformMatrix4fv(m_uniforms[MODEL_U], 1, GL_FALSE, &transform.GetMatrix()[0][0]);
}

void Basic_Shader::LoadUniforms()
{
	m_uniforms[MODEL_U] = glGetUniformLocation(m_program, "model");
	m_uniforms[VIEW_U] = glGetUniformLocation(m_program, "view");
	m_uniforms[PROJECTION_U] = glGetUniformLocation(m_program, "projection");
}
