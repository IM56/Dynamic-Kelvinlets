#include "MeshManager.h"

MeshManager::~MeshManager()
{
	release();
}

void MeshManager::init(SystemsInterface& systems)
{
	// Load a meshes from file
	Mesh sphere;
	create_mesh_from_obj(systems.pD3DDevice, sphere, "Assets/Models/Sphere.obj", NULL, 10.0f, "Sphere");
	m_meshes.insert({ sphere.get_name(), std::move(sphere) });

	Mesh cube;
	create_mesh_from_obj(systems.pD3DDevice, cube, "Assets/Models/Cube.obj", NULL, 10.0f, "Cube");
	m_meshes.insert({ cube.get_name(), std::move(cube) });

	Mesh lp_sphere;
	create_mesh_from_obj(systems.pD3DDevice, lp_sphere, "Assets/Models/LP_Sphere.obj", NULL, 0.1f, "LP_Sphere");
	m_meshes.insert({ lp_sphere.get_name(), std::move(lp_sphere) });

	Mesh c_shape;
	create_mesh_from_obj(systems.pD3DDevice, c_shape, "Assets/Models/C_Shape.obj", NULL, 0.05f, "C_Shape");
	m_meshes.insert({ c_shape.get_name(), std::move(c_shape) });
}

MeshManager::MeshMap::iterator MeshManager::begin()
{
	return m_meshes.begin();
}

MeshManager::MeshMap::const_iterator MeshManager::cbegin() const
{
	return m_meshes.cbegin();
}

MeshManager::MeshMap::iterator MeshManager::end()
{
	return m_meshes.end();
}

MeshManager::MeshMap::const_iterator MeshManager::cend() const
{
	return m_meshes.cend();
}

Mesh& MeshManager::get_mesh(const std::string& name)
{
	MeshMap::iterator it = m_meshes.find(name);
	
	// If we cannot find the mesh by name, return a placeholder cube by default
	if (it != end())
		return it->second;
	else
		return m_placeholder;
}

const Mesh& MeshManager::get_mesh(const std::string& name) const
{
	MeshMap::const_iterator it = m_meshes.find(name);

	// If we cannot find the mesh by name, return a placeholder cube by default
	if (it != cend())
		return it->second;
	else
		return m_placeholder;
}