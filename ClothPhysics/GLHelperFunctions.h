#pragma once
#include <vector>
#include <GL\glew.h>
namespace HELPER
{
	/* example on how to extract glm::vec4:
	std::vector<glm::vec4> temp = DEBUG::GetBufferData<glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X.size());
	*/
	template<typename T>
	std::vector<T> GetBufferData(GLenum bufferType, unsigned int bufferSize)
	{
		std::vector<T> ret;
		ret.reserve(bufferSize);
		T* pBufData = (T*)glMapBuffer(bufferType, GL_READ_ONLY);
		for (size_t i = 0; i < bufferSize; i++)
		{
			ret.push_back(*pBufData++);
		}
		glUnmapBuffer(bufferType);
		return ret;
	}

	template<typename T>
	void PushBufferData(GLenum bufferType, GLuint vbo, std::vector<T> dataToPush, unsigned int bufferSize)
	{
		glBindBuffer(bufferType, vbo);
		T* pData = (T *)glMapBufferRange(bufferType, 0, dataToPush.size(), GL_MAP_WRITE_BIT);
		for (size_t j = 0; j < dataToPush.size(); j++)
		{
			pData[j] = dataToPush[j];
		}
		glUnmapBuffer(bufferType);
	}
}