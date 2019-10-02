#include "MeshInstance.h"

// Move constructor
MeshInstance::MeshInstance(MeshInstance&& other) :
	m_id(other.m_id),
	m_position(other.m_position),
	m_pMesh(nullptr),
	m_pTexture(nullptr),
	m_kelvinletManager(std::move(other.m_kelvinletManager)),
	m_kDisplacementManager(std::move(other.m_kDisplacementManager))
{
	m_pMesh = other.m_pMesh;
	m_pTexture = other.m_pTexture;
	other.m_pMesh = nullptr;
	other.m_pTexture = nullptr;
}

MeshInstance& MeshInstance::operator=(MeshInstance&& other)
{
	if (this != &other)
	{
		m_id = other.m_id;
		m_position = other.m_position;
		m_pMesh = other.m_pMesh;
		m_pTexture = other.m_pTexture;
		m_kelvinletManager = std::move(other.m_kelvinletManager);
		m_kDisplacementManager = std::move(other.m_kDisplacementManager);
	}
	
	return *this;
}

MeshInstance::~MeshInstance()
{
	release();
}

void MeshInstance::set_mesh(SystemsInterface& systems, Mesh& rMesh)
{
	// Set the new mesh
	m_pMesh = &rMesh;
	// Resize the displacement buffer
	m_kDisplacementManager.resize_displacement_buffer(systems, rMesh.num_vertices());
}

void MeshInstance::init(SystemsInterface& systems)
{
	m_kelvinletManager.init(systems);
	m_kDisplacementManager.init(systems);
}

void MeshInstance::update(SystemsInterface& systems, float dt)
{
	m_kDisplacementManager.update(systems);
	m_kelvinletManager.update(systems, dt);
}

void MeshInstance::render(SystemsInterface& systems)
{
	m_pMesh->draw(systems.pD3DContext);
}

void MeshInstance::release()
{}

void MeshInstance::play()
{
	m_kelvinletManager.play();
}

void MeshInstance::pause()
{
	m_kelvinletManager.pause();
}

void MeshInstance::stop(ID3D11DeviceContext* pContext)
{
	m_kelvinletManager.stop();
}

const std::string& MeshInstance::get_mesh_name() const
{
	return m_pMesh->get_name();
}