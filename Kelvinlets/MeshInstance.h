#pragma once

#include "KelvinletManager.h"
#include "KDisplacementManager.h"
#include "Mesh.h"

class Texture;

class MeshInstance
{
public:
	MeshInstance(size_t id, v3 pos, Mesh& rMesh, Texture& rTexture, u32 numKelvinlets) :
		m_id(id),
		m_position(pos),
		m_pMesh(&rMesh),
		m_pTexture(&rTexture),
		m_kelvinletManager(numKelvinlets),
		m_kDisplacementManager(rMesh.num_vertices())
	{}
	MeshInstance(const MeshInstance&) = delete;
	MeshInstance& operator=(const MeshInstance&) = delete;
	MeshInstance(MeshInstance&&);	// Move constructor
	MeshInstance& operator=(MeshInstance&&);	// Move assignment
	~MeshInstance();

	const size_t get_id() const { return m_id; }
	const Mesh& get_mesh() const { return *m_pMesh; }
	const Texture& get_texture() const { return *m_pTexture; }
	
	void set_mesh(SystemsInterface&, Mesh&);
	void set_texture(Texture& tex) { m_pTexture = &tex; }

	void init(SystemsInterface&);
	void update(SystemsInterface&, float);
	void render(SystemsInterface&);
	void release();

	void play();
	void pause();
	void stop(ID3D11DeviceContext*);

	v3& get_position() { return m_position; }
	const std::string& get_mesh_name() const;

	KelvinletManager& get_kelvinlet_manager() { return m_kelvinletManager; }
	const KelvinletManager& get_kelvinlet_manager() const { return m_kelvinletManager; }
	KDisplacementManager& get_displacement_manager() { return m_kDisplacementManager; }
	const KDisplacementManager& get_displacement_manager() const { return m_kDisplacementManager; }

private:
	size_t m_id;	
	v3 m_position;
	Mesh* m_pMesh = nullptr;
	Texture* m_pTexture = nullptr;
	KelvinletManager m_kelvinletManager;
	KDisplacementManager m_kDisplacementManager;
};