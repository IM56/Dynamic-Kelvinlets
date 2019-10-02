#include "MeshInstanceManager.h"

//=================================================================================================
// Custom comparator for ordering by mesh names lexically - useful for sorting mesh instances
//=================================================================================================

struct MeshInstanceNameCompare
{
	bool operator()(const MeshInstance& lhs, const MeshInstance& rhs) const
	{
		return (lhs.get_mesh_name() < rhs.get_mesh_name());
	}
};

// Counter that increments, giving each mesh instance a unique ID
size_t MeshInstanceManager::m_id = 0;

MeshInstanceManager::~MeshInstanceManager()
{
	release();
}

void MeshInstanceManager::init(SystemsInterface& systems)
{}

void MeshInstanceManager::update(SystemsInterface& systems, float dt)
{
	for (auto it = m_meshInstances.begin(); it != m_meshInstances.end(); ++it)
		it->update(systems, dt);
}

void MeshInstanceManager::release() 
{
	for (auto it = m_meshInstances.begin(); it != m_meshInstances.end(); ++it)
		it->release();
}

void MeshInstanceManager::play()
{
	for (auto it = m_meshInstances.begin(); it != m_meshInstances.end(); ++it)
		it->play();
}

void MeshInstanceManager::pause()
{
	for (auto it = m_meshInstances.begin(); it != m_meshInstances.end(); ++it)
		it->pause();
}

void MeshInstanceManager::stop(ID3D11DeviceContext* pContext)
{
	for (auto it = m_meshInstances.begin(); it != m_meshInstances.end(); ++it)
		it->stop(pContext);
}

void MeshInstanceManager::add_mesh_instance(SystemsInterface& systems, v3 pos, Mesh& rMesh, Texture& rTexture, u32 numKs)
{
	m_id += 1;	// Increment the ID counter
	MeshInstance m(m_id, pos, rMesh, rTexture, numKs);
	m.init(systems);
	m_meshInstances.push_back(std::move(m));
}

void MeshInstanceManager::remove_mesh_instance(size_t x)
{
	// Ensure that this is a valid ID
	ASSERT(x >= 0 && x <= m_id);
	m_meshInstances.erase(begin() + x);
}

std::vector<MeshInstance>::iterator MeshInstanceManager::begin()
{
	return m_meshInstances.begin();
}

std::vector<MeshInstance>::const_iterator MeshInstanceManager::cbegin() const
{
	return m_meshInstances.cbegin();
}

std::vector<MeshInstance>::iterator MeshInstanceManager::end()
{
	return m_meshInstances.end();
}

std::vector<MeshInstance>::const_iterator MeshInstanceManager::cend() const
{
	return m_meshInstances.cend();
}

void MeshInstanceManager::sort_instances_by_mesh()
{
	std::sort(m_meshInstances.begin(), m_meshInstances.end(), MeshInstanceNameCompare());
}