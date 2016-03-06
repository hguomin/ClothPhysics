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
	
	glDisable(GL_CULL_FACE);
	renderShader.Use();
	check_gl_error();
	renderShader.UpdateValues(transform, camera);
	check_gl_error();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(GLushort), &indices[0], GL_DYNAMIC_COPY);
	std::vector<GLushort> tmp = DEBUG::GetBufferData<GLushort>(GL_ELEMENT_ARRAY_BUFFER, indices.size());
	glBindBuffer(GL_ARRAY_BUFFER, vboID_TexCoord);
	glBufferData(GL_ARRAY_BUFFER, Tex_coord.size()*sizeof(glm::vec2), &Tex_coord[0], GL_DYNAMIC_COPY);

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
	//set up the halfedge mesh
	std::vector < trimesh::triangle_t> triangle_mesh;
	fillTriangles(triangle_mesh);

	std::vector<trimesh::edge_t> edges;
	trimesh::unordered_edges_from_triangles(triangle_mesh.size(), &triangle_mesh[0], edges);

	trimesh::trimesh_t mesh;
	mesh.build(total_points, triangle_mesh.size(), &triangle_mesh[0], edges.size(), &edges[0]);

	std::vector<trimesh::index_t> neighs_triang;
	mesh.vertex_face_neighbors(split_index, neighs_triang);

	std::vector<glm::vec3> triangle_center;
	triangle_center.resize(neighs_triang.size());

	glm::vec3 p1 = glm::vec3(X[split_index]);
	
	bool oneAbove = false;
	bool oneBelow = false;
	//need both one triangle above the cutt point as well as one below to be able to split
	for each (trimesh::index_t face in neighs_triang)
	{
		glm::vec3 point1 = glm::vec3(X[triangle_mesh[face].v[0]]);
		glm::vec3 point2 = glm::vec3(X[triangle_mesh[face].v[1]]);
		glm::vec3 point3 = glm::vec3(X[triangle_mesh[face].v[2]]);
		glm::vec3 center = (point1 + point2 + point3) * 0.333333f;
		if (isPointAbovePlane(center,p1, planeNormal))
		{
			oneAbove = true;
		}
		else
		{
			oneBelow = true;
		}
	}
	if (oneAbove && oneBelow)
	{
		//so we are going to do a cut. Better changing the mass for the point we are cutting
		glm::vec4 split_pos = X[split_index];
		glm::vec4 prev_split_pos = X_last[split_index];
		glm::vec2 tex = Tex_coord[split_index];
		split_pos.w = split_pos.w / 2;
		prev_split_pos.w = prev_split_pos.w / 2;
		X[split_index] = split_pos;
		X_last[split_index] = prev_split_pos;
		int new_index = X.size();
		X.push_back(split_pos);
		X_last.push_back(prev_split_pos);
		Tex_coord.push_back(tex);
		splitAdded = false;
		//for struct spring calculation
		
		//for shear spring calculation
		glm::ivec4 new_shear(-1);
		glm::ivec4 new_struct = struct_springs[split_index];
		glm::ivec4 new_bend = bend_springs[split_index];
		//Remapping the springs for the CUT particle
		for (unsigned int i = 0; i < 4; i++)
		{
			
			FixSprings(struct_springs, new_struct, p1, planeNormal, split_index, new_index, i, SPRING::STRUCT);
			//check for shear spring
			FixSprings(shear_springs, new_shear, p1, planeNormal, split_index, new_index, i, SPRING::SHEAR);
			FixSprings(bend_springs, new_bend, p1, planeNormal, split_index, new_index, i, SPRING::BEND);
			
		}
		shear_springs.push_back(new_shear);
		struct_springs.push_back(new_struct);
		bend_springs.push_back(new_bend);
		//now lets fix the indices for the triangles below the cutting point
		for each (trimesh::index_t index in neighs_triang)
		{
			glm::vec3 p1 = glm::vec3(X[triangle_mesh[index].v[0]]);
			glm::vec3 p2 = glm::vec3(X[triangle_mesh[index].v[1]]);
			glm::vec3 p3 = glm::vec3(X[triangle_mesh[index].v[2]]);
			glm::vec3 center = (p1 + p2 + p3) * 0.333333f;
			if (!isPointAbovePlane(center, glm::vec3(split_pos), planeNormal))
			{
				trimesh::index_t currentIndex = mesh.m_face_halfedges.at(index);
				for (int  i = 0; i < 3; i++)
				{
					long tovert = mesh.m_halfedges.at(currentIndex).to_vertex;
					if (tovert == split_index)
					{
						trimesh::trimesh_t::halfedge_t tmp = mesh.halfedge_ref(currentIndex);
						tmp.to_vertex = new_index;
						mesh.m_halfedges.at(currentIndex) = tmp;
						//delete(tmp);
					}
					currentIndex = mesh.halfedge(currentIndex).next_he;
				}
			}
		}
	}
	std::vector<GLushort> newIndices = calculateIndices(mesh);
	indices = newIndices;
	//Update Buffers
	if (!splitAdded)
	{
		for (int i = 0; i < 2; i++)
		{
			glBindVertexArray(vaoUpdateID[i]);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboID_Pos[i]); //current position
			glBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, X.size()*sizeof(glm::vec4), &X[0].x);
			std::vector<glm::vec4> POS = DEBUG::GetBufferData <glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X.size());
			check_gl_error();
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vboID_PrePos[i]); //last position
			check_gl_error();
			glBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, X_last.size()*sizeof(glm::vec4), &X_last[0].x);
			std::vector<glm::vec4> PRE_POS = DEBUG::GetBufferData <glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X_last.size());
			check_gl_error();
		}
		splitAdded = true;
	}
	total_points++;
}

