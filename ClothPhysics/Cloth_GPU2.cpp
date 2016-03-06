#include "Cloth_GPU2.h"
#include "glm\gtc\type_ptr.hpp"

#include "GLError.h"
#include "GLHelperFunctions.h"



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
	check_gl_error();
	createVBO();
	check_gl_error();
	setupTransformFeedback();
	check_gl_error();
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

	glDeleteTransformFeedbacks(1, &tfID_ForceCalc);
	glDeleteTransformFeedbacks(1, &tfID_SplitCalc);
}

void Cloth_GPU2::Draw(const Transform& transform, const Camera& camera)
{
	glm::mat4 mMVP = transform.GetMatrix() * camera.GetViewProjection();
	
	Simulate(mMVP);
	

	
	//CHECK_GL_ERRORS;
	check_gl_error();
	glBindVertexArray(vaoRenderID[writeID]);
	glDisable(GL_DEPTH_TEST);
	renderShader.Use();
	check_gl_error();
	renderShader.UpdateValues(transform, camera);
	check_gl_error();
	//glUniformMatrix4fv(renderShader("MVP"), 1, GL_FALSE, glm::value_ptr(mMVP));
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);
	renderShader.UnUse();
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	check_gl_error();
	//CHECK_GL_ERRORS
}

//assuming split_index is in range
void Cloth_GPU2::Split(const unsigned int split_index, glm::vec3 planeNormal)
{
	//collecting data from GPU
	glBindVertexArray(vaoUpdateID[writeID]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboID_Pos[readID]); //current position
	X = DEBUG::GetBufferData<glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X.size());
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vboID_PrePos[readID]); //last position
	X_last = DEBUG::GetBufferData<glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X_last.size());
	
	//I am assuming springdata does not change on the GPU so no need to collect that
	//using the spring connections as information about what triangles to draw
	int index_up = struct_springs[split_index][UP];
	int index_down = struct_springs[split_index][DOWN];
	int index_left = struct_springs[split_index][LEFT];
	int index_right = struct_springs[split_index][RIGHT];
	int index_upLeft = shear_springs[split_index][UP_LEFT];
	int index_upRight = shear_springs[split_index][UP_RIGHT];
	int index_downLeft = shear_springs[split_index][DOWN_LEFT];
	int index_downRight = shear_springs[split_index][DOWN_RIGHT];

	//calculate the triangle centers
	/*
	*  - - *  - - *
	|   /  | 1 /  |
	|  / 0 |  / 2 |
	*  - - *  - - *
	| 5 /  | 3 /  |
	|  / 4 |  /   |
	*  - - *  - - *
	*/
	triangle_t tri0;
	triangle_t tri1;
	triangle_t tri2;
	triangle_t tri3;
	triangle_t tri4;
	triangle_t tri5;
	std::vector<triangle_t> triangles;
	glm::vec3 p1 = glm::vec3(X[split_index]);
	//Populate triangle 0
	tri0.ID = 0;
	populateTriangle(tri0, p1, index_left, index_up);
	triangles.push_back(tri0);
	//Populate triangle 1
	tri1.ID = 1;
	populateTriangle(tri1, p1, index_up, index_upRight);
	triangles.push_back(tri1);
	//Populate triangle 2
	tri2.ID = 2;
	populateTriangle(tri2, p1, index_upRight, index_right);
	triangles.push_back(tri2);
	//Populate triangle 3
	tri3.ID = 3;
	populateTriangle(tri3, p1, index_right, index_down);
	triangles.push_back(tri3);
	//Populate triangle 4
	tri4.ID = 4;
	populateTriangle(tri4, p1, index_down,index_downLeft);
	triangles.push_back(tri4);
	//Populate triangle 5
	tri5.ID = 5;
	populateTriangle(tri5, p1, index_downLeft,index_left);
	triangles.push_back(tri5);

	bool oneAbove = false;
	bool oneBelow = false;
	//need both one triangle above the cutt point as well as one below to be able to split
	for each (triangle_t tri in triangles)
	{
		if (tri.exists)
		{
			if (isPointAbovePlane(tri.center,p1, planeNormal))
			{
				oneAbove = true;
			}
			else
			{
				oneBelow = true;
			}
		}
	}
	if (oneAbove && oneBelow)
	{
		//so we are going to do a cut. Better changing the mass for the point we are cutting
		glm::vec4 split_pos = X[split_index];
		glm::vec4 prev_split_pos = X_last[split_index];
		split_pos.w = split_pos.w / 2;
		prev_split_pos.w = prev_split_pos.w / 2;
		X[split_index] = split_pos;
		int new_index = X.size();
		X.push_back(split_pos);
		X_last.push_back(prev_split_pos);
		//for struct spring calculation
		glm::ivec4 before_split_index_struct = struct_springs[split_index];
		glm::ivec4 after_split_index_struct(-1);
		glm::ivec4 before_split_new_stuct(-1);
		glm::ivec4 after_split_new_struct(-1);

		//for shear spring calculation
		glm::ivec4 new_shear = shear_springs[split_index];
		glm::ivec4 new_struct = struct_springs[split_index];
		//Remapping the springs for the CUT particle
		for (unsigned int i = 0; i < 4; i++)
		{
			/*
			if (before_split_index_struct[i] != -1)
			{
				//get the position of the connected point
				glm::vec3 pos = glm::vec3(X[before_split_index_struct[i]]);
				bool above = isPointAbovePlane(pos, p1, planeNormal);
				if (!above) //if the point is below the plane
				{
					//fixes the "old" vertex
					after_split_index_struct = before_split_index_struct;
					after_split_index_struct[i] = -1;
					//now we relink the companion spring below the cutting plane to the "new" vertex
					//first get the companion spring
					glm::ivec4 companionSpring = struct_springs[before_split_index_struct[i]];
					//find the index in the companion spring that is pointing towards the "old" vertex
					//and point it towards the "new" one
					//a bad way of finding but easy to do.
					for (int j = 0; j < 4; j++)
					{
						if (companionSpring[j] == split_index)
						{
							companionSpring[j] = new_index;
						}
					}
					//save back the companionSpring
					struct_springs[before_split_index_struct[i]] = companionSpring;
					//save back the after_index springs
					struct_springs[split_index] = after_split_index_struct;
					//now lets fix the newly created spring index
					//it is a copy of the original spring data, but remove the inverse if companion index
					after_split_new_struct = before_split_index_struct;
					switch (i)
					{
					case(UP) :
						after_split_new_struct[DOWN] = -1;
						break;
					case(DOWN) :
						after_split_new_struct[UP] = -1;
						break;
					case(LEFT) :
						after_split_new_struct[RIGHT] = -1;
						break;
					case(RIGHT) :
						after_split_new_struct[LEFT] = -1;
						break;
					default:
						break;
					}
					//pushback the new spring data for the new vertex
					struct_springs.push_back(after_split_new_struct);
				}
			}
			*/
			FixSprings(struct_springs, new_struct, p1, planeNormal, split_index, new_index, i);
			//check for shear spring
			FixSprings(shear_springs, new_shear, p1, planeNormal, split_index, new_index, i);
			/*
			if (shear_springs[split_index][i] != -1)
			{
				glm::vec3 pos = glm::vec3(X[shear_springs[split_index][i]]);
				bool above = isPointAbovePlane(pos, p1, planeNormal);
				if (!above) //if the point is below the plane
				{
					int temp = shear_springs[split_index][i];
					shear_springs[split_index][i] = -1; //remove link to below
					for (int j = 0; j < 4; j++)
					{
						if (shear_springs[temp][j] == split_index)
						{
							shear_springs[temp][j] = new_index;
						}
					}
					new_shear[i] = temp;
				}
			}
			*/
		}
		shear_springs.push_back(new_shear);
		struct_springs.push_back(new_struct);
	}
	index_down;
}

