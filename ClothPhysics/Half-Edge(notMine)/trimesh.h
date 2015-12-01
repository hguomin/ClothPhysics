#ifndef __trimesh_h__
#define __trimesh_h__

/*This code is taken from https://github.com/yig/halfedge 29 november 2015.
Author: Yotam Gingold <yotam (strudel) yotamgingold.com>
License: Public Domain.  (I, Yotam Gingold, the author, release this code into the public domain.)
GitHub: https://github.com/yig/halfedge
*/

#include "trimesh_types.h" // triangle_t, edge_t
#include <vector>
#include <map>

namespace trimesh
{

	// trimesh_t::build() needs the unordered edges of the mesh.  If you don't have them, call this first.
	void unordered_edges_from_triangles(const unsigned long num_triangles, const trimesh::triangle_t* triangles, std::vector< trimesh::edge_t >& edges_out);

	class trimesh_t
	{
	public:
		// I need positive and negative numbers so that I can use -1 for an invalid index.
		typedef long index_t;

		struct halfedge_t
		{
			// Index into the vertex array.
			index_t to_vertex;
			// Index into the face array.
			index_t face;
			// Index into the edges array.
			index_t edge;
			// Index into the halfedges array.
			index_t opposite_he;
			// Index into the halfedges array.
			index_t next_he;
			//index into the halfedges array, before destruction
			index_t ghost_he;

			halfedge_t() :
				to_vertex(-1),
				face(-1),
				edge(-1),
				opposite_he(-1),
				next_he(-1),
				ghost_he(-1) //used for cloth destruction
			{}
		};

		// Builds the half-edge data structures from the given triangles and edges.
		// NOTE: 'edges' can be derived from 'triangles' by calling
		//       unordered_edges_from_triangles(), above.  build() does not
		//       but could do this for callers who do not already have edges.
		// NOTE: 'triangles' and 'edges' are not needed after the call to build()
		//       completes and may be destroyed.
		void build(const unsigned long num_vertices, const unsigned long num_triangles, const trimesh::triangle_t* triangles, const unsigned long num_edges, const trimesh::edge_t* edges);

		void clear();

		const halfedge_t& halfedge(const index_t i) const { return m_halfedges.at(i); }

		/*
		Given the index of a halfedge_t, returns the corresponding directed edge (i,j).

		untested
		*/
		std::pair< index_t, index_t > he_index2directed_edge(const index_t he_index) const;
		/*
		Given a directed edge (i,j), returns the index of the 'halfedge_t' in
		halfedges().

		untested
		*/
		index_t directed_edge2he_index(const index_t i, const index_t j) const;

		/*
		Returns in 'result' the vertex neighbors (as indices) of the vertex 'vertex_index'.

		untested
		*/
		void vertex_vertex_neighbors(const index_t vertex_index, std::vector< index_t >& result) const;

		std::vector< index_t > vertex_vertex_neighbors(const index_t vertex_index) const;

		/*
		Returns the valence (number of vertex neighbors) of vertex with index 'vertex_index'.

		untested
		*/
		int vertex_valence(const index_t vertex_index) const;

		/*
		Returns in 'result' the face neighbors (as indices) of the vertex 'vertex_index'.

		untested
		*/
		void vertex_face_neighbors(const index_t vertex_index, std::vector< index_t >& result) const;
		std::vector< index_t > vertex_face_neighbors(const index_t vertex_index) const;

		/*
		Returns whether the vertex with given index is on the boundary.

		untested
		*/
		bool vertex_is_boundary(const index_t vertex_index) const;

		/*
		Returns in 'result' the three vertices that builds face_index
		*/
		void vertices_for_face(const index_t face_index, std::vector< index_t>& result) const;
		std::vector< index_t > vertices_for_face(const index_t face_index) const;

		void halfedge_for_face(const index_t face_index, std::vector<trimesh::trimesh_t::halfedge_t>& result) const;
		std::vector<trimesh::trimesh_t::halfedge_t> halfedge_for_face(const index_t face_index) const;

		std::vector< index_t > boundary_vertices() const;

		std::vector< std::pair< index_t, index_t > > boundary_edges() const;

	private:
		std::vector< halfedge_t > m_halfedges;
		// Offsets into the 'halfedges' sequence, one per vertex.
		std::vector< index_t > m_vertex_halfedges;
		// Offset into the 'halfedges' sequence, one per face.
		std::vector< index_t > m_face_halfedges;
		// Offset into the 'halfedges' sequence, one per edge (unordered pair of vertex indices).
		std::vector< index_t > m_edge_halfedges;
		// A map from an ordered edge (an std::pair of index_t's) to an offset into the 'halfedge' sequence.
		typedef std::map< std::pair< index_t, index_t >, index_t > directed_edge2index_map_t;
		directed_edge2index_map_t m_directed_edge2he_index;
	};

}

#endif /* __trimesh_h__ */