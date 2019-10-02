
#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader/tiny_obj_loader.h"

//==================================
// Mesh Class Implementation
//==================================

Mesh::Mesh()
	: m_pVertexBuffer(nullptr)
	, m_pIndexBuffer(nullptr)
{}

Mesh::~Mesh()
{
	release();
}

Mesh::Mesh(Mesh&& other) :
	m_vertices(other.m_vertices),
	m_pVertexBuffer(nullptr),
	m_pIndexBuffer(nullptr),
	m_pVertexStructuredBuffer(nullptr),
	m_pVertexStructuredBufferSRV(nullptr),
	m_numVertices(other.m_numVertices),
	m_numIndices(other.m_numIndices),
	m_name(other.m_name)
{
	// Take ownership of the other's contents
	m_pVertexBuffer = other.m_pVertexBuffer;
	m_pIndexBuffer = other.m_pIndexBuffer;
	m_pVertexStructuredBuffer = other.m_pVertexStructuredBuffer;
	m_pVertexStructuredBufferSRV = other.m_pVertexStructuredBufferSRV;
	// Prevent multiple attempts to free resources
	other.m_pVertexBuffer = nullptr;
	other.m_pIndexBuffer = nullptr;
	other.m_pVertexStructuredBuffer = nullptr;
	other.m_pVertexStructuredBufferSRV = nullptr;
}

Mesh& Mesh::operator=(Mesh&& other)
{
	if (this != &other)
	{
		// Release this object's old resources
		release();
		// Copy other object into this one
		m_vertices = other.m_vertices;
		m_numVertices = other.m_numVertices;
		m_numIndices = other.m_numIndices;
		m_name = other.m_name;
		m_pVertexBuffer = other.m_pVertexBuffer;
		m_pIndexBuffer = other.m_pIndexBuffer;
		m_pVertexStructuredBuffer = other.m_pVertexStructuredBuffer;
		m_pVertexStructuredBufferSRV = other.m_pVertexStructuredBufferSRV;
		// Prevent multiple attempts to free resources
		other.m_pVertexBuffer = nullptr;
		other.m_pIndexBuffer = nullptr;
		other.m_pVertexStructuredBuffer = nullptr;
		other.m_pVertexStructuredBufferSRV = nullptr;
	}

	return *this;
}

void Mesh::init_buffers(ID3D11Device* pDevice, const MeshVertex* pVertices, const u32 kNumVerts, const u16* pIndices, const u32 kNumIndices, const std::string& meshName)
{
	ASSERT(!m_pVertexBuffer && !m_pIndexBuffer);

	if (pVertices)
	{
		// Copy the array of vertex data into the mesh class before it is destroyed
		m_vertices.resize(kNumVerts);
		for (size_t i = 0; i < kNumVerts; ++i)
			m_vertices[i] = pVertices[i];

		// Create a vertex buffer
		D3D11_BUFFER_DESC vb_desc = {};
		vb_desc.ByteWidth = sizeof(MeshVertex) * kNumVerts;
		vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
		vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vb_data = {};
		vb_data.pSysMem = pVertices;

		HRESULT hr = pDevice->CreateBuffer(&vb_desc, &vb_data, &m_pVertexBuffer);
		ASSERT(!FAILED(hr));

		D3D11_SUBRESOURCE_DATA vsb_data = {};
		vsb_data.pSysMem = pVertices;
		vsb_data.SysMemPitch = 0;
		vsb_data.SysMemSlicePitch = 0;

		// Create a vertex structured buffer for use in other shaders
		m_pVertexStructuredBuffer =
			create_default_structured_buffer<MeshVertex>(pDevice, kNumVerts, &vsb_data);

		// Create an SRV to the vertex structured buffer
		m_pVertexStructuredBufferSRV = 
			create_structured_buffer_SRV(pDevice, kNumVerts, m_pVertexStructuredBuffer);
	}

	// Create an index buffer
	if (pIndices)
	{
		D3D11_BUFFER_DESC ib_desc = {};
		ib_desc.ByteWidth = sizeof(u16) * kNumIndices;
		ib_desc.Usage = D3D11_USAGE_IMMUTABLE;
		ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

		D3D11_SUBRESOURCE_DATA ib_data = {};
		ib_data.pSysMem = pIndices;

		HRESULT hr = pDevice->CreateBuffer(&ib_desc, &ib_data, &m_pIndexBuffer);
		ASSERT(!FAILED(hr));
	}

	m_numVertices = kNumVerts;
	m_numIndices = kNumIndices;
	m_name = meshName;
}

