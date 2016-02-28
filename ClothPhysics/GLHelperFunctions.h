#pragma once
#include <vector>
#include <GL\glew.h>
namespace DEBUG
{
	/* example on how to extract glm::vec4:
	std::vector<glm::vec4> temp = DEBUG::GetBufferData<glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X.size());
	*/
	template<typename p>
	std::vector<p> GetBufferData(GLenum bufferType, unsigned int bufferSize)
	{
		std::vector<p> ret;
		p* pBufData = (p*)glMapBuffer(bufferType, GL_READ_ONLY);
		for (unsigned int i = 0; i < bufferSize; i++)
		{
			ret.push_back( *pBufData++);
		}
		glUnmapBuffer(bufferType);
		return ret;
	}
}