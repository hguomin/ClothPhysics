#include "Cloth_GPU2.h"
#include "glm\gtc\type_ptr.hpp"

#include "GLError.h"
#include "GLHelperFunctions.h"

Cloth_GPU2::Cloth_GPU2()
{
	setupPositions();
	setupIndices();
	setupHEMesh();
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
	glDeleteBuffers(1, &vboID_Indices);
	glDeleteBuffers(1, &vboID_Normal);
	glDeleteBuffers(1, &vboID_TexCoord);

	glDeleteBuffers(1, &vboID_Struct);
	glDeleteBuffers(1, &vboID_Shear);
	glDeleteBuffers(1, &vboID_Bend);

	m_he_mesh.clear();
}

void Cloth_GPU2::Draw(const Transform& transform, const Camera& camera)
{
	glm::mat4 mMVP = transform.GetMatrix() * camera.GetViewProjection();
	
	Simulate(mMVP);
	
	check_gl_error();
	glBindVertexArray(vaoRenderID[writeID]);
	
	glDisable(GL_CULL_FACE);
	renderShader.Use();
	check_gl_error();
	renderShader.UpdateValues(transform, camera);
	check_gl_error();
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, 0);
	renderShader.UnUse();
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	check_gl_error();
}

void Cloth_GPU2::createVBO()
{
	
	glGenVertexArrays(2, vaoUpdateID);
	glGenVertexArrays(2, vaoRenderID);
	
	glGenBuffers(2, vboID_Pos);
	glGenBuffers(2, vboID_PrePos);
	glGenBuffers(1, &vboID_Indices);
	glGenBuffers(1, &vboID_TexCoord);
	glGenBuffers(1, &vboID_Normal);
	glGenBuffers(1, &vboID_Struct);
	glGenBuffers(1, &vboID_Shear);
	glGenBuffers(1, &vboID_Bend);
	
	glGenTextures(2, texPosID);
	glGenTextures(2, texPrePosID);

	unsigned int max_vertices = 3 * X.size();
	
	//set update vao
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindVertexArray(vaoUpdateID[i]);
		glBindBuffer(GL_ARRAY_BUFFER, vboID_Pos[i]);
		glBufferData(GL_ARRAY_BUFFER, max_vertices*sizeof(glm::vec4), &X[0].x, GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		
		glBindBuffer(GL_ARRAY_BUFFER, vboID_PrePos[i]);
		glBufferData(GL_ARRAY_BUFFER, max_vertices*sizeof(glm::vec4), &X_last[0].x, GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Struct);
		glBufferData(GL_ARRAY_BUFFER, max_vertices*sizeof(glm::ivec4), &struct_springs[0].x, GL_STATIC_READ);
		glEnableVertexAttribArray(2);
		glVertexAttribIPointer(2, 4, GL_INT,  0, 0);
		check_gl_error();

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Shear);
		glBufferData(GL_ARRAY_BUFFER, max_vertices*sizeof(glm::ivec4), &shear_springs[0].x, GL_STATIC_READ);
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 4, GL_INT,  0, 0);
		check_gl_error();

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Bend);
		glBufferData(GL_ARRAY_BUFFER, max_vertices*sizeof(glm::ivec4), &bend_springs[0].x, GL_STATIC_READ);
		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 4, GL_INT, 0, 0);
		check_gl_error();

		glBindBuffer(GL_ARRAY_BUFFER, vboID_Normal);
		glBufferData(GL_ARRAY_BUFFER, 3* max_vertices*sizeof(glm::vec3), nullptr, GL_STATIC_READ);
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

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboID_Indices);
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
	const char* varying_names[] = { 
		"out_position_mass",
		"out_prev_position",
		"out_vertexNormal"};
	glTransformFeedbackVaryings(massSpringShader.getProgram(), 3, varying_names, GL_SEPARATE_ATTRIBS);
	check_gl_error();
	glLinkProgram(massSpringShader.getProgram());
	check_gl_error();
}

