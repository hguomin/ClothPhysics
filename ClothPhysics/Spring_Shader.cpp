#include "Spring_Shader.h"



Spring_Shader::Spring_Shader(const std::string& filePath)
{
	Shader::LoadFromFile(GL_VERTEX_SHADER, filePath + ".vert");
}


Spring_Shader::~Spring_Shader()
{
}

void Spring_Shader::UpdateValues(const Transform & transform, const Camera & camera)
{
	glUniformMatrix4fv(UnifLoc("projection"), 1, GL_FALSE, &camera.GetProjectionMatrix()[0][0]);
	glUniformMatrix4fv(UnifLoc("view"), 1, GL_FALSE, &camera.GetViewMatrix()[0][0]);
	glUniformMatrix4fv(UnifLoc("model"), 1, GL_FALSE, &transform.GetMatrix()[0][0]);
}