void Cloth_GPU2::FixSprings(std::vector<glm::ivec4>& springs, glm::ivec4& new_spring, glm::vec3 p1, glm::vec3 planeNormal, int index, int new_index, int xyz)
{
	if (springs[index][xyz] != -1)
	{
		glm::vec3 pos = glm::vec3(X[springs[index][xyz]]);
		bool above = isPointAbovePlane(pos, p1, planeNormal);
		if (!above) //if the point is below the plane
		{
			int temp = springs[index][xyz];
			springs[index][xyz] = -1; //remove link to below
			for (int j = 0; j < 4; j++)
			{
				if (springs[temp][j] == index)
				{
					springs[temp][j] = new_index;
				}
			}
			new_spring[xyz] = temp;
			switch (xyz)
			{
			case(UP) :
				new_spring[DOWN] = -1;
				break;
			case(DOWN) :
				new_spring[UP] = -1;
				break;
			case(LEFT) :
				new_spring[RIGHT] = -1;
				break;
			case(RIGHT) :
				new_spring[LEFT] = -1;
				break;
			default:
				break;
			}
		}
	}
}

void Cloth_GPU2::populateTriangle(triangle_t& tri,glm::vec3 p1, int index2, int index3)
{
	
	if (index2 != - 1 && index3 != -1) //does it even exist
	{
		tri.exists = true;
		tri.p2_index = index2;
		tri.p3_index = index3;
		tri.p2 = glm::vec3(X[index2]);
		tri.p3 = glm::vec3(X[index3]);
		tri.center = glm::vec3((p1 + tri.p2 + tri.p3)*0.333333f); //divide by 3 to get average
	}
	else
	{
		tri.exists = false;
	}
}

