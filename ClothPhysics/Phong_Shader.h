#pragma once
#include "Shader.h"
class Phong_Shader :
	public Shader
{
public:
	Phong_Shader(const std::string& fileName) : Shader(fileName)
	{
		LoadUniforms();
	}
	~Phong_Shader();

	void Update(const Transform& transform, const Camera& camera) override;

protected:
	void LoadUniforms() override;
	enum Uniform
	{
		MODEL_U,
		VIEW_U,
		PROJECTION_U,
		LIGHTPOS_U,

		LIGHT_AMBIENTINTENSITY,
		LIGHT_DIFFUSE_INTENSITY,
		LIGHT_SPECULAR_INTENSITY,

		MATERIAL_AMBIENT_REFLECT,
		MATERIAL_DIFFUSE_REFLECT,
		MATERIAL_SPECULAR_REFLECT,
		MATERIAL_SHININESS,

		CAMERAPOS_U,

		NUM_UNIFORMS
	};

	GLuint m_uniforms[NUM_UNIFORMS];
};

