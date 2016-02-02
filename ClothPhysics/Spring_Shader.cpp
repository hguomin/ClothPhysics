#include "Spring_Shader.h"



Spring_Shader::Spring_Shader(const std::string& filePath) : Basic_Shader(filePath)
{
	Basic_Shader::Use();
		AddAttribute("position_mass");
		AddAttribute("prev_pos");
		AddUniform("tex_pos_mass");
		AddUniform("tex_prev_pos_mass");
		AddUniform("dt");
		AddUniform("gravity");
		AddUniform("ksStr");
		AddUniform("ksShr"); //shear springs
		AddUniform("ksBnd"); //bend strings
		AddUniform("kdStr");
		AddUniform("kdShr");
		AddUniform("kdBnd");
		AddUniform("DEFAULT_DAMPING");
		AddUniform("texsize_x");
		AddUniform("texsize_y");
		AddUniform("step");
		AddUniform("inv_cloth_size");
	Basic_Shader::UnUse();

}


Spring_Shader::~Spring_Shader()
{
}

void Spring_Shader::UpdateValues(const Transform & transform, const Camera & camera)
{
	Basic_Shader::UpdateValues(transform, camera);
}
