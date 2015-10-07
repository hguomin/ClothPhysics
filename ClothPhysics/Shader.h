#pragma once
#ifndef SHADER_H
#define SHADER_H
#include <Windows.h>
#include <string>
#include <GL\glew.h>
#include <GL\freeglut.h>
#include "Transform.h"
#include "Camera.h"

class Shader
{
public:
	Shader(const std::string& fileName);
	~Shader();

	void Use();
	void Update(const Transform& transform, const Camera& camera);

private:
	static const unsigned int NUM_SHADERS = 2;
	Shader(const Shader& other) {}

	enum
	{
		MODEL_U,
		VIEW_U,
		PROJECTION_U,
		CUBEMAP_U,

		NUM_UNIFORMS
	};

	GLuint m_program;
	GLuint m_shaders[NUM_SHADERS];
	GLuint m_uniforms[NUM_UNIFORMS];
};
#endif //SHADER_H