void Cloth_GPU2::Simulate(glm::mat4 MVP)
{
	massSpringShader.Use();
	massSpringShader_UploadData(MVP);
	
	check_gl_error();
	for (int i = 0;i<NUM_ITER;i++) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, texPosID[writeID]);
		glBindBuffer(GL_ARRAY_BUFFER, vboID_Pos[writeID]);
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, texPrePosID[writeID]);
		glBindBuffer(GL_ARRAY_BUFFER, vboID_PrePos[writeID]);

		glBindVertexArray(vaoUpdateID[writeID]);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboID_Pos[readID]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, X.size()*sizeof(glm::vec4), &X[0].x, GL_DYNAMIC_COPY);

		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vboID_PrePos[readID]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, X_last.size()*sizeof(glm::vec4), &X_last[0].x, GL_DYNAMIC_COPY);

		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 2, vboID_Normal);
		glEnable(GL_RASTERIZER_DISCARD); //disable rasterization because of transformfeedback calculations

		// begin computation
		glBeginTransformFeedback(GL_POINTS);
		glDrawArrays(GL_POINTS, 0, current_points);
		glEndTransformFeedback();
		//end computation

		check_gl_error();
		
		glFlush();
		glDisable(GL_RASTERIZER_DISCARD); //enable rasterization again

		std::swap(readID, writeID);
	}
	massSpringShader.UnUse();
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
	glUniform1f(massSpringShader("rest_struct"),	springData.rest_struct);
	glUniform1f(massSpringShader("rest_shear"),		springData.rest_shear);
	glUniform1f(massSpringShader("rest_bend"),		springData.rest_bend);
	glUniform1f(massSpringShader("DEFAULT_DAMPING"), DEFAULT_DAMPING);
	glUniform1i(massSpringShader("texsize_x"), texture_size_x);
	glUniform1i(massSpringShader("texsize_y"), texture_size_y);
	glUniform1f(massSpringShader("radius"), 3.0f);

	glm::vec3 ball_pos(0);
	glUniform3fv(massSpringShader("ball_position"),1, glm::value_ptr(ball_pos));
	glUniform2f(massSpringShader("inv_cloth_size"), inv_cloth_size.x, inv_cloth_size.y);
	glUniform2f(massSpringShader("step"), 1.0f / (texture_size_x - 1.0f), 1.0f / (texture_size_y - 1.0f));
	check_gl_error();
}

void Cloth_GPU2::setupPositions()
{
	X.reserve(maximum_split_points);
	X_last.reserve(maximum_split_points);
	Tex_coord.reserve(maximum_split_points);

	X.resize(current_points);
	X_last.resize(current_points);
	Tex_coord.resize(current_points);
	size_t count = 0;
	int v = points_y;
	int u = points_x;
	
	for (int j = 0;j < points_y;j++) {
		for (int i = 0;i < points_x;i++) {
			glm::vec2 texCoord = glm::vec2(float(i) / float(points_x-1), float(j) / float(points_x-1));
			glm::vec4 pos = glm::vec4(((float(i) / (u - 1)) * 2 - 1)* hsize, sizeX + 1, ((float(j) / (v - 1))* sizeY), 1);
			if ((j == 0 && i == 0) || (j == 0 && i == points_x-1)) //force corners to be fixed
			{
				pos.w = 0.0f;
			}
			X[count] = pos;
			X_last[count] = X[count];
			Tex_coord[count] = texCoord;
			count++;
		}
	}
}

