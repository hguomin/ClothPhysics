#pragma once
#include <map>
#include <string>
#include <vector>
#include "GL\glew.h"

class Shader
{
public:
	Shader();
	virtual ~Shader() = 0;

	void LoadFromString(GLenum type, const std::string& source);
	
	void CreateAndLinkProgram();
	void Use();
	void UnUse();
	void AddAttribute(const std::string& attribute);
	void AddUniform(const std::string& uniform);
	GLuint getProgram() const;

	GLuint operator[](const std::string& attribute);
	GLuint operator()(const std::string& uniform);
	void DeleteShaderProgram();

	void LoadFromFile(GLenum whichShader, const std::string& fileName);
	static GLuint LoadFromFileAndReturn(GLenum type, const std::string& fileName);
	static void PrintShaderError(GLuint shader);
	static void PrintProgramError(GLuint program);
	static GLuint LoadFromStringAndReturn(GLenum type, const std::string& source);
protected:
	
	GLuint m_program;
	int m_totalShaders;
	std::vector<GLuint> m_shaders;
	std::map<std::string, GLuint> m_attributeList;
	std::map<std::string, GLuint> m_uniformLocationList;
	std::map<GLenum, unsigned int> ShaderType;

	GLuint UnifLoc(const std::string& uniform);
	GLuint AttrList(const std::string& attrib);
private:

	
};