bool Cloth_GPU2::isPointAbovePlane(glm::vec3 p1, glm::vec3 pointOnPlane, glm::vec3 planeNormal)
{
	float cosTheta = glm::dot(p1-pointOnPlane, planeNormal);
	if (cosTheta >= 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Cloth_GPU2::createVBO()
{
	
	glGenVertexArrays(2, vaoUpdateID);
	glGenVertexArrays(2, vaoRenderID);
	
	glGenBuffers(2, vboID_Pos);
	glGenBuffers(2, vboID_PrePos);
	glGenBuffers(1, &vboIndices);
	glGenBuffers(1, &vboID_TexCoord);
	glGenBuffers(1, &vboID_Normal);
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
		glBufferData(GL_ARRAY_BUFFER, struct_springs.size()*sizeof(glm::ivec4), &struct_springs[0].x, GL_STATIC_READ);
		glEnableVertexAttribArray(2);
		glVertexAttribIPointer(2, 4, GL_INT,  0, 0);
		check_gl_error();

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Shear);
		glBufferData(GL_ARRAY_BUFFER, shear_springs.size()*sizeof(glm::ivec4), &shear_springs[0].x, GL_STATIC_READ);
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT,  0, 0);
		check_gl_error();

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Bend);
		glBufferData(GL_ARRAY_BUFFER, bend_springs.size()*sizeof(glm::ivec4), &bend_springs[0].x, GL_STATIC_READ);
		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 4, GL_INT, 0, 0);
		check_gl_error();

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Normal);
		glBufferData(GL_ARRAY_BUFFER, X.size()*sizeof(glm::vec3), nullptr, GL_STATIC_READ);
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, 0);
		
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
		glBindBuffer(GL_ARRAY_BUFFER, vboID_TexCoord);
		glBufferData(GL_ARRAY_BUFFER, Tex_coord.size()*sizeof(glm::vec2), &Tex_coord[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Normal);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
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

	check_gl_error();
}

void Cloth_GPU2::setupTransformFeedback()
{
	glGenTransformFeedbacks(1, &tfID_ForceCalc);
	check_gl_error();
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfID_ForceCalc);
	check_gl_error();
	const char* varying_names[] = { 
		"out_position_mass",
		"out_prev_position",
		"out_vertexNormal"};
	glTransformFeedbackVaryings(massSpringShader.getProgram(), 3, varying_names, GL_SEPARATE_ATTRIBS);
	check_gl_error();
	glLinkProgram(massSpringShader.getProgram());
	check_gl_error();
	/*
	glGenTransformFeedbacks(1, &tfID_SplitCalc);
	check_gl_error();
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfID_SplitCalc);
	check_gl_error();
	const char* varying_names2[] =
	{
		"out_index",
		"out_connection"
	};
	glTransformFeedbackVaryings(splitShader.getProgram(), 2, varying_names2, GL_SEPARATE_ATTRIBS);
	check_gl_error();
	*/
}

void Cloth_GPU2::Simulate(glm::mat4 MVP)
{
	massSpringShader.Use();
	massSpringShader_UploadData(MVP);
	
	for (int i = 0;i<NUM_ITER;i++) {
		check_gl_error();
		//setup data in the textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, texPosID[writeID]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, texPrePosID[writeID]);
		glBindVertexArray(vaoUpdateID[writeID]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboID_Pos[readID]); //current position
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vboID_PrePos[readID]); //last position
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, vboID_Normal); //Normal
		check_gl_error();
		glEnable(GL_RASTERIZER_DISCARD);    // disable rasterization
		/*
		//start a query
		GLuint query;
		glGenQueries(1, &query);
		//glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
		glBeginQuery(GL_PRIMITIVES_GENERATED, query);
		*/
		// begin computation
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, total_points);
		glEndTransformFeedback();
		//end computation

		//end query
		//glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
		/*
		glEndQuery(GL_PRIMITIVES_GENERATED);
		GLuint primitives;
		glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);
		printf("%u primitives written!\n\n", primitives);
		*/

		check_gl_error();
		
		//get data from the buffers
		//std::vector<glm::vec3> temp = DEBUG::GetBufferData<glm::vec3>(GL_TRANSFORM_FEEDBACK_BUFFER, X.size());
		//glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboID_Pos[readID]);
		//std::vector<glm::vec4> temp2 = DEBUG::GetBufferData<glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, 3 * X.size());
		
		//make sure all calculations are done
		glFlush();
		glDisable(GL_RASTERIZER_DISCARD); //enable rasterization again

		std::swap(readID, writeID); //switch write and read
	}
	massSpringShader.UnUse();
	//extract data to CPU for ease of use in split (maybe should be in split)
	//X = DEBUG::GetBufferData<glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X.size());
}

