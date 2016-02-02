#pragma once
#include "Shader.h"

class Phong_Shader :
	public Shader
{
public:
	Phong_Shader(const std::string& filePath = "./shaders/phong");
	~Phong_Shader();

	void UpdateValues(const Transform& transform, const Camera& camera);
};