void Mesh::bind(ID3D11DeviceContext* pContext) const
{
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11Buffer* buffers[] = { m_pVertexBuffer };
	UINT strides[] = { sizeof(MeshVertex) };
	UINT offsets[] = { 0 };
	pContext->IASetVertexBuffers(0, 1, buffers, strides, offsets);

	if (m_pIndexBuffer)
	{
		pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	}
}

void Mesh::bind_structured_VB_SRV(ID3D11DeviceContext* pContext, u32 slot) const
{
	pContext->CSSetShaderResources(slot, 1, &m_pVertexStructuredBufferSRV);
}

void Mesh::draw(ID3D11DeviceContext* pContext) const
{
	if (m_pIndexBuffer)
	{
		pContext->DrawIndexed(m_numIndices, 0, 0);
	}
	else
	{
		pContext->Draw(m_numVertices, 0);
	}
}

void Mesh::release()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pIndexBuffer);
	SAFE_RELEASE(m_pVertexStructuredBuffer);
	SAFE_RELEASE(m_pVertexStructuredBufferSRV);
}

// Computes tangents using Lengyel's method for an indexed triangle list.
// Tangents are computed as a 4d vector where w stores the sign need to reconstruct a bitangent in the shader.
void compute_tangents_lengyel(MeshVertex* pVertices, u32 kVertices, const u16* pIndices, u32 kIndices)
{
	using namespace DirectX;

	const u32 kTris = kIndices / 3;

	// Tangents are accumulated so we need some space to work in.
	v3* buffer = new v3[kVertices * 2];
	memset(buffer, 0, sizeof(v3) * kVertices * 2);
	
	// offsets into the buffer;
	v3* tan1 = buffer;
	v3* tan2 = buffer + kVertices;

	// Step through each triangle.
	for (u32 iTri = 0; iTri < kTris; ++iTri)
	{
		u16 i1 = pIndices[0];
		u16 i2 = pIndices[1];
		u16 i3 = pIndices[2];

		v3 p1 = pVertices[i1].pos;
		v3 p2 = pVertices[i2].pos;
		v3 p3 = pVertices[i3].pos;

		v2 w1 = pVertices[i1].tex;
		v2 w2 = pVertices[i2].tex;
		v2 w3 = pVertices[i3].tex;

		f32 x1 = p2.x - p1.x;
		f32 x2 = p3.x - p1.x;
		f32 y1 = p2.y - p1.y;
		f32 y2 = p3.y - p1.y;
		f32 z1 = p2.z - p1.z;
		f32 z2 = p3.z - p1.z;

		f32 s1 = w2.x - w1.x;
		f32 s2 = w3.x - w1.x;
		f32 t1 = w2.y - w1.y;
		f32 t2 = w3.y - w1.y;

		f32 r = 1.f / (s1 * t2 - s2 * t1);
		v3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		v3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		// accumulate the tangents
		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;

		pIndices += 3;
	}

	// Step through each vertex.
	for (u32 i = 0; i < kVertices; ++i)
	{
		XMVECTOR n = XMLoadFloat3(&pVertices[i].normal);
		XMVECTOR t1 = XMLoadFloat3(&tan1[i]);
		XMVECTOR t2 = XMLoadFloat3(&tan2[i]);
		
		// Gram-Schmidt Orthogonalization
		XMVECTOR tangent = XMVector3Normalize(t1 - n * XMVector3Dot(n, t1));
		XMVECTOR bitangent = XMVector3Dot(XMVector3Cross(n, t1), t2);

		XMStoreFloat4(&pVertices[i].tangent, tangent);
		pVertices[i].tangent.w = XMVectorGetX(bitangent) < 0.f ? -1.0f : 1.0f; // sign
	}

	// cleanup the temp buffer
	delete[] buffer;
}