void Cloth_GPU2::massSpringShader_UploadData(glm::mat4 MVP)
{
	glUniformMatrix4fv(massSpringShader("MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
	glUniform1f(massSpringShader("dt"), timeStep);
	glUniform3fv(massSpringShader("gravity"), 1, &gravity.x);
	glUniform1f(massSpringShader("ksStr"), springData.KsStruct);
	glUniform1f(massSpringShader("ksShr"), springData.KsShear);
	glUniform1f(massSpringShader("ksBnd"), springData.KsBend);
	glUniform1f(massSpringShader("kdStr"), springData.KdStruct / 1000.0f);
	glUniform1f(massSpringShader("kdShr"), springData.KdShear / 1000.0f);
	glUniform1f(massSpringShader("kdBnd"), springData.KdBend / 1000.0f);
	glUniform1f(massSpringShader("DEFAULT_DAMPING"), DEFAULT_DAMPING);
	glUniform1i(massSpringShader("texsize_x"), texture_size_x);
	glUniform1i(massSpringShader("texsize_y"), texture_size_y);
	glUniform1f(massSpringShader("rest_struct"), springData.rest_struct);
	glUniform1f(massSpringShader("rest_shear"), springData.rest_shear);
	glUniform1f(massSpringShader("rest_bend"), springData.rest_bend);
	glUniform1f(massSpringShader("radius"), 3.0f);
	glm::vec3 ball_pos(0);
	glUniform3fv(massSpringShader("ball_position"),1, glm::value_ptr(ball_pos));
	check_gl_error();

	glUniform2f(massSpringShader("inv_cloth_size"), inv_cloth_size.x, inv_cloth_size.y);
	glUniform2f(massSpringShader("step"), 1.0f / (texture_size_x - 1.0f), 1.0f / (texture_size_y - 1.0f));
}

void Cloth_GPU2::setupPositions()
{
	X.resize(total_points);
	X_last.resize(total_points);
	Tex_coord.resize(total_points);
	size_t count = 0;
	int v = numY + 1;
	int u = numX + 1;
	//fill in positions
	for (int j = 0;j <= numY;j++) {
		for (int i = 0;i <= numX;i++) {
			glm::vec2 tempTex = glm::vec2(float(i) / float(numX), float(j) / float(numX));
			glm::vec4 temp = glm::vec4(((float(i) / (u - 1)) * 2 - 1)* hsize, sizeX + 1, ((float(j) / (v - 1))* sizeY), 1);
			if ((j == 0 && i == 0) || (j == 0 && i == numX)) //force corners to be fixed
			{
				temp.w = 0.0f;
			}
			X[count] = temp;
			X_last[count] = X[count];
			Tex_coord[count] = tempTex;
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
				*id++ = i0; *id++ = i2; *id++ = i1;
				*id++ = i1; *id++ = i2; *id++ = i3;
			}
		}
	}
}

void Cloth_GPU2::setupShaders()
{
	massSpringShader.LoadFromFile(GL_VERTEX_SHADER, "shaders/Spring.vert");
	massSpringShader.LoadFromFile(GL_GEOMETRY_SHADER, "shaders/Spring.geom");
	
	splitShader.LoadFromFile(GL_VERTEX_SHADER, "shaders/Split.vert");
	check_gl_error();
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
	massSpringShader.AddUniform("radius");
	massSpringShader.AddUniform("ball_position");

	glUniform1i(massSpringShader("tex_position_mass"), 0);
	
	glUniform1i(massSpringShader("tex_pre_position_mass"), 1);
	

	massSpringShader.UnUse();
	

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
	springData.rest_struct = glm::length(glm::vec2(0, 1)*inv_cloth_size);
	springData.rest_shear = glm::length(glm::vec2(1, 1)*inv_cloth_size);
	springData.rest_bend = glm::length(glm::vec2(0, 2)*inv_cloth_size);
	float springConstant = 50.75f;
	float springDamp = -0.25f;
	springData.KsStruct = springConstant;
	springData.KdStruct = springDamp;

	springData.KsShear= springConstant;
	springData.KdShear= springDamp;

	springData.KsBend= springConstant;
	springData.KdBend= springDamp;
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