void Cloth_GPU2::setupIndices()
{
	int indice_size = 2 * 3 * (points_x - 1)*(points_y - 1);
	indices.reserve(indice_size);
	indices.resize(indice_size);
	GLushort* id = &indices[0];
	for (int i = 0; i < points_y - 1; i++) {
		for (int j = 0; j < points_x - 1; j++) {
			int i0 = i * points_x + j;
			int i1 = i0 + 1;
			int i2 = i0 + points_x;
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
	num_indices = indices.size();
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
	massSpringShader.AddUniform("ksStr");
	massSpringShader.AddUniform("ksShr");
	massSpringShader.AddUniform("ksBnd");
	massSpringShader.AddUniform("kdStr");
	massSpringShader.AddUniform("kdShr");
	massSpringShader.AddUniform("kdBnd");
	massSpringShader.AddUniform("DEFAULT_DAMPING");
	massSpringShader.AddUniform("rest_struct");
	massSpringShader.AddUniform("rest_shear");
	massSpringShader.AddUniform("rest_bend");

	massSpringShader.AddUniform("MVP");
	massSpringShader.AddUniform("dt");
	massSpringShader.AddUniform("gravity");

	massSpringShader.AddUniform("tex_position_mass");
	massSpringShader.AddUniform("tex_pre_position_mass");
	massSpringShader.AddUniform("texsize_x");
	massSpringShader.AddUniform("texsize_y");

	massSpringShader.AddUniform("step");
	massSpringShader.AddUniform("inv_cloth_size");
	massSpringShader.AddUniform("radius");
	massSpringShader.AddUniform("ball_position");

	glUniform1i(massSpringShader("tex_position_mass"), 0); //textureSampler0
	glUniform1i(massSpringShader("tex_pre_position_mass"), 1); //textureSampler1
	
	massSpringShader.UnUse();
}

void Cloth_GPU2::setupHEMesh()
{
	std::vector < trimesh::triangle_t> triangle_mesh;
	fillTriangles(triangle_mesh);
	std::vector<trimesh::edge_t> edges;
	trimesh::unordered_edges_from_triangles(triangle_mesh.size(), &triangle_mesh[0], edges);

	m_he_mesh.build(current_points, triangle_mesh.size(), &triangle_mesh[0], edges.size(), &edges[0]);
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

void Cloth_GPU2::setupSprings()
{
	struct_springs.reserve(maximum_split_points);
	shear_springs.reserve(maximum_split_points);
	bend_springs.reserve(maximum_split_points);
	for (int iy = 0; iy < points_y; iy++)
	{
		for (int ix = 0; ix < points_x; ix++)
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

//assuming split_index is in range
void Cloth_GPU2::Split(const unsigned int split_index, glm::vec3 planeNormal)
{
	//collecting data from GPU
	/*
	glBindVertexArray(vaoUpdateID[writeID]);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vboID_Pos[readID]); //current position
	X = DEBUG::GetBufferData<glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X.size());
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, vboID_PrePos[readID]); //last position
	X_last = DEBUG::GetBufferData<glm::vec4>(GL_TRANSFORM_FEEDBACK_BUFFER, X_last.size());
	*/
	glm::vec3 p1 = glm::vec3(X[split_index]);

	vec neighs_triang;
	vec face_above;
	vec face_below;

	m_he_mesh.vertex_face_neighbors_quad(split_index, neighs_triang);

	for each (trimesh::index_t face in neighs_triang)
	{
		vec triangle_vertices = m_he_mesh.vertices_for_face(face);
		glm::vec3 point1 = glm::vec3(X[triangle_vertices[0]]);
		glm::vec3 point2 = glm::vec3(X[triangle_vertices[1]]);
		glm::vec3 point3 = glm::vec3(X[triangle_vertices[2]]);
		glm::vec3 center = (point1 + point2 + point3) * 0.333333f;
		if (isPointAbovePlane(center, p1, planeNormal))
		{
			face_above.push_back(face);
		}
		else
		{
			face_below.push_back(face);
		}
	}
	//we need atleast one above and one below the cut to split the mesh
	if (face_above.size() > 0 && face_below.size() > 0 && (current_points < maximum_split_points))
	{
		//so we are going to do a cut. Better changing the mass for the point we are cutting
		glm::vec4 split_pos = X[split_index];
		glm::vec4 prev_split_pos = X_last[split_index];
		glm::vec2 tex = Tex_coord[split_index];
		split_pos.w = split_pos.w / 2;
		prev_split_pos.w = prev_split_pos.w / 2;
		X[split_index] = split_pos;
		X_last[split_index] = prev_split_pos;

		X.push_back(split_pos);
		X_last.push_back(prev_split_pos);
		Tex_coord.push_back(tex);
		current_points++;
		//Remapping the springs for the CUT particle
		FixSprings(face_above,face_below, split_index);
		
		m_he_mesh.split_vertex(split_index, face_above, face_below);
	}
	indices = m_he_mesh.get_indices();
	num_indices = indices.size();
}

void Cloth_GPU2::FixSprings(vec& faces_above, vec& faces_below, int split_index)
{
	std::vector<trimesh::index_t> common_verts;
	std::vector<trimesh::index_t> vertices_below;
	vec nearby_vertices = m_he_mesh.vertex_vertex_neighbors(split_index);
	common_verts = getCommonVertices(faces_above, faces_below);
	vertices_below = getVertices(faces_below);

	for each(trimesh::index_t vert in common_verts)
	{
		vertices_below.erase(std::remove(vertices_below.begin(), vertices_below.end(), vert), vertices_below.end());
	}

	FixStructSprings(vertices_below, split_index);
	FixShearSprings(vertices_below, split_index);
	FixBendSprings(vertices_below, split_index);
}

void Cloth_GPU2::FixStructSprings(const vec & vertices_below, int split_index)
{
	glm::ivec4 new_spring;
	new_spring = SplitSpring(struct_springs, split_index, vertices_below);
	struct_springs.push_back(new_spring);
}

void Cloth_GPU2::FixShearSprings(const vec & vertices_below, int split_index)
{
	glm::ivec4 new_spring;
	new_spring = SplitSpring(shear_springs, split_index, vertices_below);
	shear_springs.push_back(new_spring);
}

void Cloth_GPU2::FixBendSprings(const vec & vertices_below, int split_index)
{
	glm::ivec4 new_spring;
	new_spring = SplitSpring(bend_springs, split_index, vertices_below);
	bend_springs.push_back(new_spring);
	/*TODO: Cut bend springs*/
	int split_column = split_index / points_y;
	int above = split_index - points_x;
	int left = split_index - 1;
	if (above >= 0)
	{
		int temp_index = bend_springs[above][DOWN];
		bend_springs[above][DOWN] = -1;
		bend_springs[temp_index][UP] = -1;
	}
	if ((left / points_y) == split_column )
	{
		int temp_index = bend_springs[left][RIGHT];
		bend_springs[left][RIGHT] = -1;
		bend_springs[temp_index][LEFT] = -1;
	}
}

glm::ivec4 Cloth_GPU2::SplitSpring(std::vector<glm::ivec4>& springs, trimesh::index_t split_index, vec indexes_to_remove_from_original)
{
	glm::ivec4 old_spring = springs[split_index];
	glm::ivec4 new_spring = old_spring;
	for each (trimesh::index_t remove_index in indexes_to_remove_from_original)
	{
		for (size_t i = 0; i < 4; i++)
		{
			if (old_spring[i] == remove_index)
			{
				int reverse_dir = getReverseDirection(i);
				springs[remove_index][reverse_dir] = springs.size();
				old_spring[i] = -1;
				new_spring[reverse_dir] = -1;
			}
		}
	}
	springs[split_index] = old_spring;
	return new_spring;
}

unsigned int Cloth_GPU2::getReverseDirection(unsigned int direction)
{
	unsigned int reverse_dir;
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
	return reverse_dir;
}

std::vector<trimesh::index_t> Cloth_GPU2::getCommonVertices(vec faces_above, vec faces_below)
{
	vec common_verts;
	for each (trimesh::index_t faceA in faces_above)
	{
		for each (trimesh::index_t faceB in faces_below)
		{
			vec temp = m_he_mesh.common_vertices_for_faces(faceA, faceB);
			common_verts.insert(common_verts.cend(), temp.cbegin(), temp.cend());
		}
	}
	std::sort(common_verts.begin(), common_verts.end());
	common_verts.erase(std::unique(common_verts.begin(), common_verts.end()),
		common_verts.end());
	return common_verts;
}

std::vector<trimesh::index_t> Cloth_GPU2::getVertices(vec faces)
{
	vec vertices;
	for each (trimesh::index_t faceB in faces)
	{
		vec temp = m_he_mesh.vertices_for_face(faceB);
		vertices.insert(vertices.cend(), temp.cbegin(), temp.cend());
	}
	std::sort(vertices.begin(), vertices.end());
	vertices.erase(std::unique(vertices.begin(), vertices.end()), vertices.end());
	return vertices;
}

bool Cloth_GPU2::isPointAbovePlane(glm::vec3 p1, glm::vec3 pointOnPlane, glm::vec3 planeNormal)
{
	glm::vec3 diff = p1 - pointOnPlane;
	//rounding

	diff.x = floorf(diff.x * 1000) / 1000;
	diff.y = floorf(diff.y * 1000) / 1000;
	diff.z = floorf(diff.z * 1000) / 1000;
	
	float cosTheta = glm::dot(diff, planeNormal);
	if (cosTheta > 0.001)
	{
		return true;
	}
	else
	{
		return false;
	}
}