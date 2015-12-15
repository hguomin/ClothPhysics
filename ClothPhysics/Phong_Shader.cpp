#include "Phong_Shader.h"
#include "glm\gtc\type_ptr.hpp"

Phong_Shader::~Phong_Shader()
{
}

void Phong_Shader::Update(const Transform & transform, const Camera & camera)
{
	glUniformMatrix4fv(m_uniforms[PROJECTION_U], 1, GL_FALSE, &camera.GetProjectionMatrix()[0][0]);
	glUniformMatrix4fv(m_uniforms[VIEW_U], 1, GL_FALSE, &camera.GetViewMatrix()[0][0]);
	glUniformMatrix4fv(m_uniforms[MODEL_U], 1, GL_FALSE, &transform.GetMatrix()[0][0]);

	glm::vec3 light_pos = glm::vec3(1, 0, -15);
	glUniform3fv(m_uniforms[LIGHTPOS_U], 1, glm::value_ptr(light_pos));

	glm::vec3 ambintensity = glm::vec3(255/255, 105/255, 180/255);
	glm::vec3 diffuseintensity = glm::vec3(1, 1, 1);
	glm::vec3 specularintensity = glm::vec3(0, 1, 0);
	glUniform3fv(m_uniforms[LIGHT_AMBIENTINTENSITY], 1, glm::value_ptr(ambintensity));
	glUniform3fv(m_uniforms[LIGHT_DIFFUSE_INTENSITY], 1, glm::value_ptr(diffuseintensity));
	glUniform3fv(m_uniforms[LIGHT_SPECULAR_INTENSITY], 1, glm::value_ptr(specularintensity));

	glm::vec3 ambreflect = glm::vec3(1, 1, 1);
	glUniform3fv(m_uniforms[MATERIAL_AMBIENT_REFLECT], 1, glm::value_ptr(ambreflect));
	glm::vec3 diffusereflect = glm::vec3(1, 1, 1);
	glUniform3fv(m_uniforms[MATERIAL_DIFFUSE_REFLECT], 1, glm::value_ptr(diffusereflect));
	glm::vec3 specreflect = glm::vec3(1, 1, 1);
	glUniform3fv(m_uniforms[MATERIAL_SPECULAR_REFLECT], 1, glm::value_ptr(specreflect));

	float shininess = 64;
	glUniform1f(m_uniforms[MATERIAL_SHININESS], shininess);


	glUniform3fv(m_uniforms[CAMERAPOS_U], 1, glm::value_ptr(camera.GetPosition()));
}

void Phong_Shader::LoadUniforms()
{
	m_uniforms[MODEL_U] = glGetUniformLocation(m_program, "model");
	m_uniforms[VIEW_U] = glGetUniformLocation(m_program, "view");
	m_uniforms[PROJECTION_U] = glGetUniformLocation(m_program, "projection");

	m_uniforms[LIGHTPOS_U] = glGetUniformLocation(m_program, "u_lightPosition");

	m_uniforms[LIGHT_AMBIENTINTENSITY] = glGetUniformLocation(m_program, "u_lightAmbientIntensitys");
	m_uniforms[LIGHT_DIFFUSE_INTENSITY] = glGetUniformLocation(m_program, "u_lightDiffuseIntensitys");
	m_uniforms[LIGHT_SPECULAR_INTENSITY] = glGetUniformLocation(m_program, "u_lightSpecularIntensitys");

	m_uniforms[MATERIAL_AMBIENT_REFLECT] = glGetUniformLocation(m_program, "u_matAmbientReflectances");
	m_uniforms[MATERIAL_DIFFUSE_REFLECT] = glGetUniformLocation(m_program, "u_matDiffuseReflectances");
	m_uniforms[MATERIAL_SPECULAR_REFLECT] = glGetUniformLocation(m_program, "u_matSpecularReflectances");
	m_uniforms[MATERIAL_SHININESS] = glGetUniformLocation(m_program, "u_matShininess");

	m_uniforms[CAMERAPOS_U] = glGetUniformLocation(m_program, "u_cameraPosition");
}