std::vector<GLushort> Cloth_GPU2::calculateIndices(trimesh::trimesh_t halfedge_mesh)
{
	std::vector<GLushort> ret;
	for each (trimesh::index_t face in halfedge_mesh.m_face_halfedges)
	{
		trimesh::trimesh_t::halfedge_t halfedge = halfedge_mesh.m_halfedges.at(face);
		ret.push_back(halfedge.to_vertex);
		halfedge = halfedge_mesh.halfedge_ref(halfedge.next_he);
		ret.push_back(halfedge.to_vertex);
		halfedge = halfedge_mesh.halfedge_ref(halfedge.next_he);
		ret.push_back(halfedge.to_vertex);
	}
	return ret;
}

void Cloth_GPU2::fillTriangles(std::vector<trimesh::triangle_t>& triang)
{
	triang.resize(indices.size() / 3);
	int count = 0;
	for (int i = 0; i < indices.size(); i++)
	{
		triang[count].v[0] = indices[i];
		i++;
		triang[count].v[1] = indices[i];
		i++;
		triang[count].v[2] = indices[i];
		count++;
	}
}

void Cloth_GPU2::FixSprings(std::vector<glm::ivec4>& springs, glm::ivec4& new_spring, glm::vec3 p1, glm::vec3 planeNormal, int index, int new_index, int direction, SPRING springType)
{
	if (springs[index][direction] != -1)
	{
		glm::vec3 pos = glm::vec3(X[springs[index][direction]]);
		bool above = isPointAbovePlane(pos, p1, planeNormal);
		if (!above) //if the point is below the plane
		{
			int temp = springs[index][direction];
			springs[index][direction] = -1; //remove link to below plane
			for (int j = 0; j < 4; j++)
			{
				if (springs[temp][j] == index)
				{
					springs[temp][j] = new_index;
				}
			}
			new_spring[direction] = temp;
			//remap so that we don't use the opposite
			int reverse_dir;
			switch (direction) //shear springs use the same winding, so the opposite is at the same position as for a struct/bend string
			{
			case(UP) :
				reverse_dir = DOWN;
				break;
			case(DOWN) :
				reverse_dir = UP;
				break;
			case(LEFT) :
				reverse_dir = RIGHT;
				break;
			case(RIGHT) :
				reverse_dir = LEFT;
				break;
			default:
				break;
			}
			new_spring[reverse_dir] = -1;
			//if we are in a struct, lets also remove the cut bend because we have the data ready
			if (springType == SPRING::STRUCT)
			{
				int bend_above = bend_springs[temp][reverse_dir];
				bend_springs[temp][reverse_dir] = -1;
				bend_springs[bend_above][direction] = -1;
			}
		}
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
		glBufferData(GL_ARRAY_BUFFER, 3*X.size()*sizeof(glm::vec4), &X[0].x, GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		

		glBindBuffer(GL_ARRAY_BUFFER, vboID_PrePos[i]);
		glBufferData(GL_ARRAY_BUFFER, 3*X_last.size()*sizeof(glm::vec4), &X_last[0].x, GL_DYNAMIC_COPY);
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
		glBufferData(GL_ARRAY_BUFFER, 3*X.size()*sizeof(glm::vec3), nullptr, GL_STATIC_READ);
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
	//Syncronize the buffer data. We might have had a cut
	
	
	
	//update spring data
	/*
	for (int i = 0; i < 2; i++)
	{
		glBindVertexArray(vaoUpdateID[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vboID_Struct);
		glBufferData(GL_ARRAY_BUFFER, struct_springs.size()*sizeof(glm::ivec4), &struct_springs[0].x, GL_STATIC_DRAW);
		check_gl_error();

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Shear);
		glBufferData(GL_ARRAY_BUFFER, shear_springs.size()*sizeof(glm::ivec4), &shear_springs[0].x, GL_STATIC_DRAW);
		check_gl_error();

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Bend);
		glBufferData(GL_ARRAY_BUFFER, bend_springs.size()*sizeof(glm::ivec4), &bend_springs[0].x, GL_STATIC_DRAW);
		check_gl_error();
	}
	*/
	for (int i = 0;i<NUM_ITER;i++) {
		check_gl_error();
		//setup data in the textures
		glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_BUFFER, texPosID[readID]); /*remove this*/
		//glBindBuffer(GL_ARRAY_BUFFER, vboID_Pos[readID]); /*remove this*/
		//std::vector<glm::vec4> texBuff0Read = DEBUG::GetBufferData <glm::vec4>(GL_ARRAY_BUFFER, X.size());
		glBindTexture(GL_TEXTURE_BUFFER, texPosID[writeID]);
		glBindBuffer(GL_ARRAY_BUFFER, vboID_Pos[writeID]);
		std::vector<glm::vec4> texBuff0Write = DEBUG::GetBufferData <glm::vec4>(GL_ARRAY_BUFFER, X.size());
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, texPrePosID[writeID]);
		glBindBuffer(GL_ARRAY_BUFFER, vboID_PrePos[writeID]);
		std::vector<glm::vec4> texBuff1Write = DEBUG::GetBufferData <glm::vec4>(GL_ARRAY_BUFFER, X_last.size());

		glBindVertexArray(vaoUpdateID[writeID]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboID_Pos[readID]); //current position
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, X.size()*sizeof(glm::vec4), &X[0].x, GL_DYNAMIC_COPY);
		std::vector<glm::vec4> vboID_POS = DEBUG::GetBufferData <glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X.size());

		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vboID_PrePos[readID]); //last position
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, X_last.size()*sizeof(glm::vec4), &X_last[0].x, GL_DYNAMIC_COPY);
		std::vector<glm::vec4> vboID_prePOS = DEBUG::GetBufferData <glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X_last.size());

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