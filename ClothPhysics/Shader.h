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
	virtual void Update(const Transform& transform, const Camera& camera) = 0;
protected:
	virtual void LoadUniforms() = 0; //pure virtual
	static const unsigned int NUM_SHADERS = 2;
	GLuint m_program;
	GLuint m_shaders[NUM_SHADERS];
	
private:
	
	Shader(const Shader& other) {}
};
#endif //SHADER_H