void create_mesh_cube(ID3D11Device* pDevice, Mesh& rMeshOut, const f32 kHalfSize, const std::string& meshName)
{
	// define the vertices
	const f32 s = kHalfSize;
	const u32 c = 0xFF808080;

	const u32 colours[6] = {
		0xFF800000,	  // front
		0xFF008000,	  // right
		0xFF000080,	  // back
		0xFF808000,	  // left
		0xFF800080,	  // top
		0xFF008080	  // bottom
	};

	const v3 normals[6] =
	{
		v3(0, 0, 1),	// front
		v3(1, 0, 0),	// right
		v3(0, 0, -1),	// back
		v3(-1, 0, 0),	// left
		v3(0, 1, 0),	// top
		v3(0, -1, 0)	// bottom
	};

	const v2 texCoords[4] = {
		v2(0, 0),
		v2(1, 0),
		v2(1, 1),
		v2(0, 1)
	};

	MeshVertex verts[] = {
		//front
		MeshVertex(v3(-s, -s, s), colours[0], normals[0], texCoords[0]),
		MeshVertex(v3(s, -s, s), colours[0], normals[0], texCoords[1]),
		MeshVertex(v3(s, s, s), colours[0], normals[0], texCoords[2]),
		MeshVertex(v3(-s, s, s), colours[0], normals[0], texCoords[3]),

		//right
		MeshVertex(v3(s, s, s), colours[1], normals[1], texCoords[0]),
		MeshVertex(v3(s, s, -s), colours[1], normals[1], texCoords[1]),
		MeshVertex(v3(s, -s, -s), colours[1], normals[1], texCoords[2]),
		MeshVertex(v3(s, -s, s), colours[1], normals[1], texCoords[3]),

		//back
		MeshVertex(v3(-s, -s, -s), colours[2], normals[2], texCoords[0]),
		MeshVertex(v3(s, -s, -s), colours[2], normals[2], texCoords[1]),
		MeshVertex(v3(s, s, -s), colours[2], normals[2], texCoords[2]),
		MeshVertex(v3(-s, s, -s), colours[2], normals[2], texCoords[3]),

		//left
		MeshVertex(v3(-s, -s, -s), colours[3], normals[3], texCoords[0]),
		MeshVertex(v3(-s, -s, s), colours[3], normals[3], texCoords[1]),
		MeshVertex(v3(-s, s, s), colours[3], normals[3], texCoords[2]),
		MeshVertex(v3(-s, s, -s), colours[3], normals[3], texCoords[3]),

		//top
		MeshVertex(v3(s, s, s), colours[4], normals[4], texCoords[0]),
		MeshVertex(v3(-s, s, s), colours[4], normals[4], texCoords[1]),
		MeshVertex(v3(-s, s, -s), colours[4], normals[4], texCoords[2]),
		MeshVertex(v3(s, s, -s), colours[4], normals[4], texCoords[3]),

		//bottom
		MeshVertex(v3(-s, -s, -s), colours[5], normals[5], texCoords[0]),
		MeshVertex(v3(s, -s, -s), colours[5], normals[5], texCoords[1]),
		MeshVertex(v3(s, -s, s), colours[5], normals[5], texCoords[2]),
		MeshVertex(v3(-s, -s, s), colours[5], normals[5], texCoords[3]),

	};

	const u32 kVertices = sizeof(verts) / sizeof(verts[0]);

	// and indices
	const u16 indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 13, 14, 12, 14, 15,  // left
		16, 17, 18, 16, 18, 19,  // top
		20, 21, 22, 20, 22, 23   // bottom
	};

	const u32 kIndices = sizeof(indices) / sizeof(indices[0]);

	compute_tangents_lengyel(verts, kVertices, indices, kIndices);

	rMeshOut.init_buffers(pDevice, verts, kVertices, indices, kIndices, meshName);
}

