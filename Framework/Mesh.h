#pragma once

#include "CommonHeader.h"
#include "VertexFormats.h"

#include <vector>
#include <string>

using MeshVertex = Vertex_Pos3fColour4ubNormal3fTangent3fTex2f; // vertex type

//================================================================================
// Mesh Class
// Wraps an index and vertex buffer.
// Provides methods for loading a simple model.
//================================================================================
class Mesh
{
public:
	Mesh();
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&);
	Mesh& operator=(Mesh&&);
	~Mesh();

	void init_buffers(ID3D11Device* pDevice, const MeshVertex* pVertices, const u32 kNumVerts, const u16* pIndices, const u32 kNumIndices, const std::string& meshName);
	void bind(ID3D11DeviceContext* pContext) const;
	void bind_structured_VB_SRV(ID3D11DeviceContext*, u32) const;
	void draw(ID3D11DeviceContext* pContext) const;
	void release();

	const std::vector<MeshVertex>& get_vertices() const { return m_vertices; }
	u32 num_vertices() const { return m_numVertices; }
	u32 num_indices() const { return m_numIndices; }
	const std::string& get_name() const { return m_name; }

private:
	std::vector<MeshVertex> m_vertices;
	u32 m_numVertices;
	u32 m_numIndices;
	std::string m_name;

	ID3D11Buffer* m_pVertexBuffer = nullptr;	// Vertex buffer used by the InputAssembler
	ID3D11Buffer* m_pIndexBuffer = nullptr;
	ID3D11Buffer* m_pVertexStructuredBuffer = nullptr;	// Vertex buffer used by other shaders
	ID3D11ShaderResourceView* m_pVertexStructuredBufferSRV = nullptr;
};

//================================================================================
// Helpers for creating mesh data
//================================================================================

void create_mesh_cube(ID3D11Device* pDevice, Mesh& rMeshOut, const f32 kHalfSize, const std::string& meshName);
void create_mesh_quad_xy(ID3D11Device* pDevice, Mesh& rMeshOut, const f32 kHalfSize, const std::string& meshName);
void create_mesh_from_obj(ID3D11Device* pDevice, Mesh& rMeshOut, const char* pFilename, const char* mtlBaseDir, const f32 kScale, const std::string& meshName);
