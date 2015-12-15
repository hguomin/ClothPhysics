#pragma once
#include "Shader.h"
class Skybox_Shader :
	public Shader
{
public:
	Skybox_Shader(const std::string& fileName) : Shader(fileName)
	{
		LoadUniforms();
	};
	~Skybox_Shader();
	
	void Update(const Transform& transform, const Camera& camera) override;
protected:
	void LoadUniforms() override;
private:
	
	enum Uniform
	{
		MODEL_U,
		VIEW_U,
		PROJECTION_U,
		CUBEMAP_U,

		NUM_UNIFORMS
	};
	GLuint m_uniforms[NUM_UNIFORMS];
};