void create_mesh_quad_xy(ID3D11Device* pDevice, Mesh& rMeshOut, const f32 kHalfSize, const std::string& meshName)
{
	// define the vertices
	const f32 s = kHalfSize;
	const u32 c = 0xFF808080;

	const u32 colours[] = {
		0xFFFFFFFF	  // front
	};

	const v3 normals[] =
	{
		v3(0, 0, 1)
	};

	const v2 texCoords[] = {
		v2(0, 1),
		v2(1, 1),
		v2(1, 0),
		v2(0, 0)
	};

	MeshVertex verts[] = {
		MeshVertex(v3(-s, -s, 0.f), colours[0], normals[0], texCoords[0]),
		MeshVertex(v3(s, -s, 0.f), colours[0], normals[0], texCoords[1]),
		MeshVertex(v3(s, s, 0.f), colours[0], normals[0], texCoords[2]),
		MeshVertex(v3(-s, s, 0.f), colours[0], normals[0], texCoords[3]),
	};

	const u32 kVertices = sizeof(verts) / sizeof(verts[0]);

	// and indices
	const u16 indices[] = {
		0,  1,  2, 
		0,  2,  3
	};

	const u32 kIndices = sizeof(indices) / sizeof(indices[0]);

	compute_tangents_lengyel(verts, kVertices, indices, kIndices);

	rMeshOut.init_buffers(pDevice, verts, kVertices, indices, kIndices, meshName);
}

void create_mesh_from_obj(ID3D11Device* pDevice, Mesh& rMeshOut, const char* pFilename, const char* mtlBaseDir, 
	const f32 kScale, const std::string& meshName)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::vector<MeshVertex> meshVertices;

	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, pFilename, mtlBaseDir);

	if (!err.empty()) { // `err` may contain warning message.
		debugF("load_obj_mesh( %s ) : %s", pFilename, err.c_str());
	}

	if (!ret) {
		panicF("Error Loading OBJ %s", pFilename);
	}

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {

		meshVertices.clear();

		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) 
		{
			u32 fv = shapes[s].mesh.num_face_vertices[f];

			// Flip the winding order here to match DX
			const u32 reorder[] = { 0, 2, 1 };

			// Loop over vertices in the face.
			for (u32 v = 0; v < fv; v++) {

				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + reorder[v]];
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];

				// Optional: vertex colors
				// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
				// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
				// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];


				// Flip Z in both position and normal to match DX coordinate system.
				// Export Obj from Blender with (-Z forward) should produce correct results.
				v3 pos = v3(vx, vy, -vz) * kScale;
				v3 normal(nx, ny, -nz);
				normal.Normalize();

				// Flip UV y to match DX texture flipping.
				v2 uv(tx,-ty);

				meshVertices.push_back(MeshVertex(pos, 0xFFFFFFFF, normal, uv));
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}

		// Make a sequential index buffer so we can use the tangent calculation function
		std::vector<u16> m_indices(meshVertices.size());
		for (u32 i = 0; i < meshVertices.size(); ++i)
		{
			m_indices[i] = i;
		}

		// compute the tangents,
		compute_tangents_lengyel(&meshVertices[0], meshVertices.size(), &m_indices[0], m_indices.size());

		rMeshOut.init_buffers(pDevice, &meshVertices[0], meshVertices.size(), &m_indices[0], m_indices.size(), meshName);
	}
}
