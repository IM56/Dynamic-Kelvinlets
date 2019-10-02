#pragma once
#include "Manager.h"
#include "Mesh.h"

#include <map>
#include <string>

class MeshManager : public Manager<MeshManager>
{
	using MeshMap = std::map<std::string, Mesh>;

public:
	MeshManager(){}
	~MeshManager();

	void init(SystemsInterface&);
	void update(SystemsInterface& systems) {}
	void release() {}

	MeshMap::iterator begin();
	MeshMap::const_iterator cbegin() const;
	MeshMap::iterator end();
	MeshMap::const_iterator cend() const;

	Mesh& get_mesh(const std::string&);
	const Mesh& get_mesh(const std::string&) const;

private:
	MeshMap m_meshes;
	Mesh m_placeholder;		// Placeholder mesh to return
};