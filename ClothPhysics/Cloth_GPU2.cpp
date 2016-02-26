#include "Cloth_GPU2.h"
#include "glm\gtc\type_ptr.hpp"

#include "GLError.h"



Cloth_GPU2::Cloth_GPU2()
{
	
	vRed = { 1.0f, 0.0f, 0.0f, 1.0f };
	vBeige = { 1.0f, 0.8f, 0.7f, 1.0f };
	vWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
	vGray = { .25f, .25f, .25f, 1.0f };
	setupPositions();
	setupIndices();
	setupSprings();
	setupShaders();
	
	createVBO();
	
	setupTransformFeedback();
	
}


Cloth_GPU2::~Cloth_GPU2()
{
	X.clear();
	X_last.clear();
	F.clear();
	indices.clear();

	glDeleteTextures(2, texPosID);
	glDeleteTextures(2, texPrePosID);

	glDeleteVertexArrays(2, vaoUpdateID);
	glDeleteVertexArrays(2, vaoRenderID);

	glDeleteBuffers(2, vboID_Pos);
	glDeleteBuffers(2, vboID_PrePos);
	glDeleteBuffers(1, &vboIndices);

	glDeleteTransformFeedbacks(1, &tfID);
}

void Cloth_GPU2::Draw(const Transform& transform, const Camera& camera)
{
	glm::mat4 mMVP = transform.GetMatrix() * camera.GetViewProjection();
	
	massSpringShader.Use();
	glUniformMatrix4fv(massSpringShader("MVP"), 1, GL_FALSE, glm::value_ptr(mMVP));
	glUniform1f(massSpringShader("dt"), timeStep);
	glUniform3fv(massSpringShader("gravity"), 1, &gravity.x);
	glUniform1f(massSpringShader("ksStr"), KsStruct);
	glUniform1f(massSpringShader("ksShr"), KsShear);
	glUniform1f(massSpringShader("ksBnd"), KsBend);
	glUniform1f(massSpringShader("kdStr"), KdStruct / 1000.0f);
	glUniform1f(massSpringShader("kdShr"), KdShear / 1000.0f);
	glUniform1f(massSpringShader("kdBnd"), KdBend / 1000.0f);
	glUniform1f(massSpringShader("DEFAULT_DAMPING"), DEFAULT_DAMPING);
	glUniform1i(massSpringShader("texsize_x"), texture_size_x);
	glUniform1i(massSpringShader("texsize_y"), texture_size_y);
	glUniform1f(massSpringShader("rest_struct"), rest_struct);
	glUniform1f(massSpringShader("rest_shear"), rest_shear);
	glUniform1f(massSpringShader("rest_bend"), rest_bend);

	glUniform2f(massSpringShader("inv_cloth_size"),inv_cloth_size.x, inv_cloth_size.y);
	glUniform2f(massSpringShader("step"), 1.0f / (texture_size_x - 1.0f), 1.0f / (texture_size_y - 1.0f));

	for (int i = 0;i<NUM_ITER;i++) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, texPosID[writeID]);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, texPrePosID[writeID]);

		glBindVertexArray(vaoUpdateID[writeID]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboID_Pos[readID]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vboID_PrePos[readID]);
		glEnable(GL_RASTERIZER_DISCARD);    // disable rasterization

		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, total_points);
		glEndTransformFeedback();

		glFlush();
		glDisable(GL_RASTERIZER_DISCARD);

		std::swap(readID, writeID);
	}
	// get the query result 
	massSpringShader.UnUse();

	//CHECK_GL_ERRORS;

	glBindVertexArray(vaoRenderID[writeID]);
	glDisable(GL_DEPTH_TEST);
	renderShader.Use();
	glUniformMatrix4fv(renderShader("MVP"), 1, GL_FALSE, glm::value_ptr(mMVP));
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);
	renderShader.UnUse();
	glEnable(GL_DEPTH_TEST);
	/*
	if(bDisplayMasses) {
	particleShader.Use();
	glUniformMatrix4fv(particleShader("MV"), 1, GL_FALSE, glm::value_ptr(mMV));
	glUniformMatrix4fv(particleShader("MVP"), 1, GL_FALSE, glm::value_ptr(mMVP));
	//draw the masses last
	glDrawArrays(GL_POINTS, 0, total_points);
	//glDrawTransformFeedbackStream(GL_POINTS, tfID, 0);
	particleShader.UnUse();
	}
	*/
	glBindVertexArray(0);

	//CHECK_GL_ERRORS
}

