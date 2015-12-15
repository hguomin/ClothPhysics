#pragma once
#include "Shader.h"
class Basic_Shader :
	public Shader
{
public:
	Basic_Shader(const std::string& fileName) : Shader(fileName)
	{
		LoadUniforms();
	};
	~Basic_Shader();
	void Update(const Transform& transform, const Camera& camera) override;
protected:
	enum Uniform
	{
		MODEL_U,
		VIEW_U,
		PROJECTION_U,

		NUM_UNIFORMS
	};
	GLuint m_uniforms[NUM_UNIFORMS];
	void LoadUniforms() override;
};

