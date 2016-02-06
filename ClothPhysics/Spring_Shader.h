#pragma once
#ifndef SPRING_SHADER_H
#define SPRING_SHADER_H
#include "Basic_Shader.h"
class Spring_Shader :
	public Basic_Shader
{
public:
	Spring_Shader(const std::string& filePath = "./shaders/massSpring");
	~Spring_Shader();

	void UpdateValues(const Transform & transform, const Camera & camera) override;
};

#endif //SPRING_SHADER_H