void Cloth_GPU2::createVBO()
{
	
	glGenVertexArrays(2, vaoUpdateID);
	glGenVertexArrays(2, vaoRenderID);
	
	glGenBuffers(2, vboID_Pos);
	glGenBuffers(2, vboID_PrePos);
	glGenBuffers(1, &vboIndices);

	glGenBuffers(1, &vboID_Struct);
	glGenBuffers(1, &vboID_Shear);
	glGenBuffers(1, &vboID_Bend);
	
	glGenTextures(2, texPosID);
	glGenTextures(2, texPrePosID);

	
	
	//set update vao
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindVertexArray(vaoUpdateID[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vboID_Pos[i]);
		glBufferData(GL_ARRAY_BUFFER, X.size()*sizeof(glm::vec4), &X[0].x, GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		

		glBindBuffer(GL_ARRAY_BUFFER, vboID_PrePos[i]);
		glBufferData(GL_ARRAY_BUFFER, X_last.size()*sizeof(glm::vec4), &X_last[0].x, GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Struct);
		glBufferData(GL_ARRAY_BUFFER, struct_springs.size()*sizeof(glm::ivec4), &struct_springs[0].x, GL_STATIC_COPY);
		glEnableVertexAttribArray(2);
		glVertexAttribIPointer(2, 4, GL_INT,  0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Shear);
		glBufferData(GL_ARRAY_BUFFER, shear_springs.size()*sizeof(glm::ivec4), &shear_springs[0].x, GL_STATIC_COPY);
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT,  0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Bend);
		glBufferData(GL_ARRAY_BUFFER, bend_springs.size()*sizeof(glm::ivec4), &bend_springs[0].x, GL_STATIC_COPY);
		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 4, GL_INT, 0, 0);
		
	}

	//set render vao
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindVertexArray(vaoRenderID[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vboID_Pos[i]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
		if (i == 0) //only need one element array.
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLushort), &indices[0], GL_STATIC_DRAW);
			
		}
	}

	glBindVertexArray(0);

	//bind the positional data into textures
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_BUFFER, texPosID[i]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vboID_Pos[i]);
		

		glBindTexture(GL_TEXTURE_BUFFER, texPrePosID[i]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vboID_PrePos[i]);
		
	}
	glBindVertexArray(0);

	
}

void Cloth_GPU2::setupTransformFeedback()
{
	glGenTransformFeedbacks(1, &tfID);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfID);
	const char* varying_names[] = { "out_position_mass", "out_prev_position" };
	glTransformFeedbackVaryings(massSpringShader.getProgram(), 2, varying_names, GL_SEPARATE_ATTRIBS);
	glLinkProgram(massSpringShader.getProgram());
}

void Cloth_GPU2::setupPositions()
{
	X.resize(total_points);
	X_last.resize(total_points);
	size_t count = 0;
	int v = numY + 1;
	int u = numX + 1;
	//fill in positions
	for (int j = 0;j <= numY;j++) {
		for (int i = 0;i <= numX;i++) {
			glm::vec4 temp = glm::vec4(((float(i) / (u - 1)) * 2 - 1)* hsize, sizeX + 1, ((float(j) / (v - 1))* sizeY), 1);
			if ((j == 0 && i == 0) || (j == 0 && i == numX)) //force corners to be fixed
			{
				temp.w = 0.0f;
			}
			X[count] = temp;
			X_last[count] = X[count];
			count++;
		}
	}
}

void Cloth_GPU2::setupIndices()
{
	indices.resize(2 * 3 * numX*numY);
	//fill in indices
	GLushort* id = &indices[0];
	for (int i = 0; i < numY; i++) {
		for (int j = 0; j < numX; j++) {
			int i0 = i * (numX + 1) + j;
			int i1 = i0 + 1;
			int i2 = i0 + (numX + 1);
			int i3 = i2 + 1;
			if ((j + i) % 2) {
				*id++ = i0; *id++ = i2; *id++ = i1;
				*id++ = i1; *id++ = i2; *id++ = i3;
			}
			else {
				*id++ = i0; *id++ = i2; *id++ = i3;
				*id++ = i0; *id++ = i3; *id++ = i1;
			}
		}
	}
}

void Cloth_GPU2::setupShaders()
{
	massSpringShader.LoadFromFile(GL_VERTEX_SHADER, "shaders/Spring.vp");
	
	particleShader.LoadFromFile(GL_VERTEX_SHADER, "shaders/Basic.vp");
	
	particleShader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/Basic.fp");
	
	renderShader.LoadFromFile(GL_VERTEX_SHADER, "shaders/Passthrough.vp");
	
	renderShader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/Passthrough.fp");
	
	massSpringShader.CreateAndLinkProgram();
	
	massSpringShader.Use();
	
	massSpringShader.AddAttribute("position_mass");
	massSpringShader.AddAttribute("prev_position");
	
	massSpringShader.AddAttribute("spring_struct");
	massSpringShader.AddAttribute("spring_shear");
	massSpringShader.AddAttribute("spring_bend");

	massSpringShader.AddUniform("tex_position_mass");
	massSpringShader.AddUniform("tex_pre_position_mass");
	massSpringShader.AddUniform("MVP");
	massSpringShader.AddUniform("dt");
	massSpringShader.AddUniform("gravity");
	massSpringShader.AddUniform("ksStr");
	massSpringShader.AddUniform("ksShr");
	massSpringShader.AddUniform("ksBnd");
	massSpringShader.AddUniform("kdStr");
	massSpringShader.AddUniform("kdShr");
	massSpringShader.AddUniform("kdBnd");
	massSpringShader.AddUniform("DEFAULT_DAMPING");
	massSpringShader.AddUniform("texsize_x");
	massSpringShader.AddUniform("texsize_y");
	massSpringShader.AddUniform("step");
	massSpringShader.AddUniform("inv_cloth_size");
	massSpringShader.AddUniform("rest_struct");
	massSpringShader.AddUniform("rest_shear");
	massSpringShader.AddUniform("rest_bend");
	massSpringShader.AddUniform("ellipsoid");

	glUniform1i(massSpringShader("tex_position_mass"), 0);
	
	glUniform1i(massSpringShader("tex_pre_position_mass"), 1);
	

	massSpringShader.UnUse();


	particleShader.CreateAndLinkProgram();
	particleShader.Use();
	particleShader.AddAttribute("position_mass");
	particleShader.AddUniform("pointSize");
	particleShader.AddUniform("MV");
	particleShader.AddUniform("MVP");
	particleShader.AddUniform("vColor");
	particleShader.AddUniform("selected_index");
	glUniform1f(particleShader("pointSize"), pointSize);
	glUniform4fv(particleShader("vColor"), 1, &vRed[0]);
	particleShader.UnUse();

	renderShader.CreateAndLinkProgram();
	renderShader.Use();
	renderShader.AddAttribute("position_mass");
	renderShader.AddUniform("MVP");
	renderShader.AddUniform("vColor");
	glUniform4fv(renderShader("vColor"), 1, &vGray[0]);
	renderShader.UnUse();
}

void Cloth_GPU2::setupSprings()
{
	for (int iy = 0; iy <= numY; iy++)
	{
		for (int ix = 0; ix <= numX; ix++)
		{
			std::vector<int> index_neigh;
			for (int k = 0;k < 12;k++) {
				glm::ivec2 coord = getNextNeighbor(k);
				int j = coord.x;
				int i = coord.y;
				if (((iy + i) < 0) || ((iy + i) > (texture_size_y - 1)))
				{
					index_neigh.push_back(-1);
					continue;
				}
				if (((ix + j) < 0) || ((ix + j) > (texture_size_x - 1)))
				{
					index_neigh.push_back(-1);
					continue;
				}
				index_neigh.push_back((iy + i) * texture_size_x + ix + j);
			}
			glm::ivec4 temp;
			for (int i = 0; i < 4; i++)
			{
				temp[i] = index_neigh[i];
			}
			struct_springs.push_back(temp);
			for (int i = 4; i < 8; i++)
			{
				temp[i - 4] = index_neigh[i];
			}
			shear_springs.push_back(temp);
			for (int i = 8; i < 12; i++)
			{
				temp[i - 8] = index_neigh[i];
			}
			bend_springs.push_back(temp);
		}
	}
	rest_struct = glm::length(glm::vec2(0, 1)*inv_cloth_size);
	rest_shear = glm::length(glm::vec2(1, 1)*inv_cloth_size);
	rest_bend = glm::length(glm::vec2(0, 2)*inv_cloth_size);
}

glm::ivec2 Cloth_GPU2::getNextNeighbor(int n) {
	//structural springs (adjacent neighbors)
	//        o
	//        |
	//     o--m--o
	//        |
	//        o
	
	if (n == 0)	return glm::ivec2(1, 0);
	if (n == 1)	return glm::ivec2(0, -1);
	if (n == 2)	return glm::ivec2(-1, 0);
	if (n == 3)	return glm::ivec2(0, 1);

	//shear springs (diagonal neighbors)
	//     o  o  o
	//      \   /
	//     o  m  o
	//      /   \
				//     o  o  o
	
	if (n == 4) return glm::ivec2(1, -1);
	if (n == 5) return glm::ivec2(-1, -1);
	if (n == 6) return glm::ivec2(-1, 1);
	if (n == 7) return glm::ivec2(1, 1);

	//bend spring (adjacent neighbors 1 node away)
	//
	//o   o   o   o   o
	//        | 
	//o   o   |   o   o
	//        |   
	//o-------m-------o
	//        |  
	//o   o   |   o   o
	//        |
	//o   o   o   o   o 
	
	if (n == 8)	return glm::ivec2(2, 0);
	if (n == 9) return glm::ivec2(0, -2);
	if (n == 10) return glm::ivec2(-2, 0);
	if (n == 11) return glm::ivec2(0, 2);